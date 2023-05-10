Build/kernel.elf :	Build/main.elf Build/start.elf Build/kout.elf Build/kstring.elf Build/trap.elf Build/trapEntry.elf Build/clock.elf Build/interrupt.elf Build/pmm.elf Build/vmm.elf Build/process.elf Build/Process.elf Build/spinlock.elf Build/mutex.elf Build/semaphore.elf
	riscv64-unknown-elf-ld -o Build/kernel.elf -T Linker/linker.ld \
	Build/main.elf \
	Build/start.elf \
	Build/kout.elf \
	Build/kstring.elf \
	Build/trap.elf \
	Build/trapEntry.elf \
	Build/clock.elf \
	Build/interrupt.elf \
	Build/pmm.elf \
	Build/vmm.elf \
	Build/process.elf \
	Build/Process.elf \
	Build/spinlock.elf \
	Build/mutex.elf \
	Build/semaphore.elf

#kernel
Build/start.elf:kernel/boot/start.S
	riscv64-unknown-elf-g++ -nostdlib -c kernel/boot/start.S -o Build/start.elf -I"kernel/include" -mcmodel=medany

Build/main.elf:kernel/boot/main.cpp
	riscv64-unknown-elf-g++ -nostdlib -c kernel/boot/main.cpp -o Build/main.elf -I"kernel/include" -mcmodel=medany

Build/kout.elf:kernel/lib/kout.cpp
	riscv64-unknown-elf-g++ -nostdlib -c kernel/lib/kout.cpp -o Build/kout.elf -I"kernel/include" -mcmodel=medany

Build/kstring.elf:kernel/lib/kstring.cpp
	riscv64-unknown-elf-g++ -nostdlib -c kernel/lib/kstring.cpp -o Build/kstring.elf -I"kernel/include" -mcmodel=medany

#trap
Build/trap.elf:kernel/trap/trap.cpp
	riscv64-unknown-elf-g++ -nostdlib -c kernel/trap/trap.cpp -o Build/trap.elf -I"kernel/include" -mcmodel=medany

Build/trapEntry.elf:kernel/trap/trapEntry.S
	riscv64-unknown-elf-g++ -nostdlib -c kernel/trap/trapEntry.S -o Build/trapEntry.elf -I"kernel/include" -mcmodel=medany

Build/clock.elf:kernel/trap/clock.cpp
	riscv64-unknown-elf-g++ -nostdlib -c kernel/trap/clock.cpp -o Build/clock.elf -I"kernel/include" -mcmodel=medany

Build/interrupt.elf:kernel/trap/interrupt.cpp
	riscv64-unknown-elf-g++ -nostdlib -c kernel/trap/interrupt.cpp -o Build/interrupt.elf -I"kernel/include" -mcmodel=medany

#memory
Build/pmm.elf:kernel/memory/pmm.cpp
	riscv64-unknown-elf-g++ -nostdlib -c kernel/memory/pmm.cpp -o Build/pmm.elf -I"kernel/include" -mcmodel=medany

Build/vmm.elf:kernel/memory/vmm.cpp
	riscv64-unknown-elf-g++ -nostdlib -c kernel/memory/vmm.cpp -o Build/vmm.elf -I"kernel/include" -mcmodel=medany

#process
Build/process.elf:kernel/process/process.cpp
	riscv64-unknown-elf-g++ -nostdlib -c kernel/process/process.cpp -o Build/process.elf -I"kernel/include" -mcmodel=medany

Build/Process.elf:kernel/process/Process.S
	riscv64-unknown-elf-g++ -nostdlib -c kernel/process/Process.S -o Build/Process.elf -I"kernel/include" -mcmodel=medany

#synchronize
Build/spinlock.elf:kernel/synchronize/spinlock.cpp
	riscv64-unknown-elf-g++ -nostdlib -c kernel/synchronize/spinlock.cpp -o Build/spinlock.elf -I"kernel/include" -mcmodel=medany

Build/mutex.elf:kernel/synchronize/mutex.cpp
	riscv64-unknown-elf-g++ -nostdlib -c kernel/synchronize/mutex.cpp -o Build/mutex.elf -I"kernel/include" -mcmodel=medany

Build/semaphore.elf:kernel/synchronize/semaphore.cpp
	riscv64-unknown-elf-g++ -nostdlib -c kernel/synchronize/semaphore.cpp -o Build/semaphore.elf -I"kernel/include" -mcmodel=medany

clean:
	rm Build/*

qemu:
	qemu-system-riscv64 -machine virt -m 128M -nographic -bios SBI/opensbi-qemu.elf -kernel Build/kernel.elf
