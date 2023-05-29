#ifndef __RAMDISK_DRIVER_HPP__
#define __RAMDISK_DRIVER_HPP__

#include <type.hpp>
#include <kstring.hpp>
#include <kout.hpp>

class storage
{
    virtual bool init()=0;
    virtual bool read(uint64 lba,unsigned char * buf)=0;
    virtual bool write(uint64 lba,unsigned char * buf)=0;
};

class  FAT32Device:public storage
{
public:
    bool init();
    bool read(uint64 lba,unsigned char * buf);
    bool write(uint64 lba,unsigned char * buf);
};
// extern FAT32Device fat32dev;

extern uint64 sector_size;
extern uint64 ramdisk_base;
extern uint64 ramdisk_base_k;
extern uint64 ramdisk_sector_count;





#endif