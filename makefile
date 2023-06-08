all:
	make all -C ./os
	cp ./os/Build/Kernel/kernel.elf kernel-qemu