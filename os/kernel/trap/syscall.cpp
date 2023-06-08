#include <type.hpp>
#include <klib.hpp>
#include <memory.hpp>
#include <process.hpp>
#include <syscall.hpp>
#include <trap.hpp>
#include <clock.hpp>
#include <interrupt.hpp>
#include <synchronize.hpp>
#include <FAT32.hpp>
#include <fileobject.hpp>
#include <parseELF.hpp>

// 实现系统调用函数
// 主要根据大赛要求依次进行实现

inline void SYS_putchar(char ch)
{
    putchar(ch);
}

inline char SYS_getchar()
{
    return getchar();
}

inline void SYS_exit(int ec)
{
    // 触发进程终止 无返回值
    // 输入参数ec为终止状态值

    proc_struct* cur_exited = pm.get_cur_proc();
    if (cur_exited->fa != nullptr)
    {
        // kout << "The proc has Father proc whose pid is " << cur_exited->fa->pid << endl;    
    }
    pm.exit_proc(cur_exited, ec);
    // 下面这条语句测试用 执行到这里说明有问题 上面已经保证进程被回收了
    kout[red] << "SYS_exit : reached unreachable branch!" << endl;
}

inline char* SYS_getcwd(char* buf, uint64 size)
{
    // 获取当前工作目录的系统调用
    // 传入一块字符指针缓冲区和大小
    // 成功返回当前工作目录的字符指针 失败返回nullptr

    if (buf == nullptr)
    {
        // buf为空时由系统分配缓冲区
        buf = (char*)kmalloc(size * sizeof(char));
    }
    else
    {
        proc_struct* cur_proc = pm.get_cur_proc();
        VMS::EnableAccessUser();
        const char* cwd = cur_proc->cur_work_dir;
        uint64 cwd_len = strlen(cwd);
        if (cwd_len > 0 && cwd_len < size)
        {
            strcpy(buf, cwd);
        }
        else
        {
            kout[yellow] << "SYS_getcwd the Buf Size is Not enough to Store the cwd!" << endl;
            return nullptr;
        }
        VMS::DisableAccessUser();
    }
    return buf;
}

inline int SYS_pipe2(int* fd, int flags)
{
    // 创建管道的系统调用
    // 保存两个文件描述符fd[2]
    // fd[0]为管道的读出端
    // fd[1]为管道的写入端
    // 成功返回0 失败返回-1

    // 暂时还没有实现 输出并返回-1
    return -1;
}

inline int SYS_dup(int fd)
{
    // 复制文件描述符的系统调用
    // 传入被复制的文件描述符
    // 成功返回新的文件描述符 失败返回-1

    proc_struct* cur_proc = pm.get_cur_proc();
    file_object* fo = fom.get_from_fd(cur_proc->fo_head, fd);
    if (fo == nullptr)
    {
        // 当前文件描述符不存在
        return -1;
    }
    file_object* fo_new = fom.duplicate_fo(fo);
    if (fo_new == nullptr)
    {
        return -1;
    }
    int ret_fd = -1;
    // 将复制的新的文件描述符直接插入当前的进程的文件描述符表
    ret_fd = fom.add_fo_tolist(cur_proc->fo_head, fo_new);
    return ret_fd;
}

inline int SYS_dup3(int old_fd, int new_fd)
{
    // 复制文件描述符同时指定新的文件描述符的系统调用
    // 成功执行返回新的文件描述符 失败返回-1

    if (old_fd == new_fd)
    {
        return new_fd;
    }

    proc_struct* cur_proc = pm.get_cur_proc();
    file_object* fo_old = fom.get_from_fd(cur_proc->fo_head, old_fd);
    if (fo_old == nullptr)
    {
        return -1;
    }
    file_object* fo_new = fom.duplicate_fo(fo_old);
    if (fo_new == nullptr)
    {
        return -1;
    }
    // 先查看指定的新的文件描述符是否已经存在
    file_object* fo_tmp = nullptr;
    fo_tmp = fom.get_from_fd(cur_proc->fo_head, new_fd);
    if (fo_tmp != nullptr)
    {
        // 指定的新的文件描述符已经存在
        // 将这个从文件描述符表中删除
        fom.delete_flobj(cur_proc->fo_head, fo_tmp);
    }

    // 没有串口 继续trick
    if (old_fd == STDOUT_FILENO)
    {
        fo_new->tk_fd = STDOUT_FILENO;
    }

    fom.set_fo_fd(fo_new, new_fd);
    // 再将这个新的fo插入进程的文件描述符表
    int rd = fom.add_fo_tolist(cur_proc->fo_head, fo_new);
    if (rd != new_fd)
    {
        return -1;
    }
    return rd;
}

inline int SYS_chdir(const char* path)
{
    // 切换工作目录的系统调用
    // 传入要切换的工作目录的参数
    // 成功返回0 失败返回-1

    if (path == nullptr)
    {
        return -1;
    }

    proc_struct* cur_proc = pm.get_cur_proc();
    VMS::EnableAccessUser();
    pm.set_proc_cwd(cur_proc, path);
    VMS::DisableAccessUser();
    return 0;
}

inline int SYS_openat(int fd, const char* filename, int flags, int mode)
{
    // 打开或创建一个文件的系统调用
    // fd为文件所在目录的文件描述符
    // filename为要打开或创建的文件名
    // 如果为绝对路径则忽略fd
    // 如果为相对路径 且fd是AT_FDCWD 则filename相对于当前工作目录
    // 如果为相对路径 且fd是一个文件描述符 则filename是相对于fd所指向的目录来说的
    // flags为访问模式 必须包含以下一种 O_RDONLY O_WRONLY O_RDWR
    // mode为文件的所有权描述
    // 成功返回新的文件描述符 失败返回-1

    VMS::EnableAccessUser();
    char* rela_wd = nullptr;
    proc_struct* cur_proc = pm.get_cur_proc();
    char* cwd = cur_proc->cur_work_dir;
    if (fd == AT_FDCWD)
    {
        rela_wd = cur_proc->cur_work_dir;
    }
    else
    {
        file_object* fo = fom.get_from_fd(cur_proc->fo_head, fd);
        if (fo != nullptr)
        {
            rela_wd = fo->file->path;
        }
    }

    if (flags & file_flags::O_CREAT)
    {
        // 创建文件或目录
        // 创建则在进程的工作目录进行
        if (flags & file_flags::O_DIRECTORY)
        {
            vfsm.create_dir(rela_wd, cwd, (char*)filename);
        }
        else
        {
            vfsm.create_file(rela_wd, cwd, (char*)filename);
        }
    }

    char* path = vfsm.unified_path(filename, rela_wd);
    if (path == nullptr)
    {
        return -1;
    }
    // trick
    // 暂时没有对于. 和 ..的路径名的处理
    // 特殊处理打开文件当前目录.的逻辑
    FAT32FILE* file = nullptr;
    if (filename[0] == '.' && filename[1] != '.')
    {
        int str_len = strlen(filename);
        char* str_spc = new char[str_len];
        strcpy(str_spc, filename + 1);
        file = vfsm.open(str_spc, rela_wd);
    }
    else
    {
        file = vfsm.open(filename, rela_wd);
    }

    if (file != nullptr)
    {
        if (!(file->TYPE & FAT32FILE::__DIR) && (flags & O_DIRECTORY))
        {
            file = nullptr;
        }
    }
    file_object* fo = fom.create_flobj(cur_proc->fo_head);
    if (fo == nullptr || fo->fd < 0)
    {
        return -1;
    }
    if (file != nullptr)
    {
        fom.set_fo_file(fo, file);
        fom.set_fo_flags(fo, flags);
        fom.set_fo_mode(fo, mode);
    }
    kfree(path);
    VMS::DisableAccessUser();
    return fo->fd;
}

inline int SYS_close(int fd)
{
    // 关闭一个文件描述符的系统调用
    // 传入参数为要关闭的文件描述符
    // 成功执行返回0 失败返回-1

    proc_struct* cur_proc = pm.get_cur_proc();
    file_object* fo = fom.get_from_fd(cur_proc->fo_head, fd);
    if (fo == nullptr)
    {
        return -1;
    }
    if (!fom.close_fo(cur_proc, fo))
    {
        // fom中的close_fo调用会关闭这个文件描述符
        // 主要是对相关文件的解引用并且从文件描述符表中删去这个节点
        return -1;
    }
    return 0;
}

struct dirent
{
    uint64 d_ino;	                    // 索引结点号
    int64 d_off;	                    // 到下一个dirent的偏移
    unsigned short d_reclen;	        // 当前dirent的长度
    unsigned char d_type;	            // 文件类型
    char d_name[];	                    // 文件名
}__attribute__((packed));

inline int SYS_getdents64(int fd, const void* buf, uint64 count)
{
    // 获取目录的条目的系统调用
    // fd是所要读取目录的文件描述符
    // buf是一个缓存区 用于保存所读取目录的信息
    // 成功返回读取的字节数 当到目录的结尾时返回0
    // 失败返回-1

    // 暂时还没有实现
    return -1;
}

inline int64 SYS_read(int fd, void* buf, uint64 count)
{
    // 从一个文件描述符中读取的系统调用
    // fd是要读取的文件描述符
    // buf是存放读取内容的缓冲区 count是要读取的字节数
    // 成功返回读取的字节数 0表示文件结束 失败返回-1

    if (buf == nullptr)
    {
        return -1;
    }

    proc_struct* cur_proc = pm.get_cur_proc();
    file_object* fo = fom.get_from_fd(cur_proc->fo_head, fd);
    if (fo == nullptr)
    {
        return -1;
    }
    VMS::EnableAccessUser();
    int64 rd_size = 0;
    rd_size = fom.read_fo(fo, buf, count);
    VMS::DisableAccessUser();
    if (rd_size < 0)
    {
        return -1;
    }
    return rd_size;
}

inline int64 SYS_write(int fd, void* buf, uint64 count)
{
    // 从一个文件描述符写入
    // 输入fd为一个文件描述符
    // buf为要写入的内容缓冲区 count为写入内容的大小字节数
    // 成功返回写入的字节数 失败返回-1

    if (buf == nullptr)
    {
        return -1;
    }

    if (fd == STDOUT_FILENO)
    {
        VMS::EnableAccessUser();
        for (int i = 0;i < count;i++)
        {
            putchar(((char*)buf)[i]);
            // kout << (uint64)((char*)buf)[i] << endl;
        }
        VMS::DisableAccessUser();
        return count;
    }

    proc_struct* cur_proc = pm.get_cur_proc();
    file_object* fo = fom.get_from_fd(cur_proc->fo_head, fd);
    if (fo == nullptr)
    {
        return -1;
    }

    // trick实现文件信息的打印
    // 向标准输出写
    if (fo->tk_fd == STDOUT_FILENO)
    {
        VMS::EnableAccessUser();
        for (int i = 0;i < count;i++)
        {
            putchar(*(char*)(buf + i));
        }
        VMS::DisableAccessUser();
        return count;
    }

    VMS::EnableAccessUser();
    int64 wr_size = 0;
    wr_size = fom.write_fo(fo, buf, count);
    VMS::DisableAccessUser();
    if (wr_size < 0)
    {
        return -1;
    }
    return wr_size;
}

inline int SYS_linkat(int olddirfd, char* oldpath, int newdirfd, char* newpath, uint32 flags)
{
    // 创建文件的链接的系统调用
    // olddirfd是原来的文件所在目录的文件描述符 newdirfd是新的目录
    // oldpath是文件原来的名字 newpath是文件的新名字
    // oldpath是相对路径时 且olddirfd是AT_FDCWD 则是相对于当前路径的
    // oldpath是相对路径时 则是相对于olddirfd的
    // oldpath是绝对路径时 则忽略olddirfd
    // 成功返回0 失败返回-1

    // 暂时还没有实现
    return -1;
}

inline int SYS_unlinkat(int dirfd, char* path, int flags)
{
    // 移除指定文件的链接(可用于删除文件)的系统调用
    // dirfd是要删除的链接所在的目录
    // path是要删除的链接的名字
    // flags可设置为0或AT_REMOVEDIR
    // 成功返回0 失败返回-1

    proc_struct* cur_proc = pm.get_cur_proc();
    file_object* fo_head = cur_proc->fo_head;
    file_object* fo = fom.get_from_fd(fo_head, dirfd);
    if (fo == nullptr)
    {
        return -1;
    }
    VMS::EnableAccessUser();
    if (!vfsm.unlink(path, fo->file->path))
    {
        return -1;
    }
    VMS::DisableAccessUser();
    return 0;
}

inline int SYS_mkdirat(int dirfd, const char* path, int mode)
{
    // 创建目录的系统调用
    // 相关参数的解释可以参考open的系统调用
    // 成功返回0 失败返回-1

    VMS::EnableAccessUser();
    char* rela_wd = nullptr;
    proc_struct* cur_proc = pm.get_cur_proc();
    char* cwd = cur_proc->cur_work_dir;
    if (dirfd == AT_FDCWD)
    {
        rela_wd = cur_proc->cur_work_dir;
    }
    else
    {
        file_object* fo = fom.get_from_fd(cur_proc->fo_head, dirfd);
        if (fo != nullptr)
        {
            rela_wd = fo->file->path;
        }
    }
    char* dir_path = vfsm.unified_path(rela_wd, rela_wd);
    if (dir_path == nullptr)
    {
        return -1;
    }
    bool rt = vfsm.create_dir(dir_path, cwd, (char*)path);
    kfree(dir_path);
    if (!rt)
    {
        return -1;
    }
    return 0;
}

inline int SYS_umount2(const char* special, int flags)
{
    // 卸载文件系统的系统调用
    // 输入为指定的卸载目录和卸载参数
    // 成功返回0 失败返回-1

    return 0;
}

inline int SYS_mount(const char* special, const char* dir, const char* fstype, uint64 flags, const void* data)
{
    // 挂载文件系统的系统调用
    // special为挂载设备
    // dir为挂载点
    // fstype为挂载的文件系统类型
    // flags为挂载参数
    // data为传递给文件系统的字符串参数 可为NULL
    // 成功返回0 失败返回-1

    return 0;
}

struct kstat {
    uint64 st_dev;                      // 文件存放的设备ID
    uint64 st_ino;                      // 索引节点号
    uint32 st_mode;                     // 文件的属性掩码
    uint32 st_nlink;                    // 硬链接的数量
    uint32 st_uid;                      // 文件拥有者的用户ID
    uint32 st_gid;                      // 文件拥有者的组ID
    uint64 st_rdev;                     // 设备ID 仅对部分特殊文件有效
    unsigned long __pad;
    int st_size;                        // 文件大小 单位为字节
    uint32 st_blksize;                  // 文件使用的存储块大小
    int __pad2;
    uint64 st_blocks;                   // 文件占用的存储块数量
    long st_atime_sec;                  // 最后一次访问时间 秒
    long st_atime_nsec;                 // 纳秒
    long st_mtime_sec;                  // 最后一次修改时间 秒
    long st_mtime_nsec;                 // 纳秒
    long st_ctime_sec;                  // 最后一次改变状态的时间 秒
    long st_ctime_nsec;                 // 纳秒
    unsigned __unused[2];
};

inline int SYS_fstat(int fd, kstat* kst)
{
    // 获取文件状态的系统调用
    // 输入fd为文件句柄 kst为接收保存文件状态的指针
    // 目前只需要填充几个值
    // 成功返回0 失败返回-1

    if (kst == nullptr)
    {
        return -1;
    }

    proc_struct* cur_proc = pm.get_cur_proc();
    file_object* fo = fom.get_from_fd(cur_proc->fo_head, fd);
    if (fo == nullptr)
    {
        return -1;
    }
    FAT32FILE* file = fo->file;
    if (file == nullptr)
    {
        return -1;
    }
    VMS::EnableAccessUser();
    memset(kst, 0, sizeof(kstat));
    kst->st_size = file->table.size;
    kst->st_mode = fo->mode;
    kst->st_nlink = 1;
    // ... others to be added
    VMS::DisableAccessUser();
    return 0;
}

inline int SYS_clone(TRAPFRAME* tf, int flags, void* stack, int ptid, int tls, int ctid)
{
    // 创建一个子进程系统调用
    // 成功返回子进程的线程ID 失败返回-1
    // 通过stack来进行fork还是clone
    // fork则是赋值父进程的代码空间 clone则是创建一个新的进程
    // tf参数是依赖于实现的 用来启动进程的

    constexpr uint64 SIGCHLD = 17;      // flags参数
    int pid_ret = -1;
    bool intr_flag;
    intr_save(intr_flag);               // 开关中断 操作的原语性
    proc_struct* cur_proc = pm.get_cur_proc();
    proc_struct* create_proc = pm.alloc_proc();
    if (create_proc == nullptr)
    {
        kout[red] << "SYS_clone alloc proc Fail!" << endl;
        return pid_ret;
    }
    pm.init_proc(create_proc, 2, flags);
    pid_ret = create_proc->pid;
    pm.set_proc_kstk(create_proc, nullptr, cur_proc->kstacksize);
    if (stack == nullptr)
    {
        // 根据clone的系统调用
        // 当stack为nullptr时 此时的系统调用和fork一致
        // 子进程和父进程执行的一段相同的二进制代码
        // 其实就是上下文的拷贝
        // 子进程的创建
        VMS* vms = (VMS*)kmalloc(sizeof(VMS));
        vms->init();
        vms->create_from_vms(cur_proc->vms);
        pm.set_proc_vms(create_proc, vms);
        memcpy((void*)((TRAPFRAME*)((char*)create_proc->kstack + create_proc->kstacksize) - 1),
            (const char*)tf, sizeof(TRAPFRAME));
        create_proc->context = (TRAPFRAME*)((char*)create_proc->kstack + create_proc->kstacksize) - 1;
        create_proc->context->epc += 4;
        create_proc->context->reg.a0 = 0;
    }
    else
    {
        // 此时就是相当于clone
        // 创建一个新的线程 与内存间的共享有关
        // 线程thread的创建
        // 这里其实隐含很多问题 涉及到指令执行流这些 需要仔细体会
        // 在大赛要求和本内核实现下仅仅是充当创建一个函数线程的作用
        pm.set_proc_vms(create_proc, cur_proc->vms);
        memcpy((void*)((TRAPFRAME*)((char*)create_proc->kstack + create_proc->kstacksize) - 1),
            (const char*)tf, sizeof(TRAPFRAME));
        create_proc->context = (TRAPFRAME*)((char*)create_proc->kstack + create_proc->kstacksize) - 1;
        create_proc->context->epc += 4;
        create_proc->context->reg.sp = (uint64)stack;
        create_proc->context->reg.a0 = 0;           // clone里面子线程去执行函数 父线程返回
    }
    pm.copy_other_proc(create_proc, cur_proc);
    if (flags & SIGCHLD)
    {
        // clone的是子进程
        pm.set_proc_fa(create_proc, cur_proc);
    }
    pm.switchstat_proc(create_proc, Proc_ready);
    intr_restore(intr_flag);
    return pid_ret;
}

inline int SYS_execve(const char* path, char* const argv[], char* const envp[])
{
    // 执行一个指定的程序的系统调用
    // 关键是利用解析ELF文件创建进程来实现
    // 这里需要文件系统提供的支持
    // argv参数是程序参数 envp参数是环境变量的数组指针 暂时未使用
    // 执行成功跳转执行对应的程序 失败则返回-1

    VMS::EnableAccessUser();
    proc_struct* cur_proc = pm.get_cur_proc();
    FAT32FILE* file_open = vfsm.open(path, "/");

    if (file_open == nullptr)
    {
        kout[red] << "SYS_execve open File Fail!" << endl;
        return -1;
    }

    file_object* fo = (file_object*)kmalloc(sizeof(file_object));
    fom.set_fo_file(fo, file_open);
    fom.set_fo_pos_k(fo, 0);
    fom.set_fo_flags(fo, 0);

    proc_struct* new_proc = CreateProcessFromELF(fo, cur_proc->cur_work_dir, 0);
    kfree(fo);

    int exit_value = 0;
    if (new_proc == nullptr)
    {
        kout[red] << "SYS_execve CreateProcessFromELF Fail!" << endl;
        return -1;
    }
    else
    {
        // 这里的new_proc其实也是执行这个系统调用进程的子进程
        // 因此这里父进程需要等待子进程执行完毕后才能继续执行
        proc_struct* child = nullptr;
        while (1)
        {
            // 去寻找已经结束的子进程
            // 当前场景的执行逻辑上只会有一个子进程
            proc_struct* pptr = nullptr;
            for (pptr = cur_proc->fir_child;pptr != nullptr;pptr = pptr->bro_next)
            {
                if (pptr->stat == Proc_finished)
                {
                    child = pptr;
                    break;
                }
            }
            if (child == nullptr)
            {
                // 说明当前进程应该被阻塞了
                // 触发进程管理下的信号wait条件
                // 被阻塞之后就不会再调度这个进程了 需要等待子进程执行完毕后被唤醒
                pm.calc_systime(cur_proc);
                cur_proc->sem->wait(cur_proc);
            }
            else
            {
                // 在父进程里回收子进程
                VMS::EnableAccessUser();
                exit_value = child->exit_value;
                VMS::DisableAccessUser();
                pm.free_proc(child);
                break;
            }
        }
    }
    // 顺利执行了execve并回收了子进程
    // 当前进程就执行完毕了 直接退出
    VMS::DisableAccessUser();
    pm.exit_proc(cur_proc, exit_value);
    kout[red] << "SYS_execve reached unreacheable branch!" << endl;
    return -1;
}

inline int SYS_wait4(int pid, int* status, int options)
{
    // 等待进程改变状态的系统调用
    // 这里的改变状态主要是指等待子进程结束
    // pid是指定进程的pid -1就是等待所有子进程
    // status是接受进程退出状态的指针
    // options是指定选项 可以参考linux系统调用文档
    // 这里主要有WNOHANG WUNTRACED WCONTINUED
    // WNOHANG:return immediately if no child has exited.
    // 成功返回进程ID

    constexpr int WNOHANG = 1;                      // 目前暂时只需要这个option
    proc_struct* cur_proc = pm.get_cur_proc();
    if (cur_proc->fir_child == nullptr)
    {
        // 当前父进程没有子进程
        // 一般调用此函数不存在此情况
        kout[red] << "The Process has Not Child Process!" << endl;
        return -1;
    }
    while (1)
    {
        proc_struct* child = nullptr;
        // 利用链接部分的指针关系去寻找满足的孩子进程
        proc_struct* pptr = nullptr;
        for (pptr = cur_proc->fir_child;pptr != nullptr;pptr = pptr->bro_next)
        {
            if (pptr->stat == Proc_finished)
            {
                // 找到对应的进程
                if (pid == -1 || pptr->pid == pid)
                {
                    child = pptr;
                    break;
                }
            }
        }
        if (child != nullptr)
        {
            // 当前的child进程即需要wait的进程
            int ret = child->pid;
            if (status != nullptr)
            {
                // status非空则需要提取相应的状态
                VMS::EnableAccessUser();
                *status = child->exit_value << 8;   // 左移8位主要是Linux系统调用的规范
                VMS::DisableAccessUser();
            }
            pm.free_proc(child);                    // 回收子进程 子进程的回收只能让父进程来进行
            return ret;
        }
        else if (options & WNOHANG)
        {
            // 当没有找到符合的子进程但是options中指定了WNOHANG
            // 直接返回 -1即可
            return -1;
        }
        else
        {
            // 说明还需要等待子进程
            pm.calc_systime(cur_proc);              // 从这个时刻之后的wait时间不应被计算到systime中
            cur_proc->sem->wait();                  // 父进程在子进程上等待 当子进程exit后解除信号量等待可以被回收
        }
    }
    return -1;
}

inline int SYS_getppid()
{
    // 获取父进程PID的系统调用
    // 直接成功返回父进程的ID
    // 根据系统调用规范此调用 always successful

    proc_struct* cur_proc = pm.get_cur_proc();
    proc_struct* fa_proc = cur_proc->fa;
    if (fa_proc == nullptr)
    {
        // 调用规范总是成功
        // 如果是无父进程 可以认为将该进程挂到根进程的孩子进程下
        return idle_proc->pid;
    }
    else
    {
        return fa_proc->pid;
    }
    return -1;
}

inline int SYS_getpid()
{
    // 获取进程PID的系统调用
    // always successful

    proc_struct* cur_proc = pm.get_cur_proc();
    return cur_proc->pid;
}

inline int SYS_brk(uint64 brk)
{
    // 修改数据段大小的系统调用
    // 即修改用户堆段空间
    // 传入待修改的地址
    // 成功返回0 失败返回-1

    proc_struct* cur_proc = pm.get_cur_proc();
    HMR* hmr = cur_proc->heap;
    if (hmr == nullptr)
    {
        return -1;
    }
    if (brk == 0)
    {
        // Linux中应该返回0
        // 这里测试应该返回用户heap堆段的brk地址
        return hmr->breakpoint();
    }
    int64 brk_dlt = brk - hmr->breakpoint();
    if (!hmr->resize(brk_dlt))
    {
        return -1;
    }
    return 0;
}

inline int SYS_munmap(void* start, uint64 len)
{
    // 将文件或设备取消映射到内存中的系统调用
    // 传入参数为映射的指定地址及空间
    // 成功返回0 失败返回-1

    // 暂时还没有实现
    return -1;
}

inline long SYS_mmap(void* start, uint64 len, int prot, int flags, int fd, int off)
{
    // 将文件或设备映射到内存中的系统调用
    // start为映射起始位置
    // len为长度
    // prot为映射的内存保护方式 可取 PROT_EXEC PROT_READ PROT_WRITE PROT_NONE
    // flags为映射是否与其他进程共享的标志
    // fd为文件句柄
    // off为文件偏移量
    // 成功返回已映射区域的指针 失败返回-1

    // 暂时还没有实现
    return -1;
}

struct tms
{
    long tms_utime;                 // user time
    long tms_stime;                 // system time
    long tms_cutime;                // user time of children
    long tms_sutime;                // system time of children
};

inline clock_t SYS_times(tms* tms)
{
    // 获取进程时间的系统调用
    // tms结构体指针 用于获取保存当前进程的运行时间的数据
    // 成功返回已经过去的滴答数 即real time
    // 这里的time基本即指进程在running态的时间
    // 系统调用规范指出为花费的CPU时间 故仅考虑运行时间
    // 而对于running态时间的记录进程结构体中在每次从running态切换时都会记录
    // 对于用户态执行的时间不太方便在用户态记录
    // 这里我们可以认为一个用户进程只有在陷入系统调用的时候才相当于在使用核心态的时间
    // 因此我们在每次系统调用前后记录本次系统调用的时间并加上
    // 进而用总的runtime减去这个时间就可以得到用户时间了
    // 记录这个时间的成员在进程结构体中提供为systime

    proc_struct* cur_proc = pm.get_cur_proc();
    clock_t time_unit = timer_1us;  // 填充tms结构体的基本时间单位 在qemu上模拟 以微秒为单位
    if (tms != nullptr)
    {
        clock_t run_time = pm.get_proc_rumtime(cur_proc, 1);    // 总运行时间
        clock_t sys_time = pm.get_proc_systime(cur_proc, 1);    // 用户陷入核心态的system时间
        clock_t user_time = run_time - sys_time;                // 用户在用户态执行的时间
        if ((int64)run_time < 0 || (int64)sys_time < 0 || (int64)user_time < 0)
        {
            // 3个time有一个为负即认为出错调用失败了
            // 返回-1
            return -1;
        }
        cur_proc->vms->EnableAccessUser();                      // 在核心态操作用户态的数据
        tms->tms_utime = user_time / time_unit;
        tms->tms_stime = sys_time / time_unit;
        // 基于父进程执行到这里时已经wait了所有子进程退出并被回收了
        // 故直接置0
        tms->tms_cutime = 0;
        tms->tms_sutime = 0;
        cur_proc->vms->DisableAccessUser();
    }
    return get_clock_time();
}

struct utsname {
    char sysname[65];
    char nodename[65];
    char release[65];
    char version[65];
    char machine[65];
    char domainname[65];
};

inline int SYS_uname(utsname* uts)
{
    // 打印系统信息的系统调用
    // utsname结构体指针用于获取系统信息数据
    // 成功返回0 失败返回-1
    // 相关信息的字符串处理

    if (uts == nullptr)
    {
        return -1;
    }

    // 操作用户区的数据
    // 需要从核心态进入用户态
    VMS::EnableAccessUser();
    strcpy(uts->sysname, "DBStars_OperatingSystem");
    strcpy(uts->nodename, "DBStars_OperatingSystem");
    strcpy(uts->release, "Debug");
    strcpy(uts->version, "1.0");
    strcpy(uts->machine, "RISCV 64");
    strcpy(uts->domainname, "DBStars");
    VMS::DisableAccessUser();

    return 0;
}

inline int SYS_sched_yield()
{
    // 让出调度器的系统调用
    // 成功返回0 失败返回-1
    // 即pm中的rest功能

    proc_struct* cur_proc = pm.get_cur_proc();
    pm.rest_proc(cur_proc);
    return 0;
}

struct timeval
{
    uint64 sec;                     // 自 Unix 纪元起的秒数
    uint64 usec;                    // 微秒数
};

inline int SYS_gettimeofday(timeval* ts, int tz = 0)
{
    // 获取时间的系统调用
    // timespec结构体用于获取时间值
    // 成功返回0 失败返回-1

    if (ts == nullptr)
    {
        return -1;
    }

    // 其他需要的信息测试平台已经预先软件处理完毕了
    VMS::EnableAccessUser();
    clock_t cur_time = get_clock_time();
    ts->sec = cur_time / timer_1s;
    ts->usec = (cur_time % timer_1s) / timer_1us;
    VMS::DisableAccessUser();
    return 0;
}

struct timespec
{
    long tv_sec;                    // 秒
    long tv_nsec;                   // 纳秒 范围0~999999999
};

inline int SYS_nanosleep(timespec* req, timespec* rem)
{
    // 执行线程睡眠的系统调用
    // sleep()库函数基于此系统调用
    // 输入睡眠的时间间隔
    // 成功返回0 失败返回-1

    if (req == nullptr || rem == nullptr)
    {
        return -1;
    }

    proc_struct* cur_proc = pm.get_cur_proc();
    semaphore.init(0);              // 利用信号量实现sleep 注意区分这里的全局信号量和每个进程内部的信号量的区别
    clock_t wait_time = 0;          // 计算qemu上需要等待的时钟滴答数
    VMS::EnableAccessUser();
    wait_time = req->tv_sec * timer_1s +
        req->tv_nsec / 1000000 * timer_1ms +
        req->tv_nsec % 1000000 / 1000 * timer_1us +
        req->tv_nsec % 1000 * timer_1ns;
    rem->tv_sec = rem->tv_nsec = 0;
    VMS::DisableAccessUser();
    semaphore.wait(cur_proc);       // 当前进程在semaphore上等待 切换为sleeping态
    pm.set_waittime_limit(cur_proc, wait_time);
    clock_t start_timebase = get_clock_time();
    while (1)
    {
        clock_t cur_time = get_clock_time();
        if (cur_time - start_timebase >= wait_time)
        {
            semaphore.signal();
            break;
        }
    }
    semaphore.destroy();            // 这个信号量仅在这里使用 每次使用都是初始化 使用完了即释放空间
    return 0;
}


// 系统调用方式遵循RISC-V ABI
// 即调用号存放在a7寄存器中 6个参数分别存放在a0-a5寄存器中 返回值存放在a0寄存器中
void trap_Syscall(TRAPFRAME* tf)
{
    // debug
    if (tf->reg.a7 >= 0)
    {
        // kout[green] << "Process's pid " << pm.get_cur_proc()->pid << " Syscall:" << endl;
        // kout[green] << "Syscall Number " << tf->reg.a7 << endl;
        // kout[green] << "Syscall name " << get_syscall_name(tf->reg.a7) << endl;
    }
    switch (tf->reg.a7)
    {
    case Sys_putchar:
        SYS_putchar(tf->reg.a0);
        break;
    case Sys_getchar:
        tf->reg.a0 = SYS_getchar();
        break;
    case Sys_getcwd:
        tf->reg.a0 = (uint64)SYS_getcwd((char*)tf->reg.a0, tf->reg.a1);
        break;
    case Sys_pipe2:
        tf->reg.a0 = SYS_pipe2((int*)tf->reg.a0, tf->reg.a1);
        break;
    case Sys_dup:
        tf->reg.a0 = SYS_dup(tf->reg.a0);
        break;
    case Sys_dup3:
        tf->reg.a0 = SYS_dup3(tf->reg.a0, tf->reg.a1);
        break;
    case Sys_chdir:
        tf->reg.a0 = SYS_chdir((const char*)tf->reg.a0);
        break;
    case Sys_openat:
        tf->reg.a0 = SYS_openat(tf->reg.a0, (const char*)tf->reg.a1, tf->reg.a2, tf->reg.a3);
        break;
    case Sys_close:
        tf->reg.a0 = SYS_close(tf->reg.a0);
        break;
    case Sys_getdents64:
        tf->reg.a0 = SYS_getdents64(tf->reg.a0, (const void*)tf->reg.a1, tf->reg.a2);
        break;
    case Sys_read:
        tf->reg.a0 = SYS_read(tf->reg.a0, (void*)tf->reg.a1, tf->reg.a2);
        break;
    case Sys_write:
        tf->reg.a0 = SYS_write(tf->reg.a0, (void*)tf->reg.a1, tf->reg.a2);
        break;
    case Sys_linkat:
        tf->reg.a0 = SYS_linkat(tf->reg.a0, (char*)tf->reg.a1, tf->reg.a2, (char*)tf->reg.a3, tf->reg.a4);
        break;
    case Sys_unlinkat:
        tf->reg.a0 = SYS_unlinkat(tf->reg.a0, (char*)tf->reg.a1, tf->reg.a2);
        break;
    case Sys_mkdirat:
        tf->reg.a0 = SYS_mkdirat(tf->reg.a0, (const char*)tf->reg.a1, tf->reg.a2);
        break;
    case Sys_umount2:
        tf->reg.a0 = SYS_umount2((const char*)tf->reg.a0, tf->reg.a1);
        break;
    case Sys_mount:
        tf->reg.a0 = SYS_mount((const char*)tf->reg.a0, (const char*)tf->reg.a1,
            (const char*)tf->reg.a2, tf->reg.a3, (const void*)tf->reg.a4);
        break;
    case Sys_fstat:
        tf->reg.a0 = SYS_fstat(tf->reg.a0, (kstat*)tf->reg.a1);
        break;
    case Sys_clone:
        // kout[green] << tf->reg.a0 << endl;
        tf->reg.a0 = SYS_clone(tf, tf->reg.a0, (void*)tf->reg.a1, tf->reg.a2, tf->reg.a3, tf->reg.a4);
        // kout[green] << tf->reg.a0 << endl;
        // kout[green] << tf->reg.a1 << endl;
        break;
    case Sys_execve:
        tf->reg.a0 = SYS_execve((char*)tf->reg.a0, (char**)tf->reg.a1, (char**)tf->reg.a2);
        break;
    case Sys_wait4:
        tf->reg.a0 = SYS_wait4(tf->reg.a0, (int*)tf->reg.a1, tf->reg.a2);
        break;
    case Sys_exit:
        SYS_exit(tf->reg.a0);
        break;
    case Sys_getppid:
        tf->reg.a0 = SYS_getppid();
        break;
    case Sys_getpid:
        tf->reg.a0 = SYS_getpid();
        break;
    case Sys_brk:
        tf->reg.a0 = SYS_brk(tf->reg.a0);
        break;
    case Sys_munmap:
        tf->reg.a0 = SYS_munmap((void*)tf->reg.a0, tf->reg.a1);
        break;
    case Sys_mmap:
        tf->reg.a0 = SYS_mmap((void*)tf->reg.a0, tf->reg.a1, tf->reg.a2, tf->reg.a3, tf->reg.a4, tf->reg.a5);
        break;
    case Sys_times:
        tf->reg.a0 = SYS_times((tms*)tf->reg.a0);
        break;
    case Sys_uname:
        tf->reg.a0 = SYS_uname((utsname*)tf->reg.a0);
        break;
    case Sys_sched_yield:
        tf->reg.a0 = SYS_sched_yield();
        break;
    case Sys_gettimeofday:
        tf->reg.a0 = SYS_gettimeofday((timeval*)tf->reg.a0, 0);
        break;
    case Sys_nanosleep:
        tf->reg.a0 = SYS_nanosleep((timespec*)tf->reg.a0, (timespec*)tf->reg.a1);
        break;
    default:
        // 遇到了未定义的系统调用号
        // 这里的处理就是输出相关信息即可
        kout[yellow] << "The Syscall has not defined!" << endl;
        kout[yellow] << "Current Process's pid is " << pm.get_cur_proc()->pid << endl;
        if (pm.get_cur_proc()->ku_flag == 2)
        {
            // 是用户进程发起的系统调用
            // 那么这个用户进程遇到这种情况应该直接终止
            pm.exit_proc(pm.get_cur_proc(), -1);
            kout[red] << "trap_Syscall : reached unreachable branch!" << endl;
        }
        break;
    }
}