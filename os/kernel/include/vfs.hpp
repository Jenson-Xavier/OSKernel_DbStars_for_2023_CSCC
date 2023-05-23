#ifndef __FVS_HPP__
#define __FVS_HPP__

#include <type.hpp>
#include <kstring.hpp>
#include <vmm.hpp>
#include <kout.hpp>
#include <pathtool.hpp>

inline const char *INVALIDPATHCHAR()
{
    return "/\\:*?\"<>|";
}

class VFS;
class VFSM;
class FileHandle;

class FileNode
{
    friend class VFSM;

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

    char *name = nullptr;
    VFS *vfs = nullptr;
    uint64 TYPE;
    FileNode *parent = nullptr,
             *pre = nullptr,
             *next = nullptr,
             *child = nullptr;

    uint64 fileSize = 0;
    uint32 RefCount = 0;

    void set_parent(FileNode *_parent);
    uint64 get_path_len(uint8 _flag);
    char *get_path_copy(char *dst, uint8 _flag);

public:
    virtual int64 read(void *dst, uint64 pos, uint64 size);
    virtual int64 write(void *src, uint64 pos, uint64 size);
    virtual int64 size();
    virtual bool ref(FileHandle *f);
    virtual bool unref(FileHandle *f);
    virtual const char *get_name();
    virtual bool set_name(char *_name);
    virtual char *get_path(uint8 _flag); //_flag=0 绝对路径   1 vfs路径
    virtual bool IsDir();

    FileNode(VFS *_VFS, uint64 _flags);
    ~FileNode();
};

class VFSM
{
private:
    FileNode *root;
    bool add_new_node(FileNode *p, FileNode *parent); // 只改变对应树的结构未改变对应存储方式
    FileNode *find_child_name(FileNode *p, const char *name);

public:
    bool is_absolutepath(char *path);
    bool create_directory(char *path);
    bool create_file(char *path); // 目前只支持VFS中的新建，即/VFS/*
    bool move(char *src, char *dst);
    bool copy(char *src, char *dst);
    bool del(char *path); // 目前只支持VFS中的删除，即/VFS/*
    // bool load_vfs(VFS* vfs,const char * path);
    FileNode *find_vfs(char *path, char *&re);
    FileNode *find_vfs(char *path);

    // FileNode * open();
    // FileNode * close();

    bool Init();
    bool Destroy();
};
extern VFSM vfsm;

class VFS
{
private:
public:
    virtual char *FileSystemName(){};
    VFS(){};
    ~VFS(){};

    virtual FileNode *open(char *path) = 0;
    virtual bool close(FileNode *p) = 0;

    virtual bool create_dir(const char *path)
    {
    }
    virtual bool create_file(const char *path)
    {
    }
    virtual bool del(const char *path)
    {
    }
};

#endif