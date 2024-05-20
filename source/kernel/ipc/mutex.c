#include "ipc/mutex.h"
#include "cpu/irq.h"
void mutex_init(mutex_t* mutex){
    mutex->owner = (task_t*) 0;
    mutex->lock_count = 0;
    list_init(&mutex->wait_list);
}

void mutex_lock(mutex_t* mutex){
    irq_state_t state = enter_protection();
    task_t* curr_task = task_manager.curr_task;
    if(mutex->lock_count == 0)
    {
        ++mutex->lock_count;
        mutex->owner = curr_task;
    }
     else if (mutex->owner == curr_task) {
        // 已经为当前任务所有，只增加计数
        mutex->lock_count++;
    }
    else {
        // 有其它任务占用，则进入队列等待
        task_t * curr = task_manager.curr_task;
        set_block(curr);
        list_insert_last(&mutex->wait_list, &curr->wait_node);
        task_dispatch();
    }
    leave_protection(state);
}

void mutex_unlock(mutex_t* mutex){
    irq_state_t state = enter_protection();
    task_t* curr_task = task_manager.curr_task;
    if(mutex->owner == curr_task){
        if(--mutex->lock_count == 0){
            mutex->owner = (task_t*) 0;

            if(list_count(&mutex->wait_list)){
                list_node_t* node = list_remove_first(&mutex->wait_list);
                task_t* task = LIST_NODE_PARENT(node, task_t, wait_node);
                set_ready(task);
                mutex->lock_count++;
                mutex->owner = task;
                task_dispatch();
            }
        }
    }

    leave_protection(state);
}