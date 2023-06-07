//
// driver for qemu's virtio disk device.
// uses qemu's mmio interface to virtio.
// qemu presents a "legacy" virtio interface.
//
// qemu ... -drive file=fs.img,if=none,format=raw,id=x0 -device virtio-blk-device,drive=x0,bus=virtio-mmio-bus.0
//

// #include "include/types.h"
// #include "include/riscv.h"
// #include "include/param.h"
// #include "include/memlayout.h"
// #include "include/spinlock.h"
// #include "include/sleeplock.h"
// #include "include/buf.h"
// #include "include/virtio.h"
// #include "include/proc.h"
// #include "include/vm.h"
// #include "include/string.h"
// #include "include/printf.h"
#include <virtio.h>
#include <Disk.hpp>
#include <interrupt.hpp>
#include <pmm.hpp>
#include <spinlock.hpp>
#include <synchronize.hpp>

// the address of virtio mmio register r.
#define R(r) ((volatile uint32 *)(VIRTIO0_V + (r)))
// #define R(r) ((volatile uint32 *)(0))

static struct disk
{
  // memory for virtio descriptors &c for queue 0.
  // this is a global instead of allocated because it must
  // be multiple contiguous pages, which kalloc()
  // doesn't support, and page aligned.
  char pages[2 * PAGESIZE];
  struct VRingDesc* desc;
  uint16* avail;
  struct UsedArea* used;

  // our own book-keeping.
  char free[NUM];  // is a descriptor free?
  uint16 used_idx; // we've looked this far in used[2..NUM].

  // track info about in-flight operations,
  // for use when completion interrupt arrives.
  // indexed by first descriptor index of chain.
  struct
  {
    //    struct buf *b;
    Uint8* buf;
    SEMAPHORE* sem = nullptr;
    char status;
  } info[NUM];

  //  struct spinlock vdisk_lock;
  SPIN_LOCK lock;
  SEMAPHORE* sem = nullptr;

} __attribute__((aligned(PAGESIZE))) disk;

void virtio_disk_init(void)
{
  uint32 status = 0;

  //  initlock(&disk.vdisk_lock, "virtio_disk");
  disk.lock.init();
  disk.sem = new SEMAPHORE;
  disk.sem->init();

  if (*R(VIRTIO_MMIO_MAGIC_VALUE) != 0x74726976 ||
    *R(VIRTIO_MMIO_VERSION) != 1 ||
    *R(VIRTIO_MMIO_DEVICE_ID) != 2 ||
    *R(VIRTIO_MMIO_VENDOR_ID) != 0x554d4551)
  {
    //    panic("could not find virtio disk");
    kout[red] << "Cannot find VirtIO Disk!" << endl;
  }

  status |= VIRTIO_CONFIG_S_ACKNOWLEDGE;
  *R(VIRTIO_MMIO_STATUS) = status;

  status |= VIRTIO_CONFIG_S_DRIVER;
  *R(VIRTIO_MMIO_STATUS) = status;

  // negotiate features
  uint64 features = *R(VIRTIO_MMIO_DEVICE_FEATURES);
  features &= ~(1 << VIRTIO_BLK_F_RO);
  features &= ~(1 << VIRTIO_BLK_F_SCSI);
  features &= ~(1 << VIRTIO_BLK_F_CONFIG_WCE);
  features &= ~(1 << VIRTIO_BLK_F_MQ);
  features &= ~(1 << VIRTIO_F_ANY_LAYOUT);
  features &= ~(1 << VIRTIO_RING_F_EVENT_IDX);
  features &= ~(1 << VIRTIO_RING_F_INDIRECT_DESC);
  *R(VIRTIO_MMIO_DRIVER_FEATURES) = features;

  // tell device that feature negotiation is complete.
  status |= VIRTIO_CONFIG_S_FEATURES_OK;
  *R(VIRTIO_MMIO_STATUS) = status;

  // tell device we're completely ready.
  status |= VIRTIO_CONFIG_S_DRIVER_OK;
  *R(VIRTIO_MMIO_STATUS) = status;

  *R(VIRTIO_MMIO_GUEST_PAGE_SIZE) = PAGESIZE;

  // initialize queue 0.
  *R(VIRTIO_MMIO_QUEUE_SEL) = 0;
  uint32 max = *R(VIRTIO_MMIO_QUEUE_NUM_MAX);
  if (max == 0)
    //    panic("virtio disk has no queue 0");
    kout[red] << "VirtIO Disk has no queue 0!" << endl;
  if (max < NUM)
    kout[red] << "VirtIO Disk max queue too short!" << endl;
  *R(VIRTIO_MMIO_QUEUE_NUM) = NUM;
  memset(disk.pages, 0, sizeof(disk.pages));
  *R(VIRTIO_MMIO_QUEUE_PFN) = ((uint64)disk.pages - PhysiclaVirtualMemoryOffset) >> 12;

  // desc = pages -- num * VRingDesc
  // avail = pages + 0x40 -- 2 * uint16, then num * uint16
  // used = pages + 4096 -- 2 * uint16, then num * vRingUsedElem

  disk.desc = (struct VRingDesc*)disk.pages;
  disk.avail = (uint16*)(((char*)disk.desc) + NUM * sizeof(struct VRingDesc));
  disk.used = (struct UsedArea*)(disk.pages + PAGESIZE);

  for (int i = 0; i < NUM; i++)
    disk.free[i] = 1;

  // plic.c and trap.c arrange for interrupts from VIRTIO0_IRQ.
  //  #ifdef DEBUG
  //  printf("virtio_disk_init\n");
  //  #endif
}

// find a free descriptor, mark it non-free, return its index.
static int
alloc_desc()
{
  for (int i = 0; i < NUM; i++)
  {
    if (disk.free[i])
    {
      disk.free[i] = 0;
      return i;
    }
  }
  return -1;
}

// mark a descriptor as free.
static void
free_desc(int i)
{
  if (i >= NUM)
    //    panic("virtio_disk_intr 1");
    kout[red] << "virtio_disk_intr 1" << endl;
  if (disk.free[i])
    //    panic("virtio_disk_intr 2");
    kout[red] << "virtio_disk_intr 2" << endl;
  disk.desc[i].addr = 0;
  disk.free[i] = 1;
  //  wakeup(&disk.free[0]);
  disk.sem->signal();
}

// free a chain of descriptors.
static void
free_chain(int i)
{
  while (1)
  {
    free_desc(i);
    if (disk.desc[i].flags & VRING_DESC_F_NEXT)
      i = disk.desc[i].next;
    else
      break;
  }
}

static int
alloc3_desc(int* idx)
{
  for (int i = 0; i < 3; i++)
  {
    idx[i] = alloc_desc();
    if (idx[i] < 0)
    {
      for (int j = 0; j < i; j++)
        free_desc(idx[j]);
      return -1;
    }
  }
  return 0;
}

void virtio_disk_rw(Uint8* buf, uint64 sector, int write)
{
  //  uint64 sector = b->sectorno;

  //  acquire(&disk.vdisk_lock);
  disk.lock.lock();
  // the spec says that legacy block operations use three
  // descriptors: one for type/reserved/sector, one for
  // the data, one for a 1-byte status result.

  // allocate the three descriptors.
  int idx[3];
  while (1)
  {
    if (alloc3_desc(idx) == 0)
    {
      break;
    }
    disk.lock.unlock();
    pm.set_waittime_limit(pm.get_cur_proc(), 18446744073709551615);
    disk.sem->wait();
    disk.lock.lock();
  }

  // format the three descriptors.
  // qemu's virtio-blk.c reads them.

  struct virtio_blk_outhdr
  {
    uint32 type;
    uint32 reserved;
    uint64 sector;
  } buf0;
  {

    if (write)
      buf0.type = VIRTIO_BLK_T_OUT; // write the disk
    else
      buf0.type = VIRTIO_BLK_T_IN; // read the disk
    buf0.reserved = 0;
    buf0.sector = sector;

    disk.desc[idx[0]].addr = (uint64)&buf0 - PhysiclaVirtualMemoryOffset;
    disk.desc[idx[0]].len = sizeof(buf0);
    disk.desc[idx[0]].flags = VRING_DESC_F_NEXT;
    disk.desc[idx[0]].next = idx[1];

    disk.desc[idx[1]].addr = (uint64)buf - PhysiclaVirtualMemoryOffset;
    disk.desc[idx[1]].len = SectorSize; // BSIZE;
    if (write)
      disk.desc[idx[1]].flags = 0; // device reads b->data
    else
      disk.desc[idx[1]].flags = VRING_DESC_F_WRITE; // device writes b->data
    disk.desc[idx[1]].flags |= VRING_DESC_F_NEXT;
    disk.desc[idx[1]].next = idx[2];

    disk.info[idx[0]].status = 0;
    disk.desc[idx[2]].addr = (uint64)&disk.info[idx[0]].status - PhysiclaVirtualMemoryOffset;
    disk.desc[idx[2]].len = 1;
    disk.desc[idx[2]].flags = VRING_DESC_F_WRITE; // device writes the status
    disk.desc[idx[2]].next = 0;

    if (disk.info[idx[0]].sem == nullptr)
    {
      disk.info[idx[0]].sem = new SEMAPHORE;
      disk.info[idx[0]].sem->init();
    }
    disk.info[idx[0]].buf = buf;
  }
  // avail[1] tells the device how far to look in avail[2...].
  // avail[2...] are desc[] indices the device should process.
  // we only tell device the first index in our chain of descriptors.
  {
    disk.avail[2 + (disk.avail[1] % NUM)] = idx[0];
    __sync_synchronize();
    disk.avail[1] = disk.avail[1] + 1;

    disk.lock.unlock();
    //	while (disk.info[idx[0]].sem->Value()!=0)
    //		kout[Fault]<<POS_PM.Current()->GetPID()<<" sem "<<disk.info[idx[0]].sem<<" "<<disk.info[idx[0]].sem->Value()<<" | "<<buf<<" "<<sector<<endl;

    *R(VIRTIO_MMIO_QUEUE_NOTIFY) = 0; // value is queue number

    // Wait for virtio_disk_intr() to say request has finished.
    //  while(b->disk == 1) {
    //    sleep(b, &disk.vdisk_lock);
    //  }
    //	kout[Debug]<<POS_PM.Current()->GetPID()<<" Wait "<<disk.info[idx[0]].sem<<" | "<<buf<<" "<<sector<<endl;
    pm.set_waittime_limit(pm.get_cur_proc(), 18446744073709551615);
    disk.info[idx[0]].sem->wait();
    disk.lock.lock();

    // kout[Debug]<<"Wait OK"<<endl;

    //  disk.info[idx[0]].b = 0;
    free_chain(idx[0]);
    disk.lock.unlock();
    //  release(&disk.vdisk_lock);

    //	for (int i=0;i<=1e4;++i);//Debug...
  }
}

void virtio_disk_intr()
{

  disk.lock.lock();
  while ((disk.used_idx % NUM) != (disk.used->id % NUM))
  {
    int id = disk.used->elems[disk.used_idx].id;

    int i;
    for (i = 0; disk.info[id].status != 0 && i <= 10; ++i)
      if (i >= 100)
        kout[red] << "virtio_disk_intr status " << (int)disk.info[id].status << endl;
    disk.info[id].sem->signal();

    disk.used_idx = (disk.used_idx + 1) % NUM;
  }
  *R(VIRTIO_MMIO_INTERRUPT_ACK) = *R(VIRTIO_MMIO_INTERRUPT_STATUS) & 0x3;
  disk.lock.unlock();
}

extern "C"
{
#include <_plic.h>
};

bool DiskInit()
{
  kout[yellow] << "Disk init..." << endl;
  kout[yellow] << "plic init..." << endl;
  // kout[red]<<Hex(f())<<endl;
  plicinit();

  kout[yellow] << "plic init hart..." << endl;
  plicinithart();
  kout[yellow] << "VirtIO disk init..." << endl;
  virtio_disk_init();

  kout[yellow] << "Disk init OK!" << endl;
  kout << endl;
  return true;
}

bool DiskReadSector(unsigned long long LBA, Sector* sec, int cnt)
{
  // bool intr_flag;
  // intr_save(intr_flag);

  for (int i = 0; i < cnt; ++i)
  {
    virtio_disk_rw((Uint8*)(sec + i), LBA + i, 0);
  }
  // intr_restore(intr_flag);
  return true;
}

bool DiskWriteSector(unsigned long long LBA, const Sector* sec, int cnt)
{
  for (int i = 0; i < cnt; ++i)
    virtio_disk_rw((Uint8*)(sec + i), LBA + i, 1);
  return true;
}

bool DiskInterruptSolve()
{
  virtio_disk_intr();
  return true;
}
