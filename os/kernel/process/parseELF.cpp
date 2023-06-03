#include <parseELF.hpp>

int start_process_formELF(void* userdata)
{
    // 从ELF解析产生的进程会执行这个启动函数
    // 传入参数完成进程的初始化后进入用户状态执行程序
    procdata_fromELF* proc_data = (procdata_fromELF*)userdata;
    file_object* fo = proc_data->fo;
    proc_struct* proc = proc_data->proc;
    VMS* vms = proc_data->vms;

    vms->EnableAccessUser();

    uint64 breakpoint = 0;                              // 用来定义用户数据段
    uint16 phnum = proc_data->e_header.e_phnum;         // 可执行文件中的需要解析的段数量
    uint64 phoff = proc_data->e_header.e_phoff;         // 段表在文件中的偏移量 需要通过段表访问到每个段
    uint16 phentsize = proc_data->e_header.e_phentsize; // 段表中每个段表项的大小
    // kout << phnum << ' ' << phoff << ' ' << phentsize << endl;
    for (int i = 0;i < phnum;i++)
    {
        // 解析每一个程序头 即段
        Elf_Phdr pgm_hdr{ 0 };
        // 为了读取文件中这个位置的段
        // 需要计算偏移量通过seek操作进行读取
        int64 offset = phoff + i * phentsize;
        fom.seek_fo(fo, offset, file_ptr::Seek_beg);
        int64 rd_size = 0;
        rd_size = fom.read_fo(fo, &pgm_hdr, sizeof(pgm_hdr));
        if (rd_size != sizeof(pgm_hdr))
        {
            // 没有成功读到指定大小的文件
            // 输出提示信息并且终止函数执行            
            kout[red] << "Read ELF program header Fail!" << endl;
            return -1;
        }

        bool is_continue = 0;
        uint32 type = pgm_hdr.p_type;
        switch (type)
        {
        case P_type::PT_LOAD:
            // 读到可装载的段了可以直接退出
            kout << "TT" << endl;
            break;
        default:
            // 其他的情况输出相关提示信息即可
            if (type >= P_type::PT_LOPROC && type <= P_type::PT_HIPROC)
            {
                kout[yellow] << "Currently the ELF Segment parsing is unsolvable!" << endl;
                kout[yellow] << "The Segment Type is " << type << endl;
                is_continue = 1;
            }
            else
            {
                kout[red] << "Unsolvable ELF Segment whose Type is " << type << endl;
                return -1;
            }
            break;
        }

        if (is_continue)
        {
            // 说明需要继续去读段
            // 直接在这里跳转 否则就继续进行下面的转载
            continue;
        }

        // 统计相关信息
        // 加入VMR
        uint32 vmr_flags = 0;
        uint32 ph_flags = pgm_hdr.p_flags;
        // 段权限中可读与可执行是通用的
        // 可写权限是最高权限 可以覆盖其他两个
        if (ph_flags & P_flags::PF_R)
        {
            kout << "SS" << endl;
            vmr_flags |= 0b1;
            vmr_flags |= 0b100;
        }
        if (ph_flags & P_flags::PF_X)
        {
            kout << "SSS" << endl;
            vmr_flags |= 0b100;
            vmr_flags |= 0b1;
        }
        if (ph_flags & P_flags::PF_W)
        {
            kout << "S" << endl;
        }
        if (ph_flags & P_flags::PF_MASKPROC)
        {
            kout << "SSSS" << endl;
        }
        vmr_flags = VM_rwx;
        // 权限统计完 可以在进程的虚拟空间中加入这一片VMR区域了
        // 这边使用的memsz信息来作为vmr的信息
        // 输出提示信息
        uint64 vmr_begin = pgm_hdr.p_vaddr;
        uint64 vmr_memsize = pgm_hdr.p_memsz;
        uint64 vmr_end = vmr_begin + vmr_memsize;
        kout[green] << "Add VMR from " << vmr_begin << " to " << vmr_end << endl;

        VMR* vmr_add = (VMR*)kmalloc(sizeof(VMR));

        vmr_add->Init(vmr_begin, vmr_end, vmr_flags);
        vms->insert(vmr_add);
        kout[yellow] << "BREAK!" << endl;
        kout[yellow] << Hex(vmr_begin) << endl;
        memset((char*)vmr_begin, 0, vmr_memsize);
        kout[yellow] << "BREAK!" << endl;
        uint64 tmp_end = vmr_add->GetEnd();
        if (tmp_end > breakpoint)
        {
            // 更新用户空间的数据段(heap)
            breakpoint = tmp_end;
        }

        // 上面读取了程序头
        // 接下来继续去读取段的在文件中的内容
        // 将对应的内容存放到进程中vaddr的区域
        // 这边就是用filesz来读取具体的内容
        fom.seek_fo(fo, pgm_hdr.p_offset, file_ptr::Seek_beg);
        rd_size = fom.read_fo(fo, &vmr_begin, pgm_hdr.p_filesz);
        if (rd_size != pgm_hdr.p_filesz)
        {
            kout[red] << "Read ELF program header in file Fail!" << endl;
            return -1;
        }
    }


    // 上面的循环已经读取了所有的段信息
    // 接下来更新进程需要的相关信息即可
    // 首先是为用户进程开辟一块用户栈空间
    uint64 vmr_user_stack_beign = EmbedUserProcessStackAddr;
    uint64 vmr_user_stack_size = EmbedUserProcessStackSize;
    uint64 vmr_user_stack_end = vmr_user_stack_beign + vmr_user_stack_size;
    VMR* vmr_user_stack = (VMR*)kmalloc(sizeof(VMR));
    vmr_user_stack->Init(vmr_user_stack_beign, vmr_user_stack_end, VMR_flags::VM_userstack);
    vms->insert(vmr_user_stack);
    memset((char*)vmr_user_stack_beign, 0, vmr_user_stack_size);

    // 用户堆段信息 也即数据段
    HMR* hmr = (HMR*)kmalloc(sizeof(HMR));
    hmr->Init(breakpoint);
    vms->insert(hmr);
    memset((char*)hmr->GetStart(), 0, hmr->GetLength());
    // 在这里将进程的heap成员更新
    // 也是在进程管理部分基本不会操作heap段的原因
    pm.set_proc_heap(proc, hmr);

    vms->DisableAccessUser();

    // 正确完整地执行了这个流程
    // 接触阻塞并且返回0
    proc_data->sem.signal();
    return 0;
}

proc_struct* CreateProcessFromELF(file_object* fo, const char* wk_dir, int proc_flags)
{
    procdata_fromELF* proc_data = (procdata_fromELF*)kmalloc(sizeof(procdata_fromELF));
    proc_data->fo = fo;
    // 信号量初始化
    proc_data->sem.init();

    int64 rd_size = 0;
    rd_size = fom.read_fo(fo, &proc_data->e_header, sizeof(proc_data->e_header));

    if (rd_size != sizeof(proc_data->e_header) || !proc_data->e_header.is_ELF())
    {
        kout[red] << "Create Process from ELF Error!" << endl;
        kout[red] << "Read File is NOT ELF!" << endl;
        kfree(proc_data);
        return nullptr;
    }

    VMS* vms = (VMS*)kmalloc(sizeof(VMS));
    vms->init();

    proc_struct* proc = pm.alloc_proc();
    pm.init_proc(proc, 2, proc_flags);
    pm.set_proc_kstk(proc, nullptr, KERNELSTACKSIZE * 4);
    pm.set_proc_vms(proc, vms);
    pm.set_proc_fa(proc, pm.get_cur_proc());

    // 通过vfsm得到标准化的绝对路径
    char* abs_cwd = vfsm.unified_path(wk_dir, pm.get_cur_proc()->cur_work_dir);
    pm.set_proc_cwd(proc, abs_cwd);
    kfree(abs_cwd);

    // 填充userdata
    // 然后跳转执行指定的启动函数
    proc_data->vms = vms;
    proc_data->proc = proc;
    uint64 user_start_addr = proc_data->e_header.e_entry;
    pm.start_user_proc(proc, start_process_formELF, proc_data, user_start_addr);

    // 这里跳转到启动函数之后进程管理就会有其他进程参与轮转调度了
    // 为了确保新的进程能够顺利执行完启动函数这里再释放相关的资源
    // 这里让当前进程阻塞在这里
    // proc_data->sem.wait();
    // proc_data->sem.destroy();
    // kfree(proc_data);

    return proc;
}