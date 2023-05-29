#include <ramdisk_driver.hpp>


uint64 sector_size;
uint64 ramdisk_base;
uint64 ramdisk_base_k;
uint64 ramdisk_sector_count;

bool FAT32Device::init()
{

    ramdisk_base = 0x88200000;
    ramdisk_base_k = ramdisk_base + 0xffffffff00000000;
    
    unsigned char * t;
    t=(unsigned char *)(ramdisk_base_k+11);
    sector_size=*(t+1)*256+*(t);
    ramdisk_sector_count = 4 * 1024 * 1024 / sector_size;
    
    return true;
}
bool FAT32Device::read(uint64 lba, unsigned char *buf)
{
    if (lba > ramdisk_sector_count)
        return false;
    memcpy(buf, (char *)(lba * sector_size + ramdisk_base_k), sector_size);
    return true;
}

bool FAT32Device::write(uint64 lba, unsigned char *buf)
{
    if (lba > ramdisk_sector_count)
        return false;
    memcpy((char *)(lba * sector_size + ramdisk_base_k), (char *)buf, sector_size);
    return true;
}

// FAT32Device fat32dev;