#ifndef __KSTRING_HPP__
#define __KSTRING_HPP__

// 一开始并未考虑实现
// 在实现进程过程中发现有必要 暂时简单实现几个常用的经典cstring函数 为自己的kstring

#include <type.hpp>

uint64 strlen(const char* s);

char* strcpy(char* dst, const char* src);

int strcmp(const char* s1, const char* s2);

void* memset(void* s, char ch, uint64 size);

void* memcpy(void* dst, const char* src, uint64 size);

#endif