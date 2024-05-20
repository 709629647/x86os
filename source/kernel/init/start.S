/**
 * 自己动手写操作系统
 *
 * 二级引导，负责进行硬件检测，进入保护模式，然后加载内核，并跳转至内核运行
 *
 * 作者：李述铜
 * 联系邮箱: 527676163@qq.com
 */
    #include "os_cfg.h"
  	// 不必加.code32因默认就是32位
 	.text
 	.global _start
	.extern kernel_init, init_main

_start:
    # 第一种方法
    # push %ebp
    # mov %esp, %ebp
    # mov 0x8(%ebp), %eax
    # push %eax

    # 第二种方法
    # mov 4(%esp), %eax
    # push %eax

    # 第三种方法
    push 4(%esp)

    # kernel_init(boot_info)
    call kernel_init

    	// 重新加载GDT
	jmp $KERNEL_SELECTOR_CS, $gdt_reload

gdt_reload:
	mov $KERNEL_SELECTOR_DS, %ax		// 16为数据段选择子
	mov %ax, %ds
    mov %ax, %ss
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

	// 栈设置
	mov $(stack + KERNEL_STACK_SIZE), %esp

	// 栈和段等沿用之前的设置
	jmp init_main

	.bss
.comm stack, KERNEL_STACK_SIZE 

.macro exception_handler name num with_err_num
    .text
    .global exception_handler_\name
    .extern do_handler_\name

exception_handler_\name:
    .if \with_err_num == 0
        push $0
    .endif

    push $\num
    pusha
    push %ds
    push %es
    push %fs
    push %gs
    push %esp

    call do_handler_\name

    pop %esp
    pop %gs
    pop %fs
    pop %es
    pop %ds
    popa

    add $(2*4), %esp
    iret

.endm

exception_handler unknown, -1, 0
exception_handler divider0, 0, 0
exception_handler Debug, 1, 0
exception_handler NMI, 2, 0
exception_handler breakpoint, 3, 0
exception_handler overflow, 4, 0
exception_handler bound_range, 5, 0
exception_handler invalid_opcode, 6, 0
exception_handler device_unavailable, 7, 0
exception_handler double_fault, 8, 1
exception_handler invalid_tss, 10, 1
exception_handler segment_not_present, 11, 1
exception_handler stack_segment_fault, 12, 1
exception_handler general_protection, 13, 1
exception_handler page_fault, 14, 1
exception_handler fpu_error, 16, 0
exception_handler alignment_check, 17, 1
exception_handler machine_check, 18, 0
exception_handler smd_exception, 19, 0
exception_handler virtual_exception, 20, 0
exception_handler time, 0x20, 0
exception_handler kbd, 0x21, 0
exception_handler ide_primary, 0x2E, 0
;     .text
;     .global simple_switch
;     //simple_switch(&from, to)
; simple_switch:
;     mov 8(%esp), %edx
;     mov 4(%esp), %eax
;     push %ebp
;     push %ebx
;     push %esi
;     push %edi
;     mov %esp, (%eax)

;     mov %edx, %esp
;     pop %edi
;     pop %esi
;     pop %ebx
;     pop %ebp

    .global exception_handler_syscall
    .extern do_handler_syscall
exception_handler_syscall:
    pusha
    push %ds
    push %es
    push %fs
    push %gs
    pushf
    mov $(KERNEL_SELECTOR_DS), %eax
	mov %eax, %ds
	mov %eax, %es
	mov %eax, %fs
	mov %eax, %gs
    mov %esp, %eax
    push %eax
    call do_handler_syscall
    add $4, %esp
    popf
    pop %gs
    pop %fs
    pop %es
    pop %ds
    popa
    retf $(5*4)