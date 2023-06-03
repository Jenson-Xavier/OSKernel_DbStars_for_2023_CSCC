#include <pathtool.hpp>
#include <kout.hpp>

char* split_path_name(char* path, char* buf)
{

    if (path == nullptr)
    {
        return nullptr;
    }

    if (path[0] != '/' || path[0] == 0)
    {
        return nullptr;
    }

    char* t = path + 1;
    while (*t != '/' && *t != 0)
    {
        t++;
    }

    int i;
    for (i = 1; path[i] != '/' && path[i] != 0; i++)
    {
        buf[i - 1] = path[i];
    }
    buf[i - 1] = 0;

    return t;
}

char* unicode_to_ascii(char* str)
{
    char* t = str;
    while (*t)
    {
        *str++ = *t;
        t += 2;
    }
    *str = *t;

    return str;
}

char* unicode_to_ascii(char* str, char* buf)
{
    char* t = str;
    int i = 0;
    while (*t)
    {
        buf[i++] = *t;
        t += 2;
    }
    buf[i] = *t;
    return buf;
}

char* pathcmp(char* pathfile, char* pathVMS)
{
    while (*pathVMS != '\0')
    {
        if (*pathfile != *pathVMS)
            return nullptr;
        pathfile++;
        pathVMS++;
    }

    return pathfile;
}

bool toshortname(const char* fileName, char* shortName)
{
    uint64 re = 0;
    uint64 t;
    while (re < 8 && fileName[re] != '\0' && fileName[re] != '.')
    {
        shortName[re] = fileName[re];
        re++;
    }

    t = re;

    if (re == 8 && fileName[re] != '.' && fileName[re] != '\0')
    {
        shortName[6] = '~';
        shortName[7] = '1';
    }

    while (re < 8)
    {
        shortName[re] = ' ';
        re++;
    }

    while (fileName[t] != '.' && fileName[t] != '\0')
    {
        t++;
    }
    if (fileName[t] == '.')
    {
        shortName[8] = fileName[t + 1] != '\0' ? fileName[t + 1] : ' ';
        shortName[9] = fileName[t + 2] != '\0' ? fileName[t + 2] : ' ';
        shortName[10] = fileName[t + 3] != '\0' ? fileName[t + 3] : ' ';
    }

    return true;
}

char* unicodecopy(char* dst, char* src, int len)
{
    int re = 0;
    kout[purple] << src << endl;
    while (re < len && src[re] != 0)
    {
        dst[2 * re] = src[re];
        dst[2 * re + 1] = 0;
        re++;
    }
    if (re < len)
    {
        dst[2 * re] = 0;
        dst[2 * re + 1] = 0;
    }


    return &src[re];
}