	.text
	.global _first_task_entry
	.extern first_task_main 
//全局可见可在.c文件中引用
_first_task_entry:
	mov %ss, %ax
    mov %ax, %ds
	mov %ax, %es
	mov %ax, %fs
	mov %ax, %gs
	jmp  first_task_main