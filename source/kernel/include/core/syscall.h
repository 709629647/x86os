#ifndef SYSCALL_H
#define SYSCALL_H
#include "comm/types.h"
#define SYSCALL_PARAM       5
#define SYS_SLEEP           0
#define SYS_GETPID          1
#define SYS_FORK            2
#define SYS_EXECVE          3
#define SYS_YIELD           4
#define SYS_EXIT            5
#define SYS_WAIT            6
#define SYS_OPEN            50
#define SYS_READ            51
#define SYS_WRITE           52
#define SYS_LSEEK           53
#define SYS_CLOSE           54
#define SYS_FSTAT           55
#define SYS_ISATTY          56
#define SYS_SBRK            57
#define SYS_DUP             58
#define SYS_IOCTL            59
#define SYS_UNLINK          60
#define SYS_OPENDIR         61
#define SYS_READDIR         62
#define SYS_CLOSEDIR        63

#define SYS_PRINTMSG        100
typedef struct _syscall_frame_t{
    uint32_t eflags;
    uint32_t gs, fs, es, ds;
    uint32_t edi, esi, ebp, dummy, ebx, edx, ecx, eax;
    uint32_t eip, cs;
    uint32_t id, arg0, arg1, arg2, arg3;
    uint32_t esp, ss;
}syscall_frame_t;
void exception_handler_syscall(void);
void do_handler_syscall(syscall_frame_t* frame);
#endif