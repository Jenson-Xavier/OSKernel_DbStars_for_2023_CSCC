#include <vfsm.hpp>

bool VFSM::init()
{
    FAT32* fat = new FAT32;
    FATtable a;
    a.size = 0;

    OpenedFile = new FAT32FILE(a, ".ROOT");
    OpenedFile->fat = fat; // 挂载root

    OpenedFile->TYPE = 0;
    OpenedFile->TYPE |= (FAT32FILE::__DIR | FAT32FILE::__VFS | FAT32FILE::__ROOT);
    OpenedFile->path = new char[2];
    OpenedFile->path[0] = '^'; // 与其他所有路径名都不匹配；

    a.high_clus = fat->Dbr.root_clus >> 4;
    a.low_clus = fat->Dbr.root_clus;
    OpenedFile->clus = fat->Dbr.root_clus;

    OpenedFile->next = nullptr;
    OpenedFile->pre = nullptr;
    return true;
}

FAT32FILE* VFSM::get_root()
{
    return OpenedFile;
}

char* VFSM::unified_path(const char* path, char* cwd)
{
    char* re = new char[200];
    re[0] = 0;
    if (path[0] != '/')
    {
        strcpy(re, cwd);
        if (strcmp(cwd, "/") != 0)
        {
            int str_len = strlen(cwd);
            if (cwd[str_len - 1] != '/')
            {
                strcat(re, "/");
            }
        }
    }
    strcat(re, path);
    if (re[strlen(re) - 1] == '/' && strlen(re) != 1)
    {
        re[strlen(re) - 1] == 0;
    }
    return re;
}

bool VFSM::destory()
{
}

FAT32FILE* VFSM::find_file_by_path(char* path, bool& isOpened)
{

    if (path[0] == '/' && path[1] == 0)
    {
        return get_root();
    }

    isOpened = false;
    FAT32FILE* t;
    t = OpenedFile;
    char* cpath = path;
    while (t != nullptr)
    {
        if (cpath = pathcmp(path, t->path))
        {
            // kout[red]<<rpath<<endl;
            if (*cpath == '\0')
            {
                // kout << "??" << endl;
                isOpened = true;
                return t;
            }
            else if (t->TYPE & (FAT32FILE::__DIR) && (t->TYPE & FAT32FILE::__VFS))
            {
                return t->fat->find_file_by_path(cpath);
            }
        }

        t = t->next;
    }
    // 当链表中均不存在该文件时，从根目录下开始搜索

    t = OpenedFile->fat->find_file_by_path(path);
    if (t)
        t->path = path;
    else
        // kout[red] << "can't find file" << endl;
    // kout[red]<<"Sda"<<endl;
    // t->show();
    // kout.memory(&t->table,32);

    return t;
}

FAT32FILE* VFSM::open(const char* path, char* cwd)
{
    bool isOpened;
    char* rpath = unified_path(path, cwd);
    FAT32FILE* t;
    // kout[yellow] << rpath << endl;
    t = find_file_by_path(rpath, isOpened);
    if (t)
    {
        t->ref++;
    }
    // kout[blue]<<"dsa"<<endl;
    // t->show();

    if (isOpened)
        return t;

    t->next = OpenedFile->next;
    OpenedFile->next = t;
    if (t->next)
        t->next->pre = t;

    t->ref++;
    return t;
}
void VFSM::close(FAT32FILE* t)
{
    t->ref--;
    if (t->ref == 0)
    {
        t->pre->next = t->next;
        if (t->next)
            t->next->pre = t->pre;
        delete t;
    }
}

void VFSM::show_opened_file()
{
    FAT32FILE* t = OpenedFile;
    kout << "__________________" << endl;
    while (t)
    {
        t->show();
        t = t->next;
    }
    kout << "__________________" << endl;
}

bool VFSM::create_file(const char* path, char* cwd, char* fileName, uint8 type)
{
    uint64 len = strlen(path);
    bool isOpened;
    // kout[yellow]<<"yes"<<endl;
    char* rpath = unified_path(path, cwd);
    // kout[yellow] << rpath << endl;
    FAT32FILE* t, * re;
    t = find_file_by_path(rpath, isOpened);
    if (!t)
    {
        kout[red] << "can't find dir" << endl;
        return false;
    }
    // t->show();
    re = t->fat->create_file(t, fileName, type);
    if (!re)
    {
        kout[red] << "failed create file" << endl;
        return false;
    }

    rpath[len + 1] = 0;
    rpath[len] = '/';
    strcat(rpath, fileName);
    re->path = rpath;

    // re->show();
    return true;
}


bool VFSM::create_dir(const char* path, char* cwd, char* dirName)
{
    bool isOpened;
    // kout[yellow]<<"yes"<<endl;
    char* rpath = unified_path(path, cwd);
    FAT32FILE* t, * re;
    t = find_file_by_path(rpath, isOpened);
    if (!t)
    {
        kout[red] << "can't find dir" << endl;
        return false;
    }

    // t->show();
    re = t->fat->create_file(t, dirName);
    if (!re)
    {
        kout[red] << "failed create file" << endl;
        return false;
    }

    re->path = rpath;
    // re->show();

    return true;
}


bool VFSM::del_file(const char* path, char* cwd)
{
    bool isOpened;
    // kout[yellow]<<"yes"<<endl;
    char* rpath = unified_path(path, cwd);
    FAT32FILE* t, * re;
    t = find_file_by_path(rpath, isOpened);
    // t->show();
    if (isOpened)
    {
        kout[red] << "file isn't close" << endl;
        return false;
    }

    return t->fat->del_file(t);
}
FAT32FILE* VFSM::get_next_file(FAT32FILE* dir, FAT32FILE* cur, bool (*p)(FATtable* temp))
{
    FAT32FILE* t;
    t = dir->fat->get_next_file(dir, cur, p);
    t->path = new char[200];
    if (dir != get_root())
    {
        strcpy(t->path, dir->path);
        strcat(t->path, "/");
    }
    else
        strcpy(t->path, "/");

    strcat(t->path, t->name);
    t->fat = dir->fat;
    return t;
}
FAT32FILE* VFSM::open(FAT32FILE* file)
{
    file->ref++;
    file->next = OpenedFile->next;
    OpenedFile->next = file;
    if (file->next)
        file->next->pre = file;
    return file;
}
bool VFSM::link(const char* srcpath, const char* ref_path, char* cwd)
{
    int k=strlen(ref_path);
    while (ref_path[--k]!='/');
    char * file_name=new char[200];
    char * file_path=new char[200];
    strcpy(file_name,&ref_path[k+1]);
    file_path[k]=0;
    FAT32FILE * from,*to;
    create_file(file_path,cwd,file_name);
    from=open(ref_path,cwd);
    from->show();
    to=open(srcpath,cwd);
    to->show();

    from->table.low_clus=to->table.low_clus;
    from->table.high_clus=to->table.high_clus;
    from->table.size=to->table.size;
    from->clus=to->clus;
    from->fat=to->fat;

    delete [] file_name;
    delete [] file_path;
    close(from);
    close(to);
    return true;
}
bool VFSM::unlink(const char* path, char* cwd)
{
    bool isOpened;
    // kout[yellow]<<"yes"<<endl;
    char* rpath = unified_path(path, cwd);
    FAT32FILE* t, * re;
    t = find_file_by_path(rpath, isOpened);
    // t->show();
    if (isOpened)
    {
        kout[red] << "file isn't close" << endl;
        return false;
    }
    return t->fat->unlink(t);
}

VFSM vfsm;