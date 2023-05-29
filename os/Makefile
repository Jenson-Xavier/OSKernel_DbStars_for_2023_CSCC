all: Build/test.img Build/test2.img Build/Kernel/kernel.elf 

#Kernel部分:

Build/Kernel/kernel.elf :Build/Kernel/main.elf Build/Kernel/start.elf Build/Kernel/kout.elf \
Build/Kernel/kstring.elf Build/Kernel/trap.elf Build/Kernel/trapEntry.elf Build/Kernel/clock.elf \
Build/Kernel/interrupt.elf Build/Kernel/pmm.elf Build/Kernel/vmm.elf Build/Kernel/process.elf \
Build/Kernel/syscall.elf Build/Kernel/Process_S.elf Build/Kernel/spinlock.elf Build/Kernel/mutex.elf \
Build/Kernel/semaphore.elf Build/Kernel/pathtool.elf Build/Kernel/ramdisk_driver.elf \
Build/Kernel/FAT32.elf \
#Build/Kernel/vfs.elf Build/Kernel/fat32.elf
	riscv64-unknown-elf-ld -o Build/Kernel/kernel.elf -T Linker/linker.ld \
	Build/Kernel/main.elf \
	Build/Kernel/start.elf \
	Build/Kernel/kout.elf \
	Build/Kernel/kstring.elf \
	Build/Kernel/trap.elf \
	Build/Kernel/trapEntry.elf \
	Build/Kernel/clock.elf \
	Build/Kernel/interrupt.elf \
	Build/Kernel/syscall.elf \
	Build/Kernel/pmm.elf \
	Build/Kernel/vmm.elf \
	Build/Kernel/ramdisk_driver.elf \
	Build/Kernel/FAT32.elf \
	Build/Kernel/process.elf \
	Build/Kernel/Process_S.elf \
	Build/Kernel/spinlock.elf \
	Build/Kernel/mutex.elf \
	Build/Kernel/pathtool.elf \
	Build/Kernel/semaphore.elf --format=binary Build/*.img --format=default

#Build/Kernel/vfs.elf \    
#Build/Kernel/fat32.elf \  //暂时停止

# Build/Kernel/kernel.elf :Build/Kernel/*.elf
# 	riscv64-unknown-elf-ld -o Build/Kernel/kernel.elf -T Linker/linker.ld Build/Kernel/*.elf

#kernel
Build/Kernel/start.elf:kernel/boot/start.S
	riscv64-unknown-elf-g++ -nostdlib -c kernel/boot/start.S -o Build/Kernel/start.elf -I"kernel/include" -mcmodel=medany -fno-exceptions -fno-rtti

Build/Kernel/main.elf:kernel/boot/main.cpp
	riscv64-unknown-elf-g++ -nostdlib -c kernel/boot/main.cpp -o Build/Kernel/main.elf -I"kernel/include" -mcmodel=medany -fno-exceptions -fno-rtti

#lib
Build/Kernel/kout.elf:kernel/lib/kout.cpp
	riscv64-unknown-elf-g++ -nostdlib -c kernel/lib/kout.cpp -o Build/Kernel/kout.elf -I"kernel/include" -mcmodel=medany -fno-exceptions -fno-rtti

Build/Kernel/kstring.elf:kernel/lib/kstring.cpp
	riscv64-unknown-elf-g++ -nostdlib -c kernel/lib/kstring.cpp -o Build/Kernel/kstring.elf -I"kernel/include" -mcmodel=medany -fno-exceptions -fno-rtti

#trap
Build/Kernel/trap.elf:kernel/trap/trap.cpp
	riscv64-unknown-elf-g++ -nostdlib -c kernel/trap/trap.cpp -o Build/Kernel/trap.elf -I"kernel/include" -mcmodel=medany -fno-exceptions -fno-rtti

Build/Kernel/trapEntry.elf:kernel/trap/trapEntry.S
	riscv64-unknown-elf-g++ -nostdlib -c kernel/trap/trapEntry.S -o Build/Kernel/trapEntry.elf -I"kernel/include" -mcmodel=medany -fno-exceptions -fno-rtti

Build/Kernel/clock.elf:kernel/trap/clock.cpp
	riscv64-unknown-elf-g++ -nostdlib -c kernel/trap/clock.cpp -o Build/Kernel/clock.elf -I"kernel/include" -mcmodel=medany -fno-exceptions -fno-rtti

Build/Kernel/interrupt.elf:kernel/trap/interrupt.cpp
	riscv64-unknown-elf-g++ -nostdlib -c kernel/trap/interrupt.cpp -o Build/Kernel/interrupt.elf -I"kernel/include" -mcmodel=medany -fno-exceptions -fno-rtti

Build/Kernel/syscall.elf:kernel/trap/syscall.cpp
	riscv64-unknown-elf-g++ -nostdlib -c kernel/trap/syscall.cpp -o Build/Kernel/syscall.elf -I"kernel/include" -mcmodel=medany -fno-exceptions -fno-rtti

#memory
Build/Kernel/pmm.elf:kernel/memory/pmm.cpp
	riscv64-unknown-elf-g++ -nostdlib -c kernel/memory/pmm.cpp -o Build/Kernel/pmm.elf -I"kernel/include" -mcmodel=medany -fno-exceptions -fno-rtti

Build/Kernel/vmm.elf:kernel/memory/vmm.cpp
	riscv64-unknown-elf-g++ -nostdlib -c kernel/memory/vmm.cpp -o Build/Kernel/vmm.elf -I"kernel/include" -mcmodel=medany -fno-exceptions -fno-rtti

#process
Build/Kernel/process.elf:kernel/process/process.cpp
	riscv64-unknown-elf-g++ -nostdlib -c kernel/process/process.cpp -o Build/Kernel/process.elf -I"kernel/include" -mcmodel=medany -fno-exceptions -fno-rtti

Build/Kernel/Process_S.elf:kernel/process/Process.S
	riscv64-unknown-elf-g++ -nostdlib -c kernel/process/Process.S -o Build/Kernel/Process_S.elf -I"kernel/include" -mcmodel=medany -fno-exceptions -fno-rtti

#file
Build/Kernel/vfs.elf:kernel/file/vfs.cpp
	riscv64-unknown-elf-g++ -nostdlib -c kernel/file/vfs.cpp -o Build/Kernel/vfs.elf -I"kernel/include" -mcmodel=medany -fno-exceptions -fno-rtti

Build/Kernel/pathtool.elf:kernel/file/pathtool.cpp
	riscv64-unknown-elf-g++ -nostdlib -c kernel/file/pathtool.cpp -o Build/Kernel/pathtool.elf -I"kernel/include" -mcmodel=medany -fno-exceptions -fno-rtti

# Build/Kernel/fat32.elf:kernel/file/fat32.cpp
# riscv64-unknown-elf-g++ -nostdlib -c kernel/file/fat32.cpp -o Build/Kernel/fat32.elf -I"kernel/include" -mcmodel=medany -fno-exceptions -fno-rtti

Build/Kernel/FAT32.elf:kernel/file/FAT32.cpp
	riscv64-unknown-elf-g++ -nostdlib -c kernel/file/FAT32.cpp -o Build/Kernel/FAT32.elf -I"kernel/include" -mcmodel=medany -fno-exceptions -fno-rtti

#driver
Build/Kernel/ramdisk_driver.elf:kernel/driver/ramdisk_driver.cpp
	riscv64-unknown-elf-g++ -nostdlib -c kernel/driver/ramdisk_driver.cpp -o Build/Kernel/ramdisk_driver.elf -I"kernel/include" -mcmodel=medany -fno-exceptions -fno-rtti

#synchronize
Build/Kernel/spinlock.elf:kernel/synchronize/spinlock.cpp
	riscv64-unknown-elf-g++ -nostdlib -c kernel/synchronize/spinlock.cpp -o Build/Kernel/spinlock.elf -I"kernel/include" -mcmodel=medany -fno-exceptions -fno-rtti

Build/Kernel/mutex.elf:kernel/synchronize/mutex.cpp
	riscv64-unknown-elf-g++ -nostdlib -c kernel/synchronize/mutex.cpp -o Build/Kernel/mutex.elf -I"kernel/include" -mcmodel=medany -fno-exceptions -fno-rtti

Build/Kernel/semaphore.elf:kernel/synchronize/semaphore.cpp
	riscv64-unknown-elf-g++ -nostdlib -c kernel/synchronize/semaphore.cpp -o Build/Kernel/semaphore.elf -I"kernel/include" -mcmodel=medany -fno-exceptions -fno-rtti

#User部分:(主要是为了嵌入内核用户映像进行还没有文件系统下的用户进程的测试)

#library
Build/User/usermain.o:user/library/usermain.cpp
	riscv64-unknown-elf-g++ -nostdlib -c user/library/usermain.cpp -o Build/User/usermain.o -I"kernel/include" -I"user/library" -mcmodel=medany -fno-exceptions -fno-rtti

Build/User/userstart.o:user/library/userstart.S
	riscv64-unknown-elf-g++ -nostdlib -c user/library/userstart.S -o Build/User/userstart.o	-I"kernel/include" -I"user/library" -mcmodel=medany -fno-exceptions -fno-rtti

Build/User/clone.o:user/library/clone.S
	riscv64-unknown-elf-g++ -nostdlib -c user/library/clone.S -o Build/User/clone.o	-I"kernel/include" -I"user/library" -mcmodel=medany

#user test.cpp
Build/User/test.o:user/test.cpp
	riscv64-unknown-elf-g++ -nostdlib -c user/test.cpp -o Build/User/test.o -I"kernel/include" -I"user/library" -mcmodel=medany

Build/User/test.elf:Build/User/test.o Build/User/usermain.o Build/User/userstart.o
	riscv64-unknown-elf-ld -o Build/User/test.elf -T Linker/user.ld Build/User/test.o Build/User/usermain.o Build/User/userstart.o

Build/test.img:Build/User/test.elf
	riscv64-unknown-elf-objcopy Build/User/test.elf --strip-all -O binary Build/test.img

#user test2.cpp
Build/User/test2.o:user/test2.cpp user/
	riscv64-unknown-elf-g++ -nostdlib -c user/test2.cpp -o Build/User/test2.o -I"kernel/include" -I"user/library" -mcmodel=medany

Build/User/test2.elf:Build/User/test2.o Build/User/usermain.o Build/User/userstart.o Build/User/clone.o
	riscv64-unknown-elf-ld -o Build/User/test2.elf -T Linker/user.ld Build/User/clone.o Build/User/test2.o Build/User/usermain.o Build/User/userstart.o 

Build/test2.img:Build/User/test2.elf
	riscv64-unknown-elf-objcopy Build/User/test2.elf --strip-all -O binary Build/test2.img

clean:
	rm Build/* Build/Kernel/* Build/User/*

clean1:
	rm Build/Kernel/kernel.elf

qemu:
	cp img/udisk.img Build/udisk.img
	qemu-system-riscv64 -machine virt -m 256M -nographic -bios SBI/opensbi-qemu.elf -kernel Build/Kernel/kernel.elf -initrd Build/udisk.img
