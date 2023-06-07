#ifndef POS_DISK_HPP
#define POS_DISK_HPP

using Uint8 = unsigned char;

constexpr int SectorSize = 512;
struct Sector
{
	Uint8 data[SectorSize];

	inline Uint8& operator [] (unsigned pos)
	{
		return data[pos];
	}

	inline const Uint8& operator [] (int i) const
	{
		return data[i];
	}
};

bool DiskInit();
bool DiskReadSector(unsigned long long LBA, Sector* sec, int cnt = 1);
bool DiskWriteSector(unsigned long long LBA, const Sector* sec, int cnt = 1);
bool DiskInterruptSolve();

#endif
