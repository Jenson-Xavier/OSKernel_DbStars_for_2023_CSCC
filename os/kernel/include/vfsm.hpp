#ifndef __VFSM_HPP__
#define __VFSM_HPP__

#include <FAT32.hpp>
#include <kstring.hpp>

class VFSM
{
private:
    FAT32FILE* OpenedFile;

    FAT32FILE* find_file_by_path(char* path, bool& isOpened);

public:
    FAT32FILE* open(const char* path, char* cwd);
    FAT32FILE* open(FAT32FILE* file);
    void close(FAT32FILE* t);

    bool create_file(const char* path, char* cwd, char* fileName, uint8 type = FATtable::FILE);
    bool create_dir(const char* path, char* cwd, char* dirName);
    bool del_file(const char* path, char* cwd);
    bool link(const char* srcpath, const char* ref_path, char* cwd);//ref_path为被指向的文件
    bool unlink(const char* path, char* cwd);
    FAT32FILE* get_next_file(FAT32FILE* dir, FAT32FILE* cur = nullptr, bool (*p)(FATtable* temp) = VALID); // 获取到的dir下cur的下一个满足p条件的文件，如果没有则返回空

    char* unified_path(const char* path, char* cwd);
    void show_opened_file();
    bool init();
    bool destory();
    FAT32FILE* get_root();
};

extern VFSM vfsm;

#endif