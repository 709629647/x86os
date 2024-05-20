#ifndef TASK_H
#define TASK_H
#include "cpu/cpu.h"
#include "comm/types.h"
#include "tools/list.h"
#include "comm/elf.h"
#include "fs/file.h"
#define TASK_NAME_SIZE      32
#define TASK_TIME_TICKS     10
#define TASK_NUM            128
#define TASK_SYSTEM_FLAG    (1 << 0)
#define TASK_OFILE_NR       128
typedef struct _task_arg_t
{
    int retval;
    int argc;
    char** argv;
}task_arg_t;

typedef struct _task_t
{
    // uint32_t* stack;
    enum{
        CREATE,
        RUNNING,
        READY,
        SLEEPING,
        WAITING,
        ZOMBIE,
    }state;
    uint32_t pid;
    uint32_t heap_start;
    uint32_t heap_end;
    list_node_t     run_node;
    list_node_t     wait_node;
    list_node_t     all_node;
    file_t* file_table[TASK_OFILE_NR];
    struct _task_t * parent;
    char name[TASK_NAME_SIZE];
    int sleep_ticks;
    int slice_ticks;
    int time_ticks;
    int status;
    tss_t tss;
    uint16_t tss_selector;
}task_t;

int   task_init(task_t* task, const char* name, uint32_t entry, uint32_t esp, int flag);

static int tss_init(task_t* task, uint32_t entry, uint32_t esp, int flag);
void task_switch_from_to(task_t* from, task_t* to);
int cpy_to_stack(char* to, uint32_t page_dir, int argc, char*const* argv);
typedef struct _task_manager_t{
    list_t ready_list;
    list_t task_list;
    list_t sleep_list;
    task_t* curr_task;
    task_t  first_task;
    task_t idle_task;
    uint32_t app_code_sel;
    uint32_t app_data_sel;
}task_manager_t;

void task_manager_init(void);
void first_task_init(void);

task_t* task_first_task(void);
void task_dispatch(void);
void set_ready(task_t* task);
void sys_shed_yield(void);
void set_block(task_t* task);
task_t * current_task(void);
void task_time_tick(void);
void set_sleep(task_t* task, int ticks);
void set_wake(task_t* task);
void sys_sleep(uint32_t ms);
task_manager_t task_manager;
int sys_print_msg (char * fmt, int arg);
uint32_t sys_getpid(void);
int sys_fork(void);
static task_t* alloc_task(void);
static void free_task(task_t* task);
int sys_execve(const char*name , char* const* argv, char* const* env);
// void simple_switch(uint32_t** from, uint32_t* to);
static uint32_t load_elf_file (task_t * task, const char * name, uint32_t page_dir) ;
int load_phdr(int fd, Elf32_Phdr* elf_phdr ,uint32_t page_dir);
file_t * task_file (int fd);
int task_alloc_fd (file_t * file) ;
void task_remove_fd (int fd);
void sys_exit(int status);
int sys_wait(int* status);
#endif