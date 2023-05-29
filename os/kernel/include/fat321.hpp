#ifndef __FAT32_HPP__
#define __FAT32_HPP__
#include <vfs.hpp>
#include <ramdisk_driver.hpp>

struct DBR
{
    uint32 BPB_sector_size;
    uint32 BPB_sector_per_clus;
    uint32 BPB_rs_sector_num;
    uint32 BPB_FAT_num;
    uint32 BPB_all_sector_num;
    uint32 BPB_sector_per_FAT;
    uint32 BPB_root_clus;


};


class FAT32FileNode:public FileNode
{
    friend class FAT32;
private:
    uint32 FirstClus;    
    uint64 readsize;
    uint64 contentLba;
    uint64 contentOffset;

    int64 read(void *dst, uint64 pos, uint64 size);
    int64 write(void *src, uint64 pos, uint64 size);
    bool set_name(char *_name);
    bool del();

public:
    FAT32FileNode(/* args */);
    ~FAT32FileNode();
};

class FAT32 : public VFS
{

public:

    DBR Dbr;
    uint64 DBPLba;
    uint64 FAT1Lab;
    uint64 FAT2Lab;
    uint64 RootLab;
    FAT32Device fat32dev;


    bool init();
    FAT32();
    ~FAT32();
};



#endif