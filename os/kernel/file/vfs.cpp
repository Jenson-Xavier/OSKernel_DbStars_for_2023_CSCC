#include <vfs.hpp>

// VFSM vfsm;

// FileNode
int64 FileNode::read(void* dst, uint64 pos, uint64 size)
{
    return -1;
}

bool FileNode::del()
{
    return true;
}

int64 FileNode::write(void* src, uint64 pos, uint64 size)
{
    return -1;
}

int64 FileNode::size()
{
    return fileSize;
}

bool FileNode::ref(FileHandle* f)
{
    RefCount++;
    return true;
}

bool FileNode::unref(FileHandle* f)
{
    RefCount--;
    return true;
}

uint64 FileNode::get_path_len(uint8 _flag)
{
    if (_flag & (TYPE & __VFS))
        return 0;
    else if (TYPE & __ROOT)
        return 0;
    else if (parent == nullptr)
        return 0;
    return parent->get_path_len(_flag) + 1 + strlen(name);
}

char* FileNode::get_path_copy(char* dst, uint8 _flag)
{
    if (_flag & (TYPE & __VFS))
        return dst;
    else if (TYPE & __ROOT)
        return dst;
    else if (parent == nullptr)
        return dst;
    else
    {
        char* s = parent->get_path_copy(dst, _flag);
        *s = '/';
        return strcpy_s(s + 1, name);
    }
}

void FileNode::set_parent(FileNode* _parent)
{
    if (_parent == nullptr)
    {
        kout[red] << "FileNode ptr is null" << endl;
        return;
    }

    if (_parent->IsDir())
    {
        kout[red] << "FileNode isn't a Dir" << endl;
        return;
    }

    if (parent != nullptr)
    {
        if (parent->child == this)
            parent->child = next;
        else if (pre != nullptr)
            pre->next = next;
        if (next != nullptr)
            next->pre = pre;
        pre = next = parent = nullptr;
    }

    parent = _parent;
    next = _parent->child;
    _parent->child = this;
    if (next)
        next->pre = this;
}

bool FileNode::set_name(char* _name)
{
    if (name != nullptr)
        delete name;

    int64 len = strlen(_name);
    name = new char[len + 1];
    strcpy(name, _name);
    name[len] = 0;
    return true;
}

char* FileNode::get_path(uint8 _flag)
{
    uint64 len = get_path_len(_flag);
    if (len == 0)
    {
        char* re = (char*)pmm.malloc(2 * sizeof(char));
        strcpy(re, "/");
        return re;
    }
    else
    {
        char* re = (char*)pmm.malloc(len * sizeof(char) + 1);
        get_path_copy(re, _flag);
        re[len] = 0;
        return re;
    }
}

const char* FileNode::get_name()
{
    return name;
}

bool FileNode::IsDir()
{
    return TYPE & __DIR;
}

FileNode::FileNode(VFS* _VFS, uint64 _type)
{
    vfs = _VFS;
    TYPE = _type;
}

FileNode::~FileNode()
{
    while (child)
        delete child;
    if (name)
        delete name;
}

// VFSM

// bool VFSM::add_new_node(FileNode *p, FileNode *parent)
// {
//     if (parent == nullptr)
//     {
//         return false;
//     }

//     p->set_parent(parent);
//     return true;
// }

// bool VFSM::is_absolutepath(char *path)
// {
//     if (path[0] != '/')
//     {
//         return false;
//     }

//     while (*path)
//     {
//         if (!isInStr(*path, INVALIDPATHCHAR()))
//             return false;

//         path++;
//     }
//     return true;
// }

// // FileNode * VFSM::get_node(char * path)
// // {
// //     FileNode * t;
// //     t=find_vfs(path,path);
// //     return t->vfs->get_node(path);
// // }
// FileNode *VFSM::create_directory(char *path)
// {
//     FileNode *t;
//     t = find_vfs(path, path);
//     return t->vfs->create_dir(path);
// }

// FileNode *VFSM::create_file(char *path)
// {
//     FileNode *t;
//     t = find_vfs(path, path);
//     return t->vfs->create_file(path);
// }
// bool VFSM::move(char *src, char *dst)
// {
//     //...
//     return true;
// }

// bool VFSM::copy(char *src, char *dst)
// {
//     //...
//     return true;
// }

// bool VFSM::del(char *path)
// {

//     FileNode *t;
//     t = get_node(path);
//     if (t==root)
//     {
//         return false;
//     }

//     t->del();

//     return true;
// }

// bool VFSM::load_vfs(VFS *vfs, char *path)
// {
//     FileNode *t;
//     t = get_node(path);
//     if (t != nullptr)
//     {
//         t->vfs = vfs;
//         return true;
//     }
//     return false;
// }

// FileNode *VFSM::find_child_name(FileNode *p, const char *name)
// {
//     FileNode *t = p->child;
//     while (t)
//     {
//         if (strcmp(t->get_name(), name) == 0)
//             return t;
//         t = t->next;
//     }

//     return nullptr;
// }

// FileNode *VFSM::get_node(char *path)
// {
//     if (path[0] != '/')
//     {
//         kout[red] << "path error" << endl;
//         return nullptr;
//     }
//     char *t, *buf;
//     FileNode *now = root;
//     buf = new char[50];
//     while (path = split_path_name(path, buf)[0])
//     {
//         now = find_child_name(now, buf);
//     }
//     delete[] buf;
//     return now;
// }
// FileNode *VFSM::find_vfs(char * path,char *&re)
// {
//     if (path[0] != '/')
//     {
//         kout[red] << "path error" << endl;
//         return nullptr;
//     }
//     char *t, *buf;
//     FileNode *now = root;
//     buf = new char[50];
//     while (path = split_path_name(path, buf)[0])
//     {
//         now=find_child_name(now, buf);
//         if (now->TYPE&FileNode::__VFS)
//         {
//             re=path;
//             delete[] buf;
//             return now;
//         }
//     }
//     delete[] buf;
//     re=path;
//     return nullptr;
// }

// bool VFSM::Init()
// {
//     root = new FileNode(nullptr, FileNode::__DIR | FileNode::__ROOT);
//     FileNode *p = new FileNode(nullptr, FileNode::__DIR);
//     p->set_name((char *)"VFS");
//     add_new_node(root, p);
//     return true;
// }

// bool VFSM::Destroy()
// {
//     return false;
// }

// bool VFSM::load_vfs(VFS *vfs, char *path)
// {
//     FileNode *t;
//     t = get_node(path);
//     if (t != nullptr)
//     {
//         t->vfs = vfs;
//         return true;
//     }
//     return false;
// }