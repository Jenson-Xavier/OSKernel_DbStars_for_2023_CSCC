#ifndef __PARSEELF_HPP__
#define __PARSEELF_HPP__

// 为了实现运行一个程序的系统调用
// 需要解析ELF可执行文件来装载进程

#include <process.hpp>
#include <FAT32.hpp>
#include <fileobject.hpp>
#include <semaphore.hpp>
#include <vfsm.hpp>

// ELF文件格式有32位版本和64版本
// 两个版本的解析并没有大的区别
// 可以使用模板实现 考虑到大赛是基于64位的
// 这里仅解析64位ELF的即可
// 就不在名字上标注32和64加以区分了

// ELF文件格式首先需要解析的就是文件头结构
// ELF header
#define EI_NIDENT 16

struct Elf_Ehdr
{
    union
    {
        // e_ident是ELF头最开始的16个字节
        // 含有ELF文件的识别标志
        // 是一个按字节连续排列的数组
        // 每个字节位置的内容是固定 可以使用union结构来定义
        uint8 e_ident[EI_NIDENT];
        struct
        {
            uint8 ei_magic[4];              // 表示ELF文件格式 魔数
            uint8 ei_class;                 // 文件的类型或容量
            uint8 ei_data;                  // 文件的数据编码格式
            uint8 ei_version;               // 文件头的版本
            uint8 ei_pad[9];                // 扩展位
        };
    };
    uint16 e_type;                          // 目标文件属于哪种类型
    uint16 e_machine;                       // 使用的处理器结构
    uint32 e_version;                       // 目标文件的版本
    uint64 e_entry;                         // 程序入口的虚拟地址 对于可执行文件 ELF文件解析并完成加载之后 程序将在这里开始运行
    uint64 e_phoff;                         // 程序头表在文件中的偏移量
    uint64 e_shoff;                         // 节头表在文件中的偏移量
    uint32 e_flags;                         // 处理器特定的标志位
    uint16 e_ehsize;                        // ELF文件头的大小 以字节为单位
    uint16 e_phentsize;                     // 程序头表中每一个表项的大小
    uint16 e_phnum;                         // 程序头表中所有表项的数目
    uint16 e_shentsize;                     // 节头表中每一个表项的大小
    uint16 e_shnum;                         // 节头表中所有表项的数目
    uint16 e_shstrndx;                      // 节头表中与节名字表相对应的表项的索引

    // 根据ELF头中的e_ident成员中的魔数判断是否是ELF文件
    // 在结构体加入一个内联函数方便判断
    inline bool is_ELF()
    {
        if (ei_magic[0] == 0x7F && ei_magic[1] == 'E' && ei_magic[2] == 'L' && ei_magic[3] == 'F')
        {
            return true;
        }
        return false;
    }

}__attribute__((packed));                   // 提示编译器取消对齐优化

// 我们解析ELF文件在当前场景下
// 主要是为了解析运行可执行文件
// 因此需要解析的部分主要是程序头表的信息 即段表
// 进行的ELF文件的装载 进而启动进程去运行这个进程
// 程序头结构
// program header
struct Elf_Phdr
{
    uint32 p_type;                          // 本程序头描述的段的类型
    uint64 p_offset;                        // 本段内容在文件中的位置 段内容的开始位置相对于文件头的偏移量
    uint64 p_vaddr;                         // 本段内容的开始位置在进程空间中的虚拟地址
    uint64 p_paddr;                         // 本段内容的开始位置在进程空间中的物理地址
    uint64 p_filesz;                        // 本段内容在文件中的大小 以字节为单位
    uint64 p_memsz;                         // 本段内容在内容镜像中的大小
    uint32 p_flags;                         // 本段内容的属性
    uint64 p_align;                         // 对齐方式 可装载段来讲应该要向内存页面对齐
}__attribute__((packed));

// 段类型的枚举表示
enum P_type
{
    PT_NULL = 0,                            // 本程序头是未使用的
    PT_LOAD = 1,                            // 本程序头指向一个可装载的段
    PT_DYNAMIC = 2,                         // 本段指明了动态链接的信息
    PT_INTERP = 3,                          // 本段指向一个字符串 是一个ELF解析器的路径
    PT_NOTE = 4,                            // 本段指向一个字符串 包含一些附加的信息
    PT_SHLIB = 5,                           // 本段类型是保留的 未定义语法
    PT_PHDR = 6,                            // 自身所在的程序头表在文件或内存中的位置和大小
    PT_LOPROC = 0x70000000,                 // 为特定处理器保留使用的区间
    PT_HIPROC = 0x7fffffff,
};

// 段属性或者段权限的枚举表示
enum P_flags
{
    PF_X = 0x1,                             // 可执行
    PF_W = 0x2,                             // 只写
    PF_R = 0x4,                             // 只读
    PF_MASKPROC = 0xf0000000,               // 未指定
};

// 从ELF文件解析得到用来创建进程的数据
struct procdata_fromELF
{
    file_object* fo;
    proc_struct* proc;
    VMS* vms;
    Elf_Ehdr e_header;
    SEMAPHORE sem;                          // 信号量保证启动函数能够完整执行完再释放相关空间
};

// 从ELF文件启动进程函数
int start_process_formELF(void* userdata);

// 最终的从ELF文件创建进程的函数
proc_struct* CreateProcessFromELF(file_object* fo, const char* wk_dir, int proc_flags = 0);

#endif