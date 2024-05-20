#ifndef SEM_H
#define SEM_H
#include "tools/list.h"
#include "core/task.h"

typedef struct _sem_t{
    int count;
    list_t wait_list;
}sem_t;
extern task_manager_t task_manager;
void sem_init (sem_t * sem, int init_count);
void sem_wait (sem_t * sem);
void sem_notify (sem_t * sem);
int sem_count (sem_t * sem);
#endif