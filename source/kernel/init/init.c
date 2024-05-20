#include "init.h"
#include "comm/boot_info.h"
#include "cpu/cpu.h"
#include "cpu/irq.h"
#include "dev/time.h"
#include "tools/log.h"
#include "tools/klib.h"
#include "core/task.h"
#include "comm/cpu_instr.h"
#include "tools/list.h"
#include "ipc/sem.h"
#include "core/memory.h"
#include "dev/console.h"
#include "dev/kbd.h"
#include "fs/fs.h"
// static boot_info_t * init_boot_info;

void kernel_init(boot_info_t* boot_info){
    // init_boot_info = boot_info;
    Assert(boot_info->ram_region_count != 0);
    // Assert(3 < 2);
    cpu_init();
    irq_init();
    log_init();
    memory_init(boot_info);
    fs_init();

    timer_init();
    task_manager_init();
    // init_main();

}
// static task_t init_task_struct;

// static uint32_t init_task_stack[1024];
static sem_t sem;
// void init_task(void){
//     int count = 0;
//     for(;;){
//         // sem_wait(&sem);
//         log_printf("init_task %d", count++);
//         // sys_sleep(500);
//         // task_switch_from_to(&init_task_struct, task_first_task());
//         // sys_shed_yield();
//     }
// }
void move_to_first_task(void) {
    task_t * curr = task_manager.curr_task;
    Assert(curr != 0);

    tss_t * tss = &(curr->tss);

    __asm__ __volatile__(
        "push %[ss]\n\t"			// SS
        "push %[esp]\n\t"			// ESP
        "push %[eflags]\n\t"           // EFLAGS
        "push %[cs]\n\t"			// CS
        "push %[eip]\n\t"		    // ip
        "iret\n\t"::[ss]"r"(tss->ss),  [esp]"r"(tss->esp), [eflags]"r"(tss->eflags),
        [cs]"r"(tss->cs), [eip]"r"(tss->eip));
}

void init_main(void){
    // int a = 3 / 0;
    // irq_enable_global();
    // list_test();
    log_printf("====================================");
    log_printf("Kernel is running....");
    log_printf("Version: %s, name: %s", "1.0.0", "tiny x86 os");
    log_printf("====================================");
    log_printf("%d %d %x %c", -123, 123456, 0x12345, 'a');
    first_task_init();
    move_to_first_task();
    // task_init(&init_task_struct, "init_task", (uint32_t)init_task, (uint32_t)&init_task_stack[1023]);
    
    
    // sem_init(&sem, 2);
    // irq_enable_global();
    // int count = 0;
    // for(;;){
    //     log_printf("init_main %d", count++);
    //     // sem_notify(&sem);
    //     // sys_sleep(1000);
    //     // sys_shed_yield();
    // }
}
