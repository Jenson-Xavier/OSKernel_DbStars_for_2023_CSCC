由于时间紧张本操作系统仅支持FAT32文件系统，但是为了实现挂载，以及将来的扩展，本操作系统实现了虚拟文件管理器VFSM，用于对所有文件进行管理。
```cpp
class VFSM
{
private:
    FAT32FILE *OpenedFile;

    FAT32FILE *find_file_by_path(char *path, bool &isOpened);

public:
    FAT32FILE *open(const char *path, char *cwd);
    FAT32FILE *open(FAT32FILE *file);
    void close(FAT32FILE *t);

    bool create_file(const char *path, char *cwd, char *fileName, uint8 type = FATtable::FILE);
    bool create_dir(const char *path, char *cwd, char *dirName);
    bool del_file(const char *path, char *cwd);
    bool link(const char *srcpath, const char *dstpath, char *cwd);
    bool unlink(const char *path, char *cwd);
    FAT32FILE *get_next_file(FAT32FILE *dir, FAT32FILE *cur = nullptr, bool (*p)(FATtable *temp) = VALID); // 获取到的dir下cur的下一个满足p条件的文件，如果没有则返回空

    char *unified_path(const char *path, char *cwd);
    void show_opened_file();
    bool init();
    bool destory();
    FAT32FILE *get_root();
};
```
VFSM中用链表将所有打开的文件进行管理。由于只有FAT32一种文件，则直接使用FAT32FILE链表，对于所有的文件，都需要用Open函数打开，插入链表当中。对于文件的特殊操作则由文件指针的对象提供，而对于所有文件对象都提供的操作则由VFSM进行统一管理。<br />同时通过VFSM打开的文件将都会获取一个统一的绝对路径用于管理。而在初始化时则需要提供默认的文件系统，将其插入文件头部。对于搜索文件则先在打开的文件结点中搜索是否存在，或者存在上级文件夹且为VFS属性文件，如果存在则调用VFS文件夹中的查找。<br />By：郭伟鑫

