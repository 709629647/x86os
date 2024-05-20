#include "lib_syscall.h"

void sleep (int ms)
{
    if(ms < 0)
        return;
    sys_arg_t args;
    args.id = SYS_SLEEP;
    args.arg0 = ms;
    sys_call(&args);
}

uint32_t getpid(void)
{
    sys_arg_t args;
    args.id = SYS_GETPID;
    return sys_call(&args);
}

void mprint_msg(char* fmt, int value)
{
    sys_arg_t args;
    args.id = SYS_PRINTMSG;
    args.arg0 = (uint32_t)fmt;
    args.arg1 = (uint32_t)value;
    sys_call(&args);
}

int fork(void)
{
    sys_arg_t args;
    args.id = SYS_FORK;
    sys_call(&args);
}

int yield(void)
{
    sys_arg_t args;
    args.id = SYS_YIELD;
    sys_call(&args);
}

int execve(const char*name , char* const* argv, char* const* env)
{
    sys_arg_t args;
    args.id = SYS_EXECVE;
    args.arg0 = (int)name;
    args.arg1 = (int)argv;
    args.arg2 = (int)env;
    sys_call(&args);
}

int open(const char* name, int flag, ...)
{
    sys_arg_t args;
    args.id = SYS_OPEN;
    args.arg0 = (int)name;
    args.arg1 = flag;
    sys_call(&args);
}
int read(int fd, char* buf, int size)
{
    sys_arg_t args;
    args.id = SYS_READ;
    args.arg0 = fd;
    args.arg1 = (int)buf;
    args.arg2 = size;
    sys_call(&args);
}
int write(int fd, char* buf, int size)
{
    sys_arg_t args;
    args.id = SYS_WRITE;
    args.arg0 = fd;
    args.arg1 = (int)buf;
    args.arg2 = size;
    sys_call(&args);
}
int lseek(int fd, int ptr, int dir)
{
    sys_arg_t args;
    args.id = SYS_LSEEK;
    args.arg0 = fd;
    args.arg1 = ptr;
    args.arg2 = dir;
    sys_call(&args);
}
int close(int fd)
{
    sys_arg_t args;
    args.id = SYS_CLOSE;
    args.arg0 = fd;
    sys_call(&args);
}

int fstat(int file, struct stat *st) {
    sys_arg_t args;
    args.id = SYS_FSTAT;
    args.arg0 = (int)file;
    args.arg1 = (int)st;
    return sys_call(&args);
}
int unlink(const char* path)
{
    sys_arg_t args;
    args.id = SYS_UNLINK;
    args.arg0 = (int)path;
    return sys_call(&args);
}
int isatty(int file) {
    sys_arg_t args;
    args.id = SYS_ISATTY;
    args.arg0 = (int)file;
    return sys_call(&args);
}

void * sbrk(ptrdiff_t incr) {
    sys_arg_t args;
    args.id = SYS_SBRK;
    args.arg0 = (int)incr;
    return (void *)sys_call(&args);
}

int dup(int file) {
    sys_arg_t args;
    args.id = SYS_DUP;
    args.arg0 = (int)file;
    return sys_call(&args);
}

void _exit(int status)
{
    sys_arg_t args;
    args.id = SYS_EXIT;
    args.arg0 = status;
    sys_call(&args);
}

int wait(int* status)
{
    sys_arg_t args;
    args.id = SYS_WAIT;
    args.arg0 = (int)status;
    return sys_call(&args);
}



DIR* opendir(const char* path)
{
    DIR* dir = (DIR*)malloc(sizeof(DIR));
    if(dir == (DIR*)0)
    {
        return (DIR*)0;
    }
    sys_arg_t args;
    args.id = SYS_OPENDIR;
    args.arg0 = (int)path;
    args.arg1 = (int)dir;
    int err = sys_call(&args);
    if(err < 0)
    {
        free(dir);
        return (DIR*)0;
    }
    return dir;
}

struct dirent* readdir(DIR* dir)
{
    sys_arg_t args;
    args.id = SYS_READDIR;
    args.arg0 = (int)dir;
    args.arg1 = (int)&dir->dirent;
    int err = sys_call(&args);
    if(err < 0)
    {
        return (struct dirent*)0;
    }
    return &dir->dirent;
}

int ioctl(int file, int cmd, int arg0, int arg1) {
    sys_arg_t args;
    args.id = SYS_IOCTL;
    args.arg0 = file;
    args.arg1 = cmd;
    args.arg2 = arg0;
    args.arg3 = arg1;
    return sys_call(&args);
}

int closedir(DIR* dir)
{
    sys_arg_t args;
    args.id = SYS_CLOSEDIR;
    args.arg0 = (int)dir;
    sys_call(&args);
    free(dir);
    return 0;
}
