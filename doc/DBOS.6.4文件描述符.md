进程的管理离不开文件的交互，经典的操作系统管理中进程会为打开的文件维护一个文件描述符，我们同样参考了这样的设计，并且在进程和文件系统的交互之间增加了一层称为`file_object`的结构体的封装，我个人认为（正如代码中注释所写得那样），这一层封装和抽象的设计是极巧妙的，非常巧妙完美地解决了进程直接管理文件而产生的复杂的资源管理的问题。<br />这个结构体的定义如下，同样也是结构体类型，具体成员的含义注释均有详细说明。
```cpp
struct file_object
{
    int fd;                     // 小的非负整数表示文件描述符fd 这里使用int 考虑-1的可用性
    FAT32FILE* file;            // 对应的具体的打开的文件的结构指针
    uint64 pos_k;               // 进程对于每个打开的文件维护文件指针的当前位置信息 用于实现seek等操作
    uint64 flags;               // 进程对于每个打开的文件有一个如何访问这个文件的标志位信息 也可以是多位掩码的或
    uint64 mode;                // 表示进程对打开的文件具有的权限信息位
    file_object* next;          // 链表结构的next指针
};
```
	这个结构体中有个`next`指针，显然它是用来维护链表结构的，而这一整个链表，其实就是我做的进程对文件描述符表的抽象，进程每打开一个文件，都会在它的这个链表中新增一个节点，进程要读写这个文件，就是通过对应的文件描述符找到这个结构体节点，并且根据其对应的`file`成员属性进入文件系统去访问对应的文件系统的接口来操作具体的文件。当进程关闭一个文件时，仅仅只需要将对应的结构体节点从链表中删除即可，并且会在删除对应节点操作中自动处理打开的具体文件指针的引用计数问题，然后文件系统中也会自动处理对于打开的文件的资源管理问题，从而通过这个接口，进程很方便地实现了资源的管理，也很好地实现了进程管理部分和文件系统的模块分离和接口交互。<br />对于这个结构体，类似进程那样，我们同样设计一个管理器来提供对这个结构体的管理和函数的接口，具体类的接口声明如下，具体每个接口的功能的解释在一旁的注释中都有清楚的说明。
```cpp
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
    bool seek_fo(file_object* fo, int64 bias,                   // 移动文件指针的位置 默认相对于文件头进行移动
        int32 base = Seek_beg);
    bool close_fo(file_object* fo);                             // 关闭文件描述符 释放fd表项资源
    file_object* duplicate_fo(file_object* fo);                 // 从当前的fd复制一个新的fd表项
};
```
By：谢骏鑫
