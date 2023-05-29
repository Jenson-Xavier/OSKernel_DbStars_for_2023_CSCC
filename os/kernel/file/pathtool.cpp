#include <pathtool.hpp>
#include <kout.hpp>


char * split_path_name(char *path, char *buf)
{

    
    if (path == nullptr)
    {
        return nullptr;
    }

    if (path[0] != '/'||path[0]==0)
    {
        return nullptr;
    }


    char *t = path + 1;
    while (*t != '/' && *t != 0)
    {
        t++;
    }


    int i;
    for (i = 1; path[i] != '/' && path[i] != 0; i++)
    {
        buf[i-1]=path[i];
    }
    buf[i-1]=0;


    return t;
}

char * unicode_to_ascii(char * str)
{
    char * t=str;
    while (*t)
    {
        *str++=*t;
        t+=2;
    }
    *str=*t;

    return str; 
}

char * unicode_to_ascii(char * str,char * buf)
{
    char * t=str;
    int i=0;
    while (*t)
    {
        buf[i++]=*t;
        t+=2;
    }
    buf[i]=*t;
    return buf;
    
}