#include <pathtool.hpp>
#include <kout.hpp>


DBCharPtr split_path_name(char *path, char *buf)
{

    
    if (path == nullptr)
    {
        return {nullptr, (char *)1};
    }

    if (path[0] != '/'||path[0]==0)
    {
        return {nullptr, nullptr};
    }


    char *t = path + 1;
    while (*t != '/' && *t != 0)
    {
        t++;
    }

    if (buf == nullptr)
    {
        buf = new char[t - path];
    }


    int i;
    for (i = 1; path[i] != '/' && path[i] != 0; i++)
    {
        buf[i-1]=path[i];
    }
    buf[i-1]=0;


    return {t,buf};
}