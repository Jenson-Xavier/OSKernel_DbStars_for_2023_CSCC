#include <FAT32.hpp>

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

        return attribute & 0xf;
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

    kout[yellow] << "Dbr.sector_size     " << Dbr.sector_size << endl;
    kout[yellow] << "Dbr.FAT_num         " << Dbr.FAT_num << endl;
    kout[yellow] << "Dbr.FAT_sector_num  " << Dbr.FAT_sector_num << endl;
    kout[yellow] << "DAtAlba             " << DATAlba << endl;
    dev.read(DBRlba, buf);
    for (int i = 0; i < 16; i++)
        kout[green] << buf[i] << ' ';
    kout[green] << endl;

    kout[yellow] << "FAT1lba             " << FAT1lba << endl;
    dev.read(FAT1lba, buf);
    for (int i = 0; i < 16; i++)
        kout[green] << buf[i] << ' ';
    kout[green] << endl;

    kout[yellow] << "FAT2lba             " << FAT2lba << endl;
    dev.read(FAT2lba, buf);
    for (int i = 0; i < 16; i++)
        kout[green] << buf[i] << ' ';
    kout[green] << endl;

    kout[yellow] << "DATAlba             " << DATAlba << endl;
    dev.read(DATAlba, buf);
    for (int i = 0; i < 16; i++)
        kout[green] << (char)buf[i] << ' ';
    kout[green] << endl;

    temp = new unsigned char[Dbr.clus_size];

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
    for (int i = 0; i < Dbr.clus_sector_num; i++)
    {
        dev.read(lba + i, buf + i * Dbr.sector_size);
    }

    return true;
}

FAT32FILE* FAT32::get_child_form_clus(char* child_name, uint64 src_lba)
{
    // kout[blue]<<child_name<<' '<<src_lba<<endl;
    unsigned char* clus = new unsigned char[Dbr.clus_size];
    for (int i = 0; i < Dbr.clus_sector_num; i++)
    {
        dev.read(src_lba + i, &clus[Dbr.sector_size * i]);
    }

    FATtable* ft = (FATtable*)clus;
    FAT32FILE* re = nullptr;

    int32 t;
    char* sName = new char[30];
    sName[26] = 0;
    sName[27] = 0;
    char* lName = new char[100];

    // for (int i = 0; i < 10; i++)
    for (int i = 0; i < Dbr.clus_size / 16; i++)
    {
        while ((t = ft[i].get_name(sName)) > 0)
        {
            unicode_to_ascii(sName);
            strcpy_no_end(&lName[(t - 1) * 13], sName);
            i++;
        }
        if (t == -1)
            continue;

        // if (sName[6] != '~' || !isdigit(sName[7]))
        //     strcpy(lName, sName);

        // kout[green] << lName << endl;

        if (strcmp(lName, child_name) == 0)
        {
            re = new FAT32FILE;
            memcpy(&(re->table), (char*)&ft[i], 32);
            re->clus = re->table.low_clus | (re->table.high_clus << 16);

            re->TYPE |= re->table.type & 0x10 ? 1 : 0;
            re->name = lName;
            re->table_clus_pos = lba_to_clus(src_lba);
            re->table_clus_off = i;
            delete[] sName;
            delete[] clus;
            return re;
        }
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
    uint64 lba = DATAlba;
    FAT32FILE* tb;

    FATtable* ft;
    while (path = split_path_name(path, sigleName))
    {
        kout[green] << sigleName << endl;
        tb = get_child_form_clus(sigleName, lba);
        if (tb == nullptr)
        {
            kout[red] << "can't find file:" << sigleName << endl;
            return nullptr;
        }
        if (path[0])
        {
            // kout[yellow] << tb->name << endl;
            if (tb->table.type & 0x10) // 目录
            {
                // kout[yellow]<<"dir"<<endl;
                lba = clus_to_lba(tb->table.low_clus | (uint64)tb->table.high_clus << 16);
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
    delete[] sigleName;
    return tb;
}

bool FAT32::create_file(char* path, char* fileName)
{
    FAT32FILE* p;
    p = find_file_by_path(path);
    if (p == nullptr)
    {
        kout[red] << "can't find dir :" << path << endl;
        return false;
    }
    else if (p->TYPE & 0x1)
    {
        kout[red] << path << "is not a director" << endl;
        return false;
    }
    if (fileName)
    {
    }
}

FAT32FILE* FAT32::open(char* path)
{
    return find_file_by_path(path);
}

bool FAT32::close(FAT32FILE* p)
{
    return 0;
}

int FAT32::read(FAT32FILE* src, unsigned char* buf, uint64 size)
{

    int n = size / Dbr.clus_size;
    uint64 clus = src->clus;
    unsigned char* p = new unsigned char[Dbr.sector_size];
    for (int i = 0; i <= n; i++)
    {
        get_clus(clus, &buf[i * Dbr.clus_size]);
        clus = get_next_clus(clus);
    }
    buf[size] = 0;
    delete[] p;
    return n;
}

uint64 FAT32::find_empty_clus()
{
    uint32* p = (uint32*)new unsigned char[Dbr.sector_size];
    for (int i = 0; i < Dbr.FAT_sector_num; i++)
    {
        dev.read(FAT1lba + i, (uint8*)p);
        for (int j = 0; j < sector_size / 4; j++)
        {
            if (p[i] == 0)
                return i * Dbr.sector_size + j;
        }
    }

    delete[] p;
    return 0xfffffff;
}

bool FAT32::del_file(FAT32FILE* file)
{
    unsigned char* p = new unsigned char[Dbr.sector_size];
    uint64 sec = clus_to_lba(file->table_clus_pos) + file->table_clus_off * 32 / sector_size;

    while (sec >= DATAlba)
    {
        dev.read(sec, p);
        p[(file->table_clus_off * 32) % Dbr.sector_size] = 0xe5;
    }
    delete[] p;
}
bool FAT32::write(FAT32FILE* dst, unsigned char* src, uint64 size)
{
    if (dst->TYPE & 0x1)
    {
        kout[red] << "this is a director" << endl;
        return false;
    }
    uint64 clus = get_next_clus(dst->clus);
    uint64 nxt;
    while (clus < 0xffffff7)
    {
        nxt = get_next_clus(clus);
        set_next_clus(clus, 0);
        clus = nxt;
    }

    set_clus(dst->clus, src);
    clus = dst->clus;

    if (size > Dbr.clus_size)
    {
        for (int i = 1; i < size / Dbr.clus_size + (size % Dbr.clus_size ? 1 : 0); i++)
        {
            nxt = find_empty_clus();
            set_next_clus(clus, nxt);
            clus = nxt;
            set_clus(clus, &src[Dbr.clus_size * i]);
        }
    }
    set_next_clus(clus, 0xfffffff);

    dst->table.size = size;
    //设置时间
    set_table(dst);

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
        << "TYPE " << TYPE << endl
        << "SIZE " << table.size << endl
        << "CLUS " << clus << endl
        << "POS  " << table_clus_pos << endl
        << "OFF  " << table_clus_off << endl
        << endl;
    // <<"Creation Time"<<1980+(table.S_date[1]&0xfe)<<'/'
}

uint32 FAT32::get_next_clus(uint32 clus)
{

    dev.read(FAT1lba + clus * 4 / sector_size, temp);
    uint32* t = (uint32*)temp;
    return t[clus % (sector_size / 4)];
}

bool FAT32::set_next_clus(uint32 clus, uint32 nxt_clus)
{
    if (clus > Dbr.FAT_sector_num / Dbr.clus_sector_num)
    {
        return false;
    }

    dev.read(FAT1lba + clus * 4 / sector_size, temp);
    uint32* t = (uint32*)temp;
    t[clus % (sector_size / 4)] = nxt_clus;

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

bool FAT32::set_table(FAT32FILE* file)
{
    FATtable* ft;
    get_clus(file->table_clus_pos, temp);
    ft = (FATtable*)temp;
    memcpy(&ft[file->table_clus_off], (char*)&(file->table), 32);
    set_clus(file->table_clus_pos, temp);

    return true;
}