#ifndef __FVS_HPP__
#define __FVS_HPP__

#include <type.hpp>
#include <kstring.hpp>
#include <vmm.hpp>
#include <kout.hpp>
#include <pathtool.hpp>

inline const char* INVALIDPATHCHAR()
{
    return "/\\:*?\"<>|";
}

class VFS;
class VFSM;
class FileHandle;

class FileNode
{
    friend class VFS;

private:
    enum : uint64
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
    VFS* vfs = nullptr;
    uint64 TYPE;
    FileNode* parent = nullptr,
        * pre = nullptr,
        * next = nullptr,
        * child = nullptr;

    uint64 fileSize = 0;
    uint32 RefCount = 0;


    void set_parent(FileNode* _parent);//只改变树结构而不改变实际存储结构
    uint64 get_path_len(uint8 _flag);//为1时为vfs路径长度，为0时为全局路径长度
    char* get_path_copy(char* dst, uint8 _flag);

public:
    virtual int64 read(void* dst, uint64 pos, uint64 size);
    virtual int64 write(void* src, uint64 pos, uint64 size);
    virtual int64 size();
    virtual bool ref(FileHandle* f);
    virtual bool unref(FileHandle* f);
    virtual const char* get_name();
    virtual bool set_name(char* _name);
    virtual char* get_path(uint8 _flag); //_flag=0 绝对路径   1 vfs路径
    virtual bool IsDir();
    virtual bool del();


    FileNode(VFS* _VFS, uint64 _flags);
    virtual ~FileNode();
};

// class VFSM
// {
// private:
//     FileNode *root;
//     bool add_new_node(FileNode *p, FileNode *parent); // 只改变对应树的结构未改变对应存储方式
//     FileNode *find_child_name(FileNode *p, const char *name);//在子节点中查找对应名字的文件节点

// public:
//     bool is_absolutepath(char *path);
//     FileNode * create_directory(char *path);
//     FileNode * create_file(char *path); // 目前只支持VFS中的新建，即/VFS/*
//     bool move(char *src, char *dst);
//     bool copy(char *src, char *dst);
//     bool del(char *path); 
//     bool load_vfs(VFS* vfs,char * path);
//     // FileNode *find_vfs(char *path, char *&re);
//     // FileNode *find_vfs(char *path);
//     FileNode * get_node(char * path);


//     bool Init();
//     bool Destroy();
// };
// extern VFSM vfsm;

class VFS
{
private:
    FileNode* root;
public:
    virtual char* FileSystemName() {};
    VFS() {};
    virtual ~VFS() {};

    virtual FileNode* open(char* path) = 0;
    virtual bool close(FileNode* p) = 0;

    virtual FileNode* create_dir(const char* path) = 0;
    virtual FileNode* create_file(const char* path) = 0;
    virtual FileNode* get_node(const char* path) = 0;
    virtual bool del(const char* path) = 0;

};

#endif