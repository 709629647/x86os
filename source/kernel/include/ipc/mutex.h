#ifndef MUTEX_H
#define MUTEX_H
#include "core/task.h"
#include "tools/list.h"
typedef struct _mutex_t
{
    
    int lock_count;
    list_t wait_list;
    task_t* owner;
}mutex_t;

void mutex_init(mutex_t* mutex);
void mutex_lock(mutex_t* mutex);
void mutex_unlock(mutex_t* mutex);
#endif