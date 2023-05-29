#include <fat32.hpp>

FAT32::FAT32()
{
    if(!this->init())
        kout[red]<<"FAT32 Init error"<<endl;

}


bool FAT32::init()
{
    fat32dev.init();
    unsigned char * buf=new unsigned char[sector_size];
    fat32dev.read(0,buf);
    if(buf[510]!=0x55||buf[511]!=0xaa)
        return false;
    DBPLba=0;
    kout[yellow]<<"DBPLba:"<<DBPLba<<endl;
    Dbr.BPB_sector_per_clus=buf[0x0d];
    Dbr.BPB_rs_sector_num=(buf[0x0f]<<8)|buf[0x0e];
    Dbr.BPB_fat_num=buf[0x10];
    Dbr.BPB_hidden_sector_num=buf[0x1c]|(buf[0x1d]<<8)|(buf[0x1e]<<16)|(buf[0x1f]<<24);
    Dbr.BPB_fat_sector_num=buf[0x24]|(buf[0x25]<<8)|(buf[0x26]<<16)|(buf[0x27]<<24);

    FAT1Lab=DBPLba+Dbr.BPB_rs_sector_num;
    FAT2Lab=DBPLba+Dbr.BPB_rs_sector_num+Dbr.BPB_fat_sector_num;
    RootLab=DBPLba+Dbr.BPB_rs_sector_num+Dbr.BPB_fat_num*Dbr.BPB_fat_sector_num;
    
    delete[] buf;
    return true;

}