#include <FAT32.hpp>

FAT32FILE::FAT32FILE(FATtable ft, char* lName, uint64 _clus, uint64 pos, char* _path)
{
    table = ft;
    clus = table.low_clus | (table.high_clus << 16);

    TYPE |= table.type & 0x10 ? 1 : 0;
    name = new char[strlen(lName) + 1];
    strcpy(name, lName);
    table_clus_pos = _clus;
    table_clus_off = pos;
    next = nullptr;
    pre = nullptr;
    fat = nullptr;
    ref = 0;

    if (_path)
    {
        path = new char[100];
        strcpy(path, _path);
    }
}
bool FAT32::unlink(FAT32FILE* file)
{
    FATtable* p = (FATtable*)new unsigned char[Dbr.clus_size];
    get_clus(file->table_clus_pos, (unsigned char*)p);


    uint64 k = file->table_clus_off;
    p[k].type1 = 0x0f;
    while (p[k].attribute != 0xe5 && p[k].type1 == 0x0f)
    {
        p[k].attribute = 0xe5;
        k--;
    }
    set_clus(file->table_clus_pos, (unsigned char*)p);

    uint64 rclus = file->clus;
    uint64 nxt;

    delete[] p;
}

FAT32FILE::~FAT32FILE()
{
    delete[] name;
    delete[] path;
}

bool FAT32FILE::set_path(char* _path)
{
    path = new char[100];
    strcpy(path, _path);
    return true;
}

int32 FATtable::get_name(char* REname)
{
    int i;
    if (name[0] == 0 || name[0] == 0xe5)
        return -1;
    else if (type == 0x0F)
    {
        int j = 0;
        i = 0;
        while (i < 10)
        {
            REname[j] = lname0[i];
            j++;
            i++;
        }
        i = 0;

        while (i < 12)
        {
            REname[j] = lname1[i];
            j++;
            i++;
        }
        i = 0;

        while (i < 4)
        {
            REname[j] = lname2[i];
            j++;
            i++;
        }

        return attribute;
    }
    else if (name[0] == 0x2e)
    {
        return -2;
    }
    else if (name[0] != 0xe5)
    {
        for (i = 0; i < 8; i++)
        {
            if (name[i] != ' ')
                REname[i] = name[i];
            else
                break;
        }
        if (ex_name[0] != ' ')
        {
            REname[i++] = '.';
            REname[i++] = ex_name[0];
            REname[i++] = ex_name[1];
            REname[i++] = ex_name[2];
        }
        REname[i] = 0;

        return 0;
    }
}

bool FAT32::init()
{
    // dev.init();
    DBRlba = 0;
    unsigned char* buf = new unsigned char[512];

    dev.init();

    dev.read(DBRlba, buf);
    if (buf[510] != 0x55 || buf[511] != 0xaa)
    {
        kout[red] << "diskpart error" << endl;
        return false;
    }

    Dbr.sector_size = buf[0xb] | (buf[0xc] << 8);
    Dbr.clus_sector_num = buf[0xd];
    Dbr.rs_sector_num = buf[0xe] | (buf[0xf] << 8);
    Dbr.FAT_num = buf[0x10];
    Dbr.all_sector_num = buf[0x20] | (buf[0x21] << 8) | (buf[0x22] << 16) | (buf[0x23] << 24);
    Dbr.FAT_sector_num = buf[0x24] | (buf[0x25] << 8) | (buf[0x26] << 16) | (buf[0x27] << 24);
    Dbr.root_clus = buf[0x2c] | (buf[0x2d] << 8) | (buf[0x2e] << 16) | (buf[0x2f] << 24);
    Dbr.clus_size = Dbr.clus_sector_num * Dbr.sector_size;

    DBRlba = 0;
    FAT1lba = Dbr.rs_sector_num;
    FAT2lba = FAT1lba + Dbr.FAT_sector_num;
    DATAlba = FAT1lba + Dbr.FAT_sector_num * Dbr.FAT_num;

    // kout[yellow] << "Dbr.sector_size     " << Dbr.sector_size << endl;
    // kout[yellow] << "Dbr.FAT_num         " << Dbr.FAT_num << endl;
    // kout[yellow] << "Dbr.FAT_sector_num  " << Dbr.FAT_sector_num << endl;
    // kout[yellow] << "DATAlba             " << DATAlba << endl;
    // kout[yellow] << "ClusSector          " << Dbr.clus_sector_num << endl;
    dev.read(DBRlba, buf);
    // for (int i = 0; i < 16; i++)
    //     kout[green] << buf[i] << ' ';
    // kout[green] << endl;

    // kout[yellow] << "FAT1lba             " << FAT1lba << endl;
    dev.read(FAT1lba, buf);
    // for (int i = 0; i < 16; i++)
    //     kout[green] << buf[i] << ' ';
    // kout[green] << endl;

    // kout[yellow] << "FAT2lba             " << FAT2lba << endl;
    dev.read(FAT2lba, buf);
    // for (int i = 0; i < 16; i++)
    //     kout[green] << buf[i] << ' ';
    // kout[green] << endl;

    // kout[yellow] << "DATAlba             " << DATAlba << endl;
    dev.read(DATAlba, buf);
    // for (int i = 0; i < 16; i++)
    //     kout[green] << (char)buf[i] << ' ';
    // kout[green] << endl;

    delete[] buf;
    return true;
}

FAT32::FAT32()
{
    this->init();
}

bool FAT32::get_clus(uint64 clus, unsigned char* buf)
{
    if (clus >= 0xffffff7)
    {
        kout[red] << "can't find clus" << endl;
        return false;
    }

    int lba = clus_to_lba(clus);
    // kout<<lba<<endl;
    for (int i = 0; i < Dbr.clus_sector_num; i++)
    {
        dev.read(lba + i, buf + i * Dbr.sector_size);
    }

    return true;
}

bool FAT32::get_clus(uint64 clus, unsigned char* buf, uint64 start, uint64 end)
{
    if (clus >= 0xffffff7)
    {
        kout[red] << "can't find clus" << endl;
        return false;
    }

    int lba = clus_to_lba(clus);

    for (int i = 0; i < Dbr.clus_sector_num; i++)
    {
        dev.read(lba + i, buf + i * Dbr.sector_size);
    }
    for (uint64 i = start; i < end - start; i++)
    {
        buf[i] = buf[i + start];
    }

    return true;
}

FAT32FILE* FAT32::get_child_form_clus(char* child_name, uint64 src_clus)
{
    // kout[blue]<<child_name<<' '<<src_lba<<endl;
    unsigned char* clus = new unsigned char[Dbr.clus_size];
    get_clus(src_clus, clus);
    FATtable* ft = (FATtable*)clus;
    FAT32FILE* re = nullptr;

    int32 t;
    int32 k;
    char* sName = new char[30];
    sName[26] = 0;
    sName[27] = 0;
    char* lName = new char[100];

    // for (int i = 0; i < 10; i++)
    while (src_clus < 0xffffff8)
    {
        for (int i = 0; i < Dbr.clus_size / 32; i++)
        {
            while ((t = ft[i].get_name(sName)) > 0)
            {
                unicode_to_ascii(sName);
                if (t & 0x40)
                {
                    // kout[red]<<"yes"<<endl;
                    strcpy(&lName[((t & 0xf) - 1) * 13], sName);
                }
                else
                    strcpy_no_end(&lName[((t & 0xf) - 1) * 13], sName);
                // kout[purple] << sName << ' ' << t << endl;
                i++;
            }
            if (t == -1)
                continue;

            // if (sName[6] != '~' || !isdigit(sName[7]))
            //     strcpy(lName, sName);

            // kout[green] << lName << endl;

            if (strcmp(lName, child_name) == 0 && ft[i].attribute != 0xe5)
            {
                re = new FAT32FILE(ft[i], lName, src_clus, i);
                delete[] sName;
                delete[] clus;
                return re;
            }
        }

        // kout[yellow]<<src_clus<<endl;
        src_clus = get_next_clus(src_clus);
        if (src_clus < 0xffffff8)
            get_clus(src_clus, clus);
    }
    delete[] sName;
    delete[] clus;
    delete[] lName;
    return re;
}

uint64 FAT32::lba_to_clus(uint64 lba)
{
    return (lba - DATAlba) / Dbr.clus_sector_num + 2;
}

uint64 FAT32::clus_to_lba(uint64 clus)
{
    return (clus - 2) * Dbr.clus_sector_num + DATAlba;
}

FAT32FILE* FAT32::FAT32::find_file_by_path(char* path)
{
    char* sigleName = new char[50];
    unsigned char* clus = new unsigned char[Dbr.clus_size];
    uint64 clus_n = Dbr.root_clus;
    FAT32FILE* tb;

    FATtable* ft;
    while (path = split_path_name(path, sigleName))
    {
        // kout[green] << sigleName << endl;
        tb = get_child_form_clus(sigleName, clus_n);
        if (tb == nullptr)
        {
            // kout[red] << "can't find file:" << sigleName << endl;
            return nullptr;
        }
        if (path[0])
        {
            // kout[yellow] << tb->name << endl;
            if (tb->table.type & 0x10) // 目录
            {
                clus_n = tb->table.low_clus | (uint64)tb->table.high_clus << 16;
                // kout[yellow]<<"dir"<<endl;
            }
            else
            {
                kout[red] << "path fail " << sigleName << " is not't a director" << endl;
            }
        }
    }
    // if (tb->table.type & 0x10)
    // {
    //     kout[red] << "error path:path is a dir " << path << endl;
    //     return nullptr;
    // }
    if (tb)
    {
        tb->fat = this;
    }
    delete[] sigleName;
    // kout[blue]<<cpath<<endl;
    return tb;
}

FAT32FILE* FAT32::create_file(FAT32FILE* dir, char* fileName, uint8 type)
{
    FATtable temp;
    memset((unsigned char*)&temp, 0, 32);
    if (dir == nullptr)
    {
        kout[red] << "dir isn't a nullptr" << endl;
        return nullptr;
    }
    else if (!(dir->TYPE & 0x1))
    {
        kout[red] << dir->name << "is not a director" << endl;
        return nullptr;
    }
    uint32 len = strlen(fileName);
    uint64 tclus;
    unsigned char* buf = new unsigned char[Dbr.clus_size];
    uint64 rclus = dir->clus;
    get_clus(rclus, buf);

    FATtable* ft = (FATtable*)buf;

    int n = len / 13 + ((len % 13) ? 1 : 0);
    int re;
    // 创建目录项不会跨簇创建
    while (rclus)
    {
        // kout[red]<<rclus<<endl;
        for (int i = 0; i < Dbr.clus_size / 32; i++)
        {
            int k = 0;
            while (ft[i].attribute == 0 || ft[i].attribute == 0xe5)
            {
                k++;
                i++;
                if (k == n + 1)
                {

                    toshortname(fileName, (char*)&temp);
                    // 设置时间
                    temp.high_clus = 0;
                    temp.low_clus = 0;
                    temp.type = type;
                    re = i;
                    ft[i--] = temp;
                    char* p = fileName;
                    for (int j = 0; j < n; j++)
                    {
                        memset(&temp, 0, 32);
                        temp.type = 0x0f;
                        temp.attribute &= 0xf0;
                        if (j == n - 1)
                        {
                            temp.attribute |= 0x40;
                        }

                        temp.attribute |= (j + 1 & 0xf);
                        p = unicodecopy((int8*)temp.lname0, p, 5);
                        p = unicodecopy((int8*)temp.lname1, p, 6);
                        p = unicodecopy((int8*)temp.lname2, p, 2);
                        ft[i--] = temp;
                        // kout.memory(&temp, 32);
                    }
                    set_clus(rclus, buf);
                    delete[] buf;
                    FAT32FILE* rfile = new FAT32FILE(ft[re], fileName, rclus, re);
                    rfile->TYPE = 0;
                    return rfile;
                }
            }
        }

        tclus = rclus;
        rclus = get_next_clus(rclus);
        if (rclus < 0xffffff8)
            get_clus(rclus, buf);
        else
        {
            rclus = find_empty_clus();
            set_next_clus(tclus, rclus);
            get_clus(rclus, buf);
        }
    }

    delete[] buf;
    return nullptr;
}

FAT32FILE* FAT32::create_dir(FAT32FILE* dir, char* fileName)
{
    FAT32FILE* re;
    re = create_file(dir, fileName, FATtable::SUBDIRENCTORY);
    uint64 rclus = find_empty_clus();
    re->table.high_clus |= (rclus >> 16) & 0xff;
    re->table.low_clus |= rclus & 0xff;
    re->clus = rclus;
    set_table(re); // 更新目录项

    // 设置.文件
    uint8* clus = new uint8[Dbr.clus_size];
    get_clus(re->clus, clus);
    FATtable* ft;
    ft = (FATtable*)clus;
    FATtable temp;

    temp = re->table;
    strcpy_no_end(temp.name, ".          ");
    ft[0] = temp;

    temp = re->table;
    strcpy_no_end(temp.name, "..         ");
    temp.low_clus = re->table_clus_pos & 0xffff;
    temp.high_clus = (re->table_clus_pos >> 16) & 0xffff;
    ft[1] = temp;

    set_clus(re->clus, clus);

    delete[] clus;
    return re;
}

FAT32FILE* FAT32::open(char* path)
{
    return find_file_by_path(path);
}

bool FAT32::close(FAT32FILE* p)
{
    return 0;
}

int64 FAT32FILE::read(unsigned char* buf, uint64 size)
{
    if (size == 0)
    {
        kout[yellow] << "There is nothing" << endl;
        return 0;
    }
    if (size > table.size)
    {
        size = table.size;
    }

    int n = size / fat->Dbr.clus_size + ((size % fat->Dbr.clus_size) ? 1 : 0);
    // kout << n << endl;
    unsigned char* p = new unsigned char[fat->Dbr.sector_size];
    uint64 rclus = clus;
    for (int i = 0; i < n; i++)
    {
        if (rclus < 2 || rclus >= 0xffffff7)
            return -1;

        fat->get_clus(rclus, &buf[i * fat->Dbr.clus_size]);
        rclus = fat->get_next_clus(rclus);
    }
    buf[size] = 0;
    delete[] p;
    return size;
}

int64 FAT32FILE::read(unsigned char* buf, uint64 pos, uint64 size)
{
    if (size == 0 && pos + size > table.size)
    {
        kout[yellow] << "There is nothing" << endl;
        return 0;
    }
    if (pos + size > table.size)
    {
        size = table.size - pos;
    }
    uint64 start = pos / fat->Dbr.clus_size;
    uint64 end = (pos + size) / fat->Dbr.clus_size + (((pos + size) % fat->Dbr.clus_size) ? 1 : 0);
    uint64 rclus = clus;
    // kout << start << ' ' << end << endl;
    unsigned char* p = new unsigned char[(end - start) * fat->Dbr.clus_size];

    int i = 0;
    while (i < start)
    {
        if (rclus < 2 || rclus >= 0xffffff7)
            return -1;
        rclus = fat->get_next_clus(rclus);
        i++;
    }
    // kout << i << endl;
    for (int i = 0; i < end - start; i++)
    {
        if (rclus < 2 || rclus >= 0xffffff7)
            return -1;

        fat->get_clus(rclus, &p[i * fat->Dbr.clus_size]);
        // kout.memory(&p[i * fat->Dbr.clus_size], fat->Dbr.clus_size);
        rclus = fat->get_next_clus(rclus);
    }
    // kout[red] << "---------------------------------" << endl;

    pos -= start * fat->Dbr.clus_size;

    for (int i = 0; i < size; i++)
    {
        buf[i] = p[pos + i];
    }

    delete[] p;
    return size;
}

uint64 FAT32::find_empty_clus()
{
    uint32* p = (uint32*)new unsigned char[Dbr.sector_size];

    for (int i = 0; i < Dbr.FAT_sector_num; i++)
    {
        dev.read(FAT1lba + i, (uint8*)p);
        for (int j = 0; j < sector_size / 4; j++)
        {
            // kout[red]<<p[j]<<' ';

            if (p[j] == 0)
                return i * Dbr.sector_size / 4 + j;
        }
    }

    delete[] p;
    return 0xfffffff;
}

bool FAT32::del_file(FAT32FILE* file)
{
    FATtable* p = (FATtable*)new unsigned char[Dbr.clus_size];
    get_clus(file->table_clus_pos, (unsigned char*)p);

    if ((file->TYPE & FAT32FILE::__DIR) && !(file->TYPE & FAT32FILE::__SPECICAL))
    {
        FAT32FILE* t;
        t = get_next_file(file);
        // t->show();
        while (t)
        {
            del_file(t);
            t = get_next_file(file, t, EXCEPTDOT);
        }
    }

    uint64 k = file->table_clus_off;
    p[k].type1 = 0x0f;
    while (p[k].attribute != 0xe5 && p[k].type1 == 0x0f)
    {
        p[k].attribute = 0xe5;
        k--;
    }
    set_clus(file->table_clus_pos, (unsigned char*)p);

    uint64 rclus = file->clus;
    uint64 nxt;

    if (rclus >= 2)
    {
        while (rclus < 0xffffff7)
        {
            nxt = get_next_clus(rclus);
            set_next_clus(rclus, 0);
            rclus = nxt;
        }
    }

    set_next_clus(rclus, 0);

    delete[] p;
}

bool FAT32FILE::write(unsigned char* src, uint64 size)
{
    if (TYPE & 0x1)
    {
        // kout[red] << "this is a director" << endl;
        return false;
    }

    uint64 rclus;

    if (clus >= 2 && clus < 0xffffff8)
    {
        rclus = fat->get_next_clus(clus);
    }
    else
    {
        clus = fat->find_empty_clus();
        rclus = 0xffffff8;
    }
    // kout[green] << clus << endl;

    uint64 nxt;

    while (rclus < 0xffffff8 && rclus >= 2)
    {
        nxt = fat->get_next_clus(rclus);
        fat->set_next_clus(rclus, 0);
        rclus = nxt;
    }

    fat->set_clus(clus, src);

    rclus = clus;

    if (size > fat->Dbr.clus_size)
    {
        for (int i = 1; i < size / fat->Dbr.clus_size + (size % fat->Dbr.clus_size ? 1 : 0); i++)
        {
            nxt = fat->find_empty_clus();
            fat->set_next_clus(rclus, nxt);
            rclus = nxt;
            fat->set_clus(rclus, &src[fat->Dbr.clus_size * i]);
        }
    }
    fat->set_next_clus(rclus, 0xfffffff);

    table.size = size;
    // 设置时间
    fat->set_table(this);

    return true;
}

// uint64 FAT32::clus_off_in_fat(uint64 clus)
// {
//     return clus % (sector_size / 4);
// }
// uint64 FAT32::clus_sector_in_fat(uint64 clus)
// {
//     return clus / (sector_size / 4);
// }

void FAT32FILE::show()
{

    kout << "name " << name << endl
        << "TYPE " << TYPE << endl;
    if (path)
        kout << "PATH " << path << endl;
    kout << "SIZE " << table.size << endl
        << "CLUS " << clus << endl
        << "POS  " << table_clus_pos << endl
        << "OFF  " << table_clus_off << endl
        << endl;
    // <<"Creation Time"<<1980+(table.S_date[1]&0xfe)<<'/'
}

uint32 FAT32::get_next_clus(uint32 clus)
{
    uint32* temp = new uint32[Dbr.clus_size / 4];
    //
    dev.read(FAT1lba + clus * 4 / sector_size, (unsigned char*)temp);

    uint32 t = temp[clus % (sector_size / 4)];
    delete[] temp;
    return t;
}

bool FAT32::set_next_clus(uint32 clus, uint32 nxt_clus)
{
    unsigned char* temp = new unsigned char[Dbr.clus_size];
    if (clus > Dbr.FAT_sector_num / Dbr.clus_sector_num)
    {
        return false;
    }

    dev.read(FAT1lba + clus * 4 / sector_size, temp);
    uint32* t = (uint32*)temp;
    t[clus % (sector_size / 4)] = nxt_clus;
    delete[] temp;

    return true;
}

bool FAT32::set_clus(uint64 clus, unsigned char* buf)
{
    uint64 lba = clus_to_lba(clus);
    for (int i = 0; i < Dbr.clus_sector_num; i++)
    {
        dev.write(lba + i, &buf[sector_size * i]);
    }

    return true;
}

FAT32::~FAT32()
{
    unsigned char* temp = new uint8[Dbr.clus_size];
    if (Dbr.FAT_num > 1)
    {
        for (int i = 0; i < Dbr.FAT_sector_num; i++) // 备份fat
        {
            dev.read(FAT1lba + i, temp);
            dev.write(FAT2lba + i, temp);
        }
    }

    delete[] temp;
}

FAT32FILE* FAT32::get_next_file(FAT32FILE* dir, FAT32FILE* cur, bool (*p)(FATtable* temp))
{
    if (!(dir->TYPE & FAT32FILE::__DIR))
    {
        kout[red] << dir->name << "isn't a dir" << endl;
        return nullptr;
    }
    unsigned char* clus = new unsigned char[Dbr.clus_size];
    uint64 src_clus;
    if (cur)
        src_clus = cur->table_clus_pos;
    else
        src_clus = dir->clus;

    get_clus(src_clus, clus);
    FATtable* ft = (FATtable*)clus;
    FAT32FILE* re = nullptr;

    int32 t;
    int32 k;
    char* sName = new char[30];
    sName[26] = 0;
    sName[27] = 0;
    char* lName = new char[100];
    uint64 pos = cur ? cur->table_clus_off + 1 : 0; // 当没传入cur时从文件初开始遍历；

    while (src_clus < 0xffffff8)
    {
        for (int i = pos; i < Dbr.clus_size / 32; i++)
        {
            while ((t = ft[i].get_name(sName)) > 0)
            {
                unicode_to_ascii(sName);
                if (t & 0x40)
                {
                    // kout[red]<<"yes"<<endl;
                    strcpy(&lName[((t & 0xf) - 1) * 13], sName);
                }
                else
                    strcpy_no_end(&lName[((t & 0xf) - 1) * 13], sName);
                i++;
            }
            if (t == -1)
                continue;
            if (t == -2)
            {
                if (ft[i].name[1] == '.')
                    strcpy(lName, "..");
                else
                    strcpy(lName, ".");
            }

            // if (strcmp("mnt",lName)==0)
            // kout[red]<<"find!!!";
            //     strcpy(lName, sName);

            // kout[green] << lName << endl;

            if (p(&ft[i]))
            {
                re = new FAT32FILE(ft[i], lName, src_clus, i);
                // kout<<lName<<' '<<Hex(clus_to_lba(src_clus)*512);
                if (t == -2)
                    re->TYPE |= FAT32FILE::__SPECICAL;

                delete[] sName;
                delete[] lName;
                delete[] clus;
                return re;
            }
        }
        pos = 0;
        src_clus = get_next_clus(src_clus);
        if (src_clus < 0xffffff8)
            get_clus(src_clus, clus);
    }

    delete[] sName;
    delete[] clus;
    delete[] lName;
    return re;
}

bool FAT32::set_table(FAT32FILE* file)
{
    uint8* temp = new uint8[Dbr.clus_size];
    FATtable* ft;
    get_clus(file->table_clus_pos, temp);
    ft = (FATtable*)temp;
    memcpy(&ft[file->table_clus_off], (char*)&(file->table), 32);
    set_clus(file->table_clus_pos, temp);
    delete[] temp;

    return true;
}

bool ALLTURE(FATtable* t)
{
    return true;
}

bool VALID(FATtable* t)
{
    return t->attribute != 0xe5;
}

bool EXCEPTDOT(FATtable* t)
{
    return t->attribute != 0x2e;
}
