#include "core/syscall.h"
#include "core/task.h"
#include "tools/log.h"
#include "core/memory.h"
#include "fs/fs.h"
typedef int (*syscall_handler_t) (uint32_t arg0, uint32_t arg1, uint32_t arg2, uint32_t arg3);
static const syscall_handler_t syscall_table[] = {
    [SYS_SLEEP] = (syscall_handler_t)sys_sleep,
    [SYS_GETPID] = (syscall_handler_t)sys_getpid,
    [SYS_FORK]   = (syscall_handler_t)sys_fork,
    [SYS_EXECVE]   = (syscall_handler_t)sys_execve,
    [SYS_YIELD]   = (syscall_handler_t)sys_shed_yield,
    [SYS_OPEN]   = (syscall_handler_t)sys_open,
    [SYS_READ]   = (syscall_handler_t)sys_read,
    [SYS_WRITE]   = (syscall_handler_t)sys_write,
    [SYS_LSEEK]   = (syscall_handler_t)sys_lseek,
    [SYS_CLOSE]   = (syscall_handler_t)sys_close,
    [SYS_FSTAT]   = (syscall_handler_t)sys_fstat,
    [SYS_ISATTY]   = (syscall_handler_t)sys_isatty,
    [SYS_SBRK]   = (syscall_handler_t)sys_sbrk,
    [SYS_DUP]   = (syscall_handler_t)sys_dup,
    [SYS_IOCTL] =  (syscall_handler_t)sys_ioctl,
    [SYS_UNLINK] = (syscall_handler_t)sys_unlink,
    [SYS_EXIT]  =   (syscall_handler_t)sys_exit,
    [SYS_WAIT]  =   (syscall_handler_t)sys_wait,
    [SYS_OPENDIR]  =  (syscall_handler_t)sys_opendir,
    [SYS_READDIR]  =  (syscall_handler_t)sys_readdir,
    [SYS_CLOSEDIR] = (syscall_handler_t)sys_closedir,
    [SYS_PRINTMSG] = (syscall_handler_t)sys_print_msg,
};
void do_handler_syscall(syscall_frame_t* frame)
{
    if(frame->id < sizeof(syscall_table) / sizeof(syscall_handler_t))
    {
        syscall_handler_t handler = syscall_table[frame->id];
        if(handler)
        {
            int ret = handler(frame->arg0, frame->arg1, frame->arg2, frame->arg3);
            frame->eax = ret;
            return;
        }
            
    }
    task_t* task = task_manager.curr_task;
    log_printf("task is : %s", task->name);
}