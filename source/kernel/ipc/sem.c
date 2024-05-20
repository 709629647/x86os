/**
 * 计数信号量
 *
 * 作者：李述铜
 * 联系邮箱: 527676163@qq.com
 */
/**
 * 信号量初始化
 */
#include "cpu/irq.h"
#include "core/task.h"
#include "ipc/sem.h"
void sem_init (sem_t * sem, int init_count) {
    sem->count = init_count;
    list_init(&sem->wait_list);
}

/**
 * 申请信号量
 */
void sem_wait (sem_t * sem) {
    irq_state_t  irq_state = enter_protection();

    if (sem->count > 0) {
        sem->count--;
    } else {
        // 从就绪队列中移除，然后加入信号量的等待队列
        task_t * curr = task_manager.curr_task;
        set_block(curr);
        list_insert_last(&sem->wait_list, &curr->wait_node);
        task_dispatch();
    }

    leave_protection(irq_state);
}

/**
 * 释放信号量
 */
void sem_notify (sem_t * sem) {
    irq_state_t  irq_state = enter_protection();

    if (list_count(&sem->wait_list)) {
        // 有进程等待，则唤醒加入就绪队列
        list_node_t * node = list_remove_first(&sem->wait_list);
        task_t * task = LIST_NODE_PARENT(node, task_t, wait_node);
        set_ready(task);

        task_dispatch();
    } else {
        sem->count++;
    }

    leave_protection(irq_state);
}

/**
 * 获取信号量的当前值
 */
int sem_count (sem_t * sem) {
    irq_state_t  irq_state = enter_protection();
    int count = sem->count;
    leave_protection(irq_state);
    return count;
}

