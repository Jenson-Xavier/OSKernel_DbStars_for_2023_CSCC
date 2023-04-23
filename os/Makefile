Build/kernal.elf :	Build/main.elf Build/start.elf Build/kout.elf Build/trap.elf Build/trapEntry.elf Build/clock.elf Build/interrupt.elf
	riscv64-unknown-elf-ld -o Build/kernal.elf -T Linker/linker.ld Build/main.elf Build/start.elf Build/kout.elf Build/trap.elf Build/trapEntry.elf Build/clock.elf Build/interrupt.elf

#kernal
Build/start.elf:Kernal/boot/start.S
	riscv64-unknown-elf-g++ -nostdlib -c Kernal/boot/start.S -o Build/start.elf -I"Kernal/include" -mcmodel=medany

Build/main.elf:Kernal/boot/main.cpp
	riscv64-unknown-elf-g++ -nostdlib -c Kernal/boot/main.cpp -o Build/main.elf -I"Kernal/include" -mcmodel=medany

Build/kout.elf:Kernal/lib/kout.cpp
	riscv64-unknown-elf-g++ -nostdlib -c Kernal/lib/kout.cpp -o Build/kout.elf -I"Kernal/include" -mcmodel=medany

#trap
Build/trap.elf:Kernal/trap/trap.cpp
	riscv64-unknown-elf-g++ -nostdlib -c Kernal/trap/trap.cpp -o Build/trap.elf -I"Kernal/include" -mcmodel=medany

Build/trapEntry.elf:Kernal/trap/trapEntry.S
	riscv64-unknown-elf-g++ -nostdlib -c Kernal/trap/trapEntry.S -o Build/trapEntry.elf -I"Kernal/include" -mcmodel=medany

Build/clock.elf:Kernal/trap/clock.cpp
	riscv64-unknown-elf-g++ -nostdlib -c Kernal/trap/clock.cpp -o Build/clock.elf -I"Kernal/include" -mcmodel=medany

Build/interrupt.elf:Kernal/trap/interrupt.cpp
	riscv64-unknown-elf-g++ -nostdlib -c Kernal/trap/interrupt.cpp -o Build/interrupt.elf -I"Kernal/include" -mcmodel=medany


clean:
	rm Build/*

qemu:
	qemu-system-riscv64 -machine virt -m 128M -nographic -bios SBI/opensbi-qemu.elf -kernel Build/kernal.elf
