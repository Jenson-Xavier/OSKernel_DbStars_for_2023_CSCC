#include <Riscv.h>

	# 修改trapEntry.S 为了实现用户态进程触发trap流程 实现 U 态和 S 态不同的特权管理
	# 进而实现特权级的切换
	# 关键是内核栈指针和用户栈指针的维护

	# 若在中断之前处于U态 则 sscratch 保存的是内核栈地址
	# 否则中断之前处于S态 sscratch 保存的是 0

	# 这里通过注释简单说明一下 sscratch CSR寄存器的角色
	# 为了能够执行内核态的中断处理流程 仅有一个入口地址是不够的 而且用户态的程序往往有两个栈指针
	# 一个是内核栈指针 还有一个是用户栈指针
	# 而中断处理流程是可能需要使用栈的 程序当前的用户栈是不安全的 需要预设安全的内核栈空间
	# 就存放在 sscratch 寄存器这里
	# 而在 S 内核态中 sp就是可以认为是安全的 故其值为 0
	# 从而 sscratch 扮演的角色就是在遇到中断时可以让机器判断是用户态 U 还是内核态 S
	# 那么需要对原来的trapEntry进行一些修改 主要是增加了几个分支跳转指令

	.altmacro								
	.align 2
	.macro SAVE_ALL
	LOCAL _restore_kernel_sp
	LOCAL _save_context

	csrrw sp,sscratch,sp				# 先交换 sp 和 sscratch 的内容

	# sp 为 0 则是内核态 那么上面交换的步骤就需要复原回来 拿回内核栈指针
	# sp 不为 0 则是用户态 sp 中就是内核栈指针 sscratch就是用户栈指针
	# 则设置对应的跳转分支指令

	bnez sp,_save_context
_restore_kernel_sp:
	csrr sp,sscratch					# 先前进行一次交换 这里再拿回内核栈指针
_save_context:
	addi sp, sp, -36 * REGBYTES
	STORE x0,  (0*REGBYTES)(sp)
	STORE x1,  (1*REGBYTES)(sp)
	STORE x3,  (3*REGBYTES)(sp)
	STORE x4,  (4*REGBYTES)(sp)
	STORE x5,  (5*REGBYTES)(sp)
	STORE x6,  (6*REGBYTES)(sp)
	STORE x7,  (7*REGBYTES)(sp)
	STORE x8,  (8*REGBYTES)(sp)
	STORE x9,  (9*REGBYTES)(sp)
	STORE x10, (10*REGBYTES)(sp)
	STORE x11, (11*REGBYTES)(sp)
	STORE x12, (12*REGBYTES)(sp)
	STORE x13, (13*REGBYTES)(sp)
	STORE x14, (14*REGBYTES)(sp)
	STORE x15, (15*REGBYTES)(sp)
	STORE x16, (16*REGBYTES)(sp)
	STORE x17, (17*REGBYTES)(sp)
	STORE x18, (18*REGBYTES)(sp)
	STORE x19, (19*REGBYTES)(sp)
	STORE x20, (20*REGBYTES)(sp)
	STORE x21, (21*REGBYTES)(sp)
	STORE x22, (22*REGBYTES)(sp)
	STORE x23, (23*REGBYTES)(sp)
	STORE x24, (24*REGBYTES)(sp)
	STORE x25, (25*REGBYTES)(sp)
	STORE x26, (26*REGBYTES)(sp)
	STORE x27, (27*REGBYTES)(sp)
	STORE x28, (28*REGBYTES)(sp)
	STORE x29, (29*REGBYTES)(sp)
	STORE x30, (30*REGBYTES)(sp)
	STORE x31, (31*REGBYTES)(sp)
	
	# 这里下面的执行 
	# 如果之前是用户态 那么用户栈指针从 sscratch 移动到了 s0 寄存器 并且 sscratch 清零

	csrrw s0, sscratch, x0
	csrr s1, sstatus
	csrr s2, sepc
	csrr s3, stval
	csrr s4, scause

	STORE s0, 2*REGBYTES(sp)			# 如果是用户态发生的中断 那么用户栈指针在这里被保存到内存中 栈空间上
	STORE s1, 32*REGBYTES(sp)
	STORE s2, 33*REGBYTES(sp)
	STORE s3, 34*REGBYTES(sp)
	STORE s4, 35*REGBYTES(sp)
	.endm

	.macro RESTORE_ALL
	LOCAL _save_kernel_sp				# 需要为恢复设计对应的 分支指令
	LOCAL _restore_context

	LOAD s1,  (32*REGBYTES)(sp)
	LOAD s2,  (33*REGBYTES)(sp)

	andi s0,s1,SSTATUS_SPP  			# s1 是 sstatus 寄存器 和对应的SPP相与宏判断特权级
	bnez s0,_restore_context			# s0 不为 0 则是内核态 为 0 则是用户态

_save_kernel_sp:
	addi s0,sp,36*REGBYTES				# 用户态需要提取回相应的内核栈指针
	csrw sscratch,s0					# 并重新保存到 sscratch 进行恢复
_restore_context:
	csrw sstatus, s1
	csrw sepc, s2

	LOAD x1,  (1*REGBYTES)(sp)
	LOAD x3,  (3*REGBYTES)(sp)
	LOAD x4,  (4*REGBYTES)(sp)
	LOAD x5,  (5*REGBYTES)(sp)
	LOAD x6,  (6*REGBYTES)(sp)
	LOAD x7,  (7*REGBYTES)(sp)
	LOAD x8,  (8*REGBYTES)(sp)
	LOAD x9,  (9*REGBYTES)(sp)
	LOAD x10, (10*REGBYTES)(sp)
	LOAD x11, (11*REGBYTES)(sp)
	LOAD x12, (12*REGBYTES)(sp)
	LOAD x13, (13*REGBYTES)(sp)
	LOAD x14, (14*REGBYTES)(sp)
	LOAD x15, (15*REGBYTES)(sp)
	LOAD x16, (16*REGBYTES)(sp)
	LOAD x17, (17*REGBYTES)(sp)
	LOAD x18, (18*REGBYTES)(sp)
	LOAD x19, (19*REGBYTES)(sp)
	LOAD x20, (20*REGBYTES)(sp)
	LOAD x21, (21*REGBYTES)(sp)
	LOAD x22, (22*REGBYTES)(sp)
	LOAD x23, (23*REGBYTES)(sp)
	LOAD x24, (24*REGBYTES)(sp)
	LOAD x25, (25*REGBYTES)(sp)
	LOAD x26, (26*REGBYTES)(sp)
	LOAD x27, (27*REGBYTES)(sp)
	LOAD x28, (28*REGBYTES)(sp)
	LOAD x29, (29*REGBYTES)(sp)
	LOAD x30, (30*REGBYTES)(sp)
	LOAD x31, (31*REGBYTES)(sp)
	LOAD x2,  (2*REGBYTES)(sp)
	.endm
	

trap_entry:
	.globl trap_entry
	SAVE_ALL
	move  a0, sp
	jal trap				# trap入口 RISCV统一的中断异常处理入口
	move  sp, a0			# trap的返回值TRAPFRAME指针放在a0 move到sp
	
TrapRet:
	.globl TrapRet
	RESTORE_ALL				# 从sp位置将保存的TRAPFRAME保存到相应的寄存器
	sret					# sret返回到正常的执行流 根据硬件设计逻辑 这一步会跳转到epc存放的指令地址执行