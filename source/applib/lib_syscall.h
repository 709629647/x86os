#ifndef LIB_SYSCALL_H
#define LIB_SYSCALL_H
#include "os_cfg.h"
#include "core/syscall.h"
#include "tools/log.h"
#include <stddef.h>
#include "sys/stat.h"
typedef struct _sys_arg_t
{
    uint32_t id;
    uint32_t arg0;
    uint32_t arg1;
    uint32_t arg2;
    uint32_t arg3;
}sys_arg_t;

static inline int sys_call(sys_arg_t* args)
{
	uint32_t addr[] = {0, SYSTEM_SELECTOR | 0};
    int ret;
	__asm__ __volatile__(
        "push %[arg3]\n\t"
        "push %[arg2]\n\t"
        "push %[arg1]\n\t"
        "push %[arg0]\n\t"
        "push %[id]\n\t"
        "lcalll *(%[a])":"=a"(ret)
        :[arg3]"r"(args->arg3), [arg2]"r"(args->arg2),
        [arg1]"r"(args->arg1),[arg0]"r"(args->arg0),[id]"r"(args->id),[a]"r"(addr));
    return ret;
}
struct dirent{
    int index;
    int type;
    char name[255];
    int size;
};
typedef struct _DIR
{
    int index;
    struct dirent dirent;
}DIR;
void sleep (int ms);
uint32_t getpid(void);
void mprint_msg(char* fmt, int value);
int fork(void);
int yield(void);
int execve(const char*name , char* const* argv, char* const* env);
int open(const char* name, int flag, ...);
int read(int fd, char* buf, int size);
int write(int fd, char* buf, int size);
int lseek(int fd, int ptr, int dir);
int close(int fd);
int fstat(int file, struct stat *st);
int isatty(int file);
void * sbrk(ptrdiff_t incr) ;
int dup(int file);
void _exit(int status);
int wait(int* status);
DIR* opendir(const char* path);
struct dirent* readdir(DIR* dir);
int closedir(DIR* dir);
int ioctl(int file, int cmd, int arg0, int arg1);
int unlink(const char* path);
#endif