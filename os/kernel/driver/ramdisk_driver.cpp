#include <ramdisk_driver.hpp>
#include <Disk.hpp>
#include <clock.hpp>

uint64 sector_size;
uint64 ramdisk_base;
uint64 ramdisk_base_k;
uint64 ramdisk_sector_count;

bool FAT32Device::init()
{

    char* ramdisk = new char[10 * 1024 * 1024];
    DiskReadSector(0, (Sector*)ramdisk, 1024 * 10 * 2);

    // ramdisk_base = ramdisk;
    ramdisk_base_k = (uint64)ramdisk;
    unsigned char* t;
    t = (unsigned char*)(ramdisk_base_k + 11);

    sector_size = 512;
    ramdisk_sector_count = 4 * 1024 * 1024 / sector_size;

    return true;
}

bool FAT32Device::read(uint64 lba, unsigned char* buf)
{
    if (lba > ramdisk_sector_count)
        return false;
    memcpy(buf, (char*)(lba * sector_size + ramdisk_base_k), sector_size);
    // DiskReadSector(lba, (Sector*)buf);
    // delay(200);

    return true;
}

bool FAT32Device::write(uint64 lba, unsigned char* buf)
{
    if (lba > ramdisk_sector_count)
        return false;
    memcpy((char*)(lba * sector_size + ramdisk_base_k), (char*)buf, sector_size);
    // DiskReadSector(lba, (Sector*)buf);
    // delay(100);
    return true;
}

// FAT32Device fat32dev;