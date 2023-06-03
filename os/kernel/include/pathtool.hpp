#ifndef __PATHTOOL_HPP__
#define __PATHTOOL_HPP__

struct DBCharPtr
{
    char* p1;
    char* p2;
    char*& operator[](int index)
    {
        return index == 0 ? p1 : p2;
    }
};

// 如果自己不自己申请空间则要释放第二个指针,当两个返回指针都为空时，说明路径分割完毕或路径不合法，当只有第一为空时，说明传入指针为null
char* split_path_name(char* path, char* buf = nullptr);

// 将名称改为ascii
char* unicode_to_ascii(char* str);

char* unicode_to_ascii(char* str, char* buf);

char* pathcmp(char* pathfile, char* pathVMS); // 当文件名路径包含（包括相等时）返回不重合部分的指针,否则返回空指针


bool toshortname(const char* fileName, char* shortName);//将文件名转为短文件名形式

char* unicodecopy(char* dst, char* src, int len);
#endif