#ifndef __VFSM_HPP__
#define __VFSM_HPP__

#include <FAT32.hpp>
#include <kstring.hpp>



class VFSM
{
private:
    
    FAT32FILE * OpenedFile;
    
    FAT32FILE * find_file_by_path(char * path,bool & isOpened); 
public:
    FAT32FILE * open(const char * path,char * cwd); 
    void close(FAT32FILE *t);

    bool create_file(const char * path,char * cwd,char * fileName,uint8 type=FATtable::FILE);
    bool create_dir(const char * path,char * cwd,char * dirName);
    bool del_file(const char *path,char * cwd);
    bool link(const char * srcpath,const char * dstpath,char * cwd);
    bool unlink(const char * path,char * cwd);

    char * unified_path(const char * path,char * cwd);
    void show_opened_file();
    bool init();
    bool destory(); 
    FAT32FILE * get_root();
};

extern VFSM vfsm;



#endif