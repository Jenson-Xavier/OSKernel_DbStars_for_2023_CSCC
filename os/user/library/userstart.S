# 给用户程序映像写的入口汇编文件
.section .text.entry

.global _start
_start:
    call _usermain

Loop:   # error Loop
    j Loop