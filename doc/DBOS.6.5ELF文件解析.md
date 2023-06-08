ELF文件解析在初赛中主要是为了对于从提供的文件系统的镜像中读取要测评的ELF文件，并且能够把这个程序正确装载到进程然后运行，并且能够在内核中进行调度。<br />至于ELF文件格式的解析就是参考标准的ELF文件格式依次进行解析接口，在初赛中需要解析的段重点其实就是段类型为`PT_LOAD`（即可装载）的段，那么按照规范格式依次解析即可，这一块并没有什么逻辑需要特别说明的，详情可以参考具体代码处，如下是进行ELF文件解析所需要涉及的结构体等相关信息。
```cpp
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
```
如上这个是ELF文件的头解析需要的结构体，为了能够从文件中正确读取信息到结构体中，我们可以加上取消对齐的编译指令提示，相关属性成员的具体含义一旁注释均有详细的说明。
```cpp
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
```
如上是程序头的结构体信息，也就是段的信息，我们目前重点只需要解析相关可装载的段即可。
```cpp
struct procdata_fromELF
{
    file_object* fo;
    proc_struct* proc;
    VMS* vms;
    Elf_Ehdr e_header;
    SEMAPHORE sem;                          // 信号量保证启动函数能够完整执行完再释放相关空间
};
```
最后这个结构体是进程来进行ELF文件格式解析从而进行启动和创建所需要的结构体。然后这里简单介绍一下从ELF文件创建进程的函数逻辑。这个函数传入的参数主要是前面提到的`file_object`结构体指针，这里存放了要读取和处理的相关文件（与文件系统相关），然后工作目录的字符指针参数，以及要创建的进程的`flags`参数。<br />首先是按照上面这个`procdata_fromELF`结构体分配一个对应内存大小的结构体指针空间，然后对相关的成员属性进行初始化和赋值过程；然后就是通过提供的文件`read`接口去读取对应文件的ELF头进行解析；然后就是创建进程所需要执行的一些逻辑，分配进程空间，初始化，设置内核栈、虚拟内存空间、父亲进程、当前工作目录；接下来就是填充`userdata`结构体信息并且作为参数传入`ProcessManager`类的从指定函数启动进程的成员函数中启动这个进程即可，顺利执行的话就会成功在内核的进程空间创建对应的进程，并且能够正常进行调度和执行；最后别忘了释放分配的相关资源的空间。<br />By：谢骏鑫
