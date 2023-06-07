#include <fileobject.hpp>
#include <vfsm.hpp>

FAT32FILE* STDIO = nullptr;

void FileObjectManager::init_proc_fo_head(proc_struct* proc)
{
    if (proc == nullptr)
    {
        kout[red] << "The Process has not existed!" << endl;
        return;
    }

    // 初始化进程的虚拟头节点
    // 使用场景下必须保证一定是空的
    if (proc->fo_head != nullptr)
    {
        kout[yellow] << "The Process's fo_head is not Empty!" << endl;
        free_all_flobj(proc->fo_head);
        kfree(proc->fo_head);
    }
    proc->fo_head = (file_object*)kmalloc(sizeof(file_object));
    if (proc->fo_head == nullptr)
    {
        kout[red] << "The fo_head malloc Fail!" << endl;
        return;
    }
    proc->fo_head->next = nullptr;
    proc->fo_head->fd = -1;                 // 头节点的fd默认置为-1
    proc->fo_head->file = nullptr;          // 头节点的file指针置空 不会有任何引用和指向
    proc->fo_head->pos_k = -1;              // 这些成员在头节点的定义下均不会使用 置-1即可 无符号数类型下
    proc->fo_head->flags = -1;
    proc->fo_head->mode = -1;
}

int FileObjectManager::get_count_fdt(file_object* fo_head)
{
    // 从虚拟头节点出发去统计这个链表的长度即可
    int ret_count = 0;
    if (fo_head == nullptr)
    {
        kout[red] << "The fo_head is Empty!" << endl;
        return ret_count;
    }

    file_object* fo_ptr = fo_head->next;
    while (fo_ptr != nullptr)
    {
        ret_count++;
        fo_ptr = fo_ptr->next;
    }
    return ret_count;
}

int FileObjectManager::find_sui_fd(file_object* fo_head)
{
    if (fo_head == nullptr)
    {
        kout[red] << "The fo_head is Empty!" << endl;
        return 0;
    }

    int cur_len = get_count_fdt(fo_head);
    int st[cur_len + 3 + 1];                // 当前有这么多节点 使用cur_len+1个数一定可以找到合适的fd
    memset(st, 0, sizeof(st));
    st[0] = 1;                              // 标准输出的fd是默认分配好的 不应该被占用
    st[1] = 1;
    st[2] = 1;
    file_object* fo_ptr = fo_head->next;
    while (fo_ptr != nullptr)
    {
        int used_fd = fo_ptr->fd;
        if (used_fd > cur_len)
        {
            // 这个使用的fd已经超出当前数组长度
            // 不统计即可
            fo_ptr = fo_ptr->next;
            continue;
        }
        st[used_fd] = 1;
        fo_ptr = fo_ptr->next;
    }
    int ret_fd = -1;
    for (int i = 0;i <= cur_len + 3;i++)
    {
        if (!st[i])
        {
            // 找到第一个符合的最小的非负整数fd
            ret_fd = i;
            break;
        }
    }
    return ret_fd;
}

file_object* FileObjectManager::create_flobj(file_object* fo_head, int fd)
{
    if (fo_head == nullptr)
    {
        kout[red] << "The fo_head is Empty!" << endl;
        return nullptr;
    }

    if (fd < -1)
    {
        kout[red] << "The fd cannot be FD!" << endl;
        return nullptr;
    }

    file_object* new_fo = (file_object*)kmalloc(sizeof(file_object));
    if (new_fo == nullptr)
    {
        kout[red] << "The fo_head malloc Fail!" << endl;
        return nullptr;
    }
    int new_fd = fd;
    if (fd == -1)
    {
        // 自动分配一个最小的合适的非负整数作为fd
        new_fd = find_sui_fd(fo_head);
    }
    new_fo->fd = new_fd;
    new_fo->file = nullptr;                 // 相关属性在之后会有相关设置
    new_fo->next = nullptr;
    new_fo->flags = -1;
    new_fo->mode = -1;
    new_fo->pos_k = 0;
    // 分配完成之后插入这个链表的最后一个位置即可
    file_object* fo_ptr = fo_head;
    while (fo_ptr->next != nullptr)
    {
        fo_ptr = fo_ptr->next;
    }
    fo_ptr->next = new_fo;
    return new_fo;                          // 返回这个创建的新的节点用来设置相关属性
}

file_object* FileObjectManager::get_from_fd(file_object* fo_head, int fd)
{
    if (fo_head == nullptr)
    {
        kout[red] << "The fo_head is Empty!" << endl;
        return nullptr;
    }

    // 简单遍历链表返回即可
    file_object* ret_fo = nullptr;
    file_object* fo_ptr = fo_head->next;
    while (fo_ptr != nullptr)
    {
        if (fo_ptr->fd == fd)
        {
            ret_fo = fo_ptr;
            break;
        }
        fo_ptr = fo_ptr->next;
    }
    if (ret_fo == nullptr)
    {
        // 对于未找到的情况加个输出信息
        kout[yellow] << "The specified fd cannot be found!" << endl;
    }
    return ret_fo;
}

void FileObjectManager::free_all_flobj(file_object* fo_head)
{
    if (fo_head == nullptr)
    {
        kout[red] << "The fo_head is Empty!" << endl;
        return;
    }

    // 接下来就是删除除虚拟头节点外链表所有的节点
    // 对于单个虚拟头节点资源的释放交给进程自己去管理
    // 释放相关的资源
    // 注意这里只是和链表的交互
    // 依旧不需要去管理具体的打开文件的资源
    // 引用计数相关的问题是在文件相关操作被处理
    file_object* fo_ptr = fo_head;
    file_object* fo_del = nullptr;
    while (fo_ptr->next != nullptr)
    {
        fo_del = fo_ptr->next;
        fo_ptr->next = fo_ptr->next->next;
        kfree(fo_del);
    }
    return;
}

int FileObjectManager::add_fo_tolist(file_object* fo_head, file_object* fo_added)
{
    if (fo_head == nullptr)
    {
        kout[red] << "The fo_head is Empty!" << endl;
        return -1;
    }

    if (fo_added == nullptr)
    {
        kout[yellow] << "The fo_added is Empty!" << endl;
        return -1;
    }

    // 因为是将新的fo插入链表
    // 所以对于插入的fo的fd需要检查 防止出现重复的情况
    if (fo_added->fd == -1 || fo_added->fd < 0)
    {
        // -1或负即在当前fo链表基础上自动分配
        int fd_added = find_sui_fd(fo_head);
        fo_added->fd = fd_added;
    }
    else
    {
        // 检查一下当前这个插入的fo的fd是否已经在待插入链表上存在了
        file_object* fo_ptr = fo_head->next;
        bool is_exist = false;
        while (fo_ptr != nullptr)
        {
            if (fo_ptr->fd == fo_added->fd)
            {
                is_exist = true;
                break;
            }
            fo_ptr = fo_ptr->next;
        }
        if (is_exist)
        {
            int fd_added = find_sui_fd(fo_head);
            fo_added->fd = fd_added;
        }
    }

    file_object* fo_ptr = fo_head;
    while (fo_ptr->next != nullptr)
    {
        fo_ptr = fo_ptr->next;
    }
    fo_ptr->next = fo_added;
    // 返回新插入的fo节点的fd
    return fo_added->fd;
}

void FileObjectManager::delete_flobj(file_object* fo_head, file_object* del)
{
    if (fo_head == nullptr)
    {
        kout[red] << "The fo_head is Empty!" << endl;
        return;
    }

    if (del == nullptr)
    {
        kout[yellow] << "The deleted fo is Empty!" << endl;
        return;
    }

    file_object* fo_ptr = fo_head->next;
    file_object* fo_pre = fo_head;
    while (fo_ptr != nullptr)
    {
        if (fo_ptr == del)
        {
            // 当前这个节点是需要被删除的
            // 理论上讲fo节点是唯一的 这里删除完了即可退出了
            // 出于维护链表结构的统一性模板
            fo_pre->next = fo_ptr->next;
            kfree(fo_ptr);
            fo_ptr = fo_pre;
        }
        fo_pre = fo_ptr;
        fo_ptr = fo_ptr->next;
    }
    return;
}

bool FileObjectManager::set_fo_fd(file_object* fo, int fd)
{
    if (fo == nullptr)
    {
        kout[red] << "The fo is Empty cannot be set!" << endl;
        return false;
    }

    if (fd < 0)
    {
        kout[red] << "The fd to be set is Negative!" << endl;
        return false;
    }

    fo->fd = fd;
    return true;
}

bool FileObjectManager::set_fo_file(file_object* fo, FAT32FILE* file)
{
    if (fo == nullptr)
    {
        kout[red] << "The fo is Empty cannot be set!" << endl;
        return false;
    }

    if (file == nullptr)
    {
        kout[red] << "The file to be set is Empty!" << endl;
        return false;
    }

    fo->file = file;
    return true;
}

bool FileObjectManager::set_fo_pos_k(file_object* fo, uint64 pos_k)
{
    if (fo == nullptr)
    {
        kout[red] << "The fo is Empty cannot be set!" << endl;
        return false;
    }

    fo->pos_k = pos_k;
    return true;
}

bool FileObjectManager::set_fo_flags(file_object* fo, uint64 flags)
{
    if (fo == nullptr)
    {
        kout[red] << "The fo is Empty cannot be set!" << endl;
        return false;
    }

    fo->flags = flags;
    return true;
}

bool FileObjectManager::set_fo_mode(file_object* fo, uint64 mode)
{
    if (fo == nullptr)
    {
        kout[red] << "The fo is Empty cannot be set!" << endl;
        return false;
    }

    fo->mode = mode;
    return true;
}

int64 FileObjectManager::read_fo(file_object* fo, void* dst, uint64 size)
{
    if (fo == nullptr)
    {
        kout[red] << "Read fo the fo is NULL!" << endl;
        return -1;
    }
    if (dst == nullptr)
    {
        kout[red] << "Read fo the dst pointer is NULL!" << endl;
        return -1;
    }

    FAT32FILE* file = fo->file;
    if (file == nullptr)
    {
        kout[red] << "Read fo the file pointer is NULL!" << endl;
        return -1;
    }
    int64 rd_size = file->read((unsigned char*)dst, fo->pos_k, size);
    return rd_size;
}

int64 FileObjectManager::write_fo(file_object* fo, void* src, uint64 size)
{
    if (fo == nullptr)
    {
        kout[red] << "Write fo the fo is NULL!" << endl;
        return -1;
    }
    if (src == nullptr)
    {
        kout[red] << "Write fo the src pointer is NULL!" << endl;
        return -1;
    }

    FAT32FILE* file = fo->file;
    if (file == nullptr)
    {
        kout[red] << "Write fo the file pointer is NULL!" << endl;
        return -1;
    }
    int64 wr_size = file->write((unsigned char*)src, size);
    return wr_size;
}

void FileObjectManager::seek_fo(file_object* fo, int64 bias, int32 base)
{
    if (base == Seek_beg)
    {
        fo->pos_k = bias;
    }
    else if (base == Seek_cur)
    {
        fo->pos_k += bias;
    }
    else if (base == Seek_end)
    {
        fo->pos_k = fo->file->table.size;
    }
}

bool FileObjectManager::close_fo(proc_struct* proc, file_object* fo)
{
    if (proc == nullptr)
    {
        kout[red] << "Close fo the Process is Not Exist!" << endl;
        return false;
    }
    if (fo == nullptr)
    {
        kout[yellow] << "The fo is Empty not need to be Closed!" << endl;
        return true;
    }

    // 关闭这个文件描述符
    // 首先通过vfsm的接口直接对于fo对于的file类调用close函数
    // 这里的close接口会自动处理引用计数相关文件
    // 进程完全不需要进行对文件的任何操作
    // 这就是这一层封装和隔离的妙处所在
    FAT32FILE* file = fo->file;
    vfsm.close(file);
    // 同时从进程的文件描述符表中删去这个节点
    file_object* fo_head = proc->fo_head;
    fom.delete_flobj(fo_head, fo);
    return true;
}

file_object* FileObjectManager::duplicate_fo(file_object* fo)
{
    if (fo == nullptr)
    {
        kout[red] << "The fo is Empty cannot be set!" << endl;
        return nullptr;
    }

    // 拷贝当前的fo并返回一个新的fo即可
    file_object* dup_fo = (file_object*)kmalloc(sizeof(file_object));
    if (dup_fo == nullptr)
    {
        kout[red] << "The fo_head malloc Fail!" << endl;
        return nullptr;
    }
    dup_fo->next = nullptr;
    // 直接原地赋值 不调用函数了 节省开销
    dup_fo->fd = -1;                        // 这个fd在需要插入这个fo节点时再具体化
    dup_fo->file = fo->file;                // 关键是file flags这些信息的拷贝
    dup_fo->flags = fo->flags;
    dup_fo->pos_k = fo->pos_k;
    return dup_fo;
}

// 声明的全局变量实例化
FileObjectManager fom;