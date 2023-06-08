FAT32文件系统提供对实际FAT文件的管理。
```cpp
class FAT32
{
    friend class FAT32FILE;
    friend class VFSM;

private:
    uint64 DBRlba;
    uint64 FAT1lba;
    uint64 FAT2lba;
    uint64 DATAlba;



public:
    DBR Dbr;
    FAT32Device dev;
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
```
该文件系统提供了打开，关闭，创建，删除等常用操作。同时FAT32中的基本操作单元为簇，FAT32提供了读取簇和写入簇的操作。而实际对象中除了FAT32中基本的数值还有FAT32Device对实际硬盘进行读写操作。<br />FAT32的返回值为FAT32FILE文件，该文件为文件的基本操作单位。
```cpp
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

```
By：郭伟鑫
