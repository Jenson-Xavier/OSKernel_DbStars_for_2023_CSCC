#ifndef __RESOURCES_HPP__
#define __RESOURCES_HPP__

// 测试用户进程
// 在没有文件系统的条件下使用用户映像嵌入内核的方式
// 主要是利用链接器ld的参数实现嵌入
// 详见文档部分"内核里嵌入用户程序"
// 处理汇编宏实现地址的获取

#define resource_declare(name)                          \
    extern "C"                                          \
    {                                                   \
        extern char _binary_Build_##name##_size[];      \
        extern char _binary_Build_##name##_start[];     \
        extern char _binary_Build_##name##_end[];       \
    };

resource_declare(test_img);
resource_declare(test2_img);

#define get_resource_begin(name)    (_binary_Build_##name##_start)
#define get_resource_end(name)      (_binary_Build_##name##_end)
#define get_resource_size(name)     (_binary_Build_##name##_size)

#endif