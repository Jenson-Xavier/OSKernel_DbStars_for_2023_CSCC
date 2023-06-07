extern "C"
{
#include <DriverTools.h>

	void set_bit(volatile uint32* bits, uint32 mask, uint32 value)
	{
		uint32 org = (*bits) & ~mask;
		*bits = org | (value & mask);
	}

	void set_bit_offset(volatile uint32* bits, uint32 mask, uint64 offset, uint32 value)
	{
		set_bit(bits, mask << offset, value << offset);
	}

	void set_gpio_bit(volatile uint32* bits, uint64 offset, uint32 value)
	{
		set_bit_offset(bits, 1, offset, value);
	}

	uint32 get_bit(volatile uint32* bits, uint32 mask, uint64 offset)
	{
		return ((*bits) & (mask << offset)) >> offset;
	}

	uint32 get_gpio_bit(volatile uint32* bits, uint64 offset)
	{
		return get_bit(bits, 1, offset);
	}
};

#include <semaphore.hpp>

void* Kalloc()
{
	return pmm.malloc(4096 /*???*/);
}

void Kfree(void* addr)
{
	pmm.free(addr);
}


void Memmove(char* dst, char* src, uint64 count)
{
	if (dst == nullptr || src == nullptr || dst == src || count == 0)
		return;
	if (dst < src)
	{
		char* end = dst + count;
		while (dst != end)
			*dst++ = *src++;
	}
	else
	{
		char* start = dst;
		dst += count;
		src += count;
		do
			*--dst = *--src;
		while (dst != start);
	}
}

void* memmove(void* dst, const void* src, unsigned long long size)
{
	Memmove((char*)dst, (char*)src, size);
	return dst;
}

SEMAPHORE* dmacSem = nullptr;
void dmacSemWait()
{
	//	dmacSem->Wait();
}

void dmacSemSignal()
{
	//	dmacSem->Signal(Semaphore::SignalAll);
}

void dmacSemInit()
{
	dmacSem = new SEMAPHORE;
	dmacSem->init();
}

void DebugInfo(const char* str)
{
	kout[yellow] << str << endl;
}

void DebugInfoI(const char* str, unsigned long long x)
{
	kout[yellow] << str << x << endl;
}
