#ifndef __FILEOBJECT_HPP__
#define __FILEOBJECT_HPP__

#include <process.hpp>
#include <FAT32.hpp>

// 文件和进程之间的接口其实是通过文件描述符去对接的
// 因此在进程结构体中需要维护打开的文件描述符表这样的结构
// 即每个进程其实都有一个独立的文件描述符表
// 这里每个表项指向fileobject这样的结构 参考了Linux设计
// 每个进程打开的文件或多或少 所以使用链表的结构去设计
// 具体的fd是一个小的非负整数 分配就是找到第一个未被分配的非负整数

// 每个进程在创建时会某人打开三个文件描述符
// 即标准输入(0) 标准输出(1) 标准错误(2)
// 宏定义它们的fd
#define STDIN_FILENO    0
#define STDOUT_FILENO   1
#define STDERR_FILENO   2

// 标准输出对应的文件实例对象
// 暂时设为空 后续找到再设置
// 应该为串口文件格式 目前先如此保留
extern FAT32FILE* STDIO;

// 枚举结构列出flags信息
// 以Linux规范为标准
enum file_flags
{
    O_RDONLY = 0,               // 以只读方式打开文件
    O_WRONLY = 1,               // 以只写方式打开文件
    O_RDWR = 2,                 // 以读写方式打开文件
    O_CREAT = 64,               // 如果文件不存在 则创建文件
    O_EXCL = 128,               // 如果使用O_CREAT标记创建文件而文件已存在 则返回错误
    O_NOCTTY = 256,             // 串口方式
    O_TRUNC = 512,              // 如果文件存在 打开时则将其长度截断为0
    O_APPEND = 1024,            // 以追加方式打开文件
    O_NONBLOCK = 2048,          // 以非阻塞方式打开文件
    O_DSYNC = 4096,             // 仅针对数据 以同步I/O方式打开文件
    O_SYNC = 8192,              // 以同步I/O方式打开文件
    O_DIRECTORY = 65536,        // 打开的文件不是目录 则返回错误
    O_NOFOLLOW = 131072,        // 不跟随符号链接
    O_CLOEXEC = 524288,         // 进程执行exec系统调用时关闭此打开的文件描述符
};

// 枚举结构列出mode信息
// 以Linux规范为标准
enum file_mode
{
    S_IRWXU = 00700,            // 权限掩码确定拥有者的读 写和执行权限
    S_IRUSR = 00400,            // 拥有者可读
    S_IWUSR = 00200,            // 拥有者可写
    S_IXUSR = 00100,            // 拥有者可执行 如果是目录 则可搜索
    S_IRWXG = 00070,            // 权限掩码确定组用户的读 写和执行权限
    S_IRGRP = 00040,            // 组用户可读
    S_IWGRP = 00020,            // 组用户可写
    S_IXGRP = 00010,            // 组用户可执行 如果是目录 则可搜索
    S_IRWXO = 00007,            // 权限掩码确定其他用户的读 写和执行权限
    S_IROTH = 00004,            // 其他用户可读
    S_IWOTH = 00002,            // 其他用户可写
    S_IXOTH = 00001,            // 其他用户可执行 如果是目录 则可搜索
};

// seek操作基于的文件指针位置的常量
// 枚举表示
enum file_ptr
{
    Seek_beg = 0,               // 文件指针相对于文件头
    Seek_cur,                   // 相对于当前位置
    Seek_end,                   // 相对于文件尾
};

// 本内核设计的FileObject结构
// 需要明确的一点是这个结构在具体场景中一定是附属于某个具体的进程而存在和使用的
struct file_object
{
    int fd;                     // 小的非负整数表示文件描述符fd 这里使用int 考虑-1的可用性
    FAT32FILE* file;            // 对应的具体的打开的文件的结构指针
    uint64 pos_k;               // 进程对于每个打开的文件维护文件指针的当前位置信息 用于实现seek等操作
    uint64 flags;               // 进程对于每个打开的文件有一个如何访问这个文件的标志位信息 也可以是多位掩码的或
    uint64 mode;                // 表示进程对打开的文件具有的权限信息位
    file_object* next;          // 链表结构的next指针
};

// file_object实现依赖的一些函数的封装与管理
// 主要还是和具体的文件读写操作相关的功能
// 这个类中的函数基本都是以这个file_object为操作对象
// 涉及链表节点分配之类的操作对象基本均为进程结构体中的虚拟头节点
class FileObjectManager
{
public:
    // 链表结构相关
    void init_proc_fo_head(proc_struct* proc);                  // 专门生成并初始化进程的fo头节点
    int get_count_fdt(file_object* fo_head);                    // 统计进程的文件描述符表的表项个数 即打开了多个文件描述符
    int find_sui_fd(file_object* fo_head);                      // 给定具体进程的虚拟头节点 找到一个合适的自动分配的fd号
    file_object* create_flobj(file_object* fo_head,             // 根据指定的fd生成一个结构体 并且插入到进程的fd表之后 -1表示自动分配一个最小的fd
        int fd = -1);                                           // 并返回创建的一个新的fo指针对象            
    file_object* get_from_fd(file_object* fo_head, int fd);     // 从具体进程的虚拟头节点返回指定fd的文件对象
    int add_fo_tolist(file_object* fo_head,                     // 将指定的fo插入fd表的末尾
        file_object* fo_added);
    void free_all_flobj(file_object* fo_head);                  // 删除一个进程所有的fd 即释放整个fd表的资源
    void delete_flobj(file_object* fo_head, file_object* del);  // 释放某个具体fd表项的资源 保证链表的连贯性
    
    // 设置属性相关
    bool set_fo_fd(file_object* fo, int fd);                    // 设置进程fo的fd 修改用 使用几率不大
    bool set_fo_file(file_object* fo, FAT32FILE* file);         // 设置fo的file指针
    bool set_fo_pos_k(file_object* fo, uint64 pos_k);           // 设置fo的当前文件指针的位置
    bool set_fo_flags(file_object* fo, uint64 flags);           // 设置fo的flags标志位信息
    bool set_fo_mode(file_object* fo, uint64 mode);             // 设置fo的mode权限位信息

    // 文件处理相关
    int64 read_fo(file_object* fo, void* dst, uint64 size);     // 在fo中从当前文件指针的位置读size大小内容到dst
    int64 write_fo(file_object* fo, void* src, uint64 size);    // 在fo中写size大小的内容到文件的当前文件指针位置中
    void seek_fo(file_object* fo, int64 bias,                   // 移动文件指针的位置 默认相对于文件头进行移动
        int32 base = Seek_beg);
    bool close_fo(proc_struct* proc, file_object* fo);          // 关闭文件描述符 释放fd表项资源
    file_object* duplicate_fo(file_object* fo);                 // 从当前的fd复制一个新的fd表项
};

// 声明全局管理器进行调用
extern FileObjectManager fom;

#endif