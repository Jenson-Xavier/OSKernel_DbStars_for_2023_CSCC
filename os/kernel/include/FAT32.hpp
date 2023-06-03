#ifndef _FAT32_HPP__
#define _FAT32_HPP__

#include <ramdisk_driver.hpp>
#include <type.hpp>
#include <kout.hpp>
#include <pathtool.hpp>
#include <kstring.hpp>

struct DBR
{
    uint32 FAT_num;
    uint32 FAT_sector_num;
    uint32 sector_size;
    uint32 clus_sector_num;
    uint32 rs_sector_num;
    uint32 all_sector_num;
    uint32 root_clus;
    uint32 clus_size;
};

union FATtable
{

    enum :uint8
    {
        READ_ONLY = 0x1 << 0,
        HIDDEN = 0x1 << 1,
        SYSTEM = 0x1 << 2,
        VOLUME_LABEL = 0x1 << 3,
        SUBDIRENCTORY = 0x1 << 4,
        FILE = 0x1 << 5,
        DEVICE = 0x1 << 6,
    };
    struct
    {
        char name[8];

        uint8 ex_name[3];
        uint8 type;
        uint8 rs;
        uint8 ns;
        uint8 S_time[2]; // 创建时间

        uint8 S_date[2];
        uint8 C_time[2]; // 访问时间
        uint16 high_clus;
        uint8 M_time[2]; // 修改时间

        uint8 M_date[2];
        uint16 low_clus;
        uint32 size;
    };
    struct
    {
        uint8 attribute;
        uint8 lname0[10];
        uint8 type1;
        uint8 rs2;
        uint8 check;
        uint8 lname1[12];
        uint16 clus1;
        uint8 lname2[4];
    };

    int32 get_name(char* REname); // 返回值为-1时说明无效，0为短名称，大于1的序列为长名称(长名称无法完成拷贝),-2为点目录
};
bool ALLTURE(FATtable* t);
bool VALID(FATtable* t);
bool EXCEPTDOT(FATtable* t);
class FAT32;
class VFSM;

class FAT32FILE
{
public:
    enum : uint32
    {
        __DIR = 1ull << 0,
        __ROOT = 1ull << 1,
        __VFS = 1ull << 2,
        __DEVICE = 1ull << 3,
        __TEMP = 1ull << 4,
        __LINK = 1ull << 5,
        __SPECICAL = 1ull << 6,
    };
    char* name = nullptr;
    uint32 TYPE;
    FATtable table;
    uint64 table_clus_pos;
    uint64 table_clus_off;
    uint32 clus;

    FAT32FILE* next = nullptr;
    FAT32FILE* pre = nullptr;

    FAT32* fat;
    uint64 ref;
    char* path;

    FAT32FILE(FATtable tb, char* lName, uint64 _clus = 0, uint64 pos = 0, char* path = nullptr);
    ~FAT32FILE();
    bool set_path(char* _path);

    int64 read(unsigned char* buf, uint64 size);
    int64 read(unsigned char* buf, uint64 pos, uint64 size);
    bool write(unsigned char* src, uint64 size);
    bool write(unsigned char* src, uint64 pos, uint64 size);

    void show();
};

class FAT32
{
    friend class FAT32FILE;
    friend class VFSM;

private:
    DBR Dbr;
    uint64 DBRlba;
    uint64 FAT1lba;
    uint64 FAT2lba;
    uint64 DATAlba;

    FAT32Device dev;

    unsigned char* temp;

public:
    uint64 clus_to_lba(uint64 clus);
    uint64 lba_to_clus(uint64 lba);
    FAT32FILE* get_child_form_clus(char* child_name, uint64 src_lba); // 返回文件table

    bool get_clus(uint64 clus, unsigned char* buf);
    bool get_clus(uint64 clus, unsigned char* buf, uint64 start, uint64 end);
    bool set_clus(uint64 clus, unsigned char* buf);
    bool set_clus(uint64 clus, unsigned char* buf, uint64 start, uint64 end);

    uint64 find_empty_clus();
    bool set_table(FAT32FILE* file);
    uint32 get_next_clus(uint32 clus);
    bool set_next_clus(uint32 clus, uint32 nxt_clus);


    bool init();
    FAT32();
    ~FAT32();
    FAT32FILE* find_file_by_path(char* path);
    FAT32FILE* open(char* path);
    bool close(FAT32FILE* p);
    bool link();
    bool unlink();
    FAT32FILE* create_file(FAT32FILE* dir, char* fileName, uint8 type = FATtable::FILE);
    FAT32FILE* create_dir(FAT32FILE* dir, char* fileName);
    bool del_file(FAT32FILE* file);                                                                        // 可用于删除文件夹
    FAT32FILE* get_next_file(FAT32FILE* dir, FAT32FILE* cur = nullptr, bool (*p)(FATtable* temp) = VALID); // 获取到的dir下cur的下一个满足p条件的文件，如果没有则返回空
};

#endif