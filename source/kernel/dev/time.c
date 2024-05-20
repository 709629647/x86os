#include "dev/time.h"
#include "comm/types.h"
#include "cpu/irq.h"
#include "os_cfg.h"
#include "comm/cpu_instr.h"
#include "core/task.h"
static uint32_t sys_tick;
/**
 * 初始化硬件定时器
 */
static void init_pit(){
    uint32_t reload_count = OS_TICK_MS * PIT_OSC_FREQ / 1000;
    outb(PIT_COMMAND_MODE_PORT, PIT_CHANNEL | PIT_LOAD_LOHI| PIT_MODE3);
    outb(PIT_CHANNEL0_DATA_PORT, reload_count & 0xFF);
    outb(PIT_CHANNEL0_DATA_PORT, (reload_count >> 8) & 0xFF);
    irq_install(IRQ0_TIME , exception_handler_time);
    irq_enable(IRQ0_TIME);
}
void do_handler_time(exception_frame_t* frame){
    ++sys_tick;
    pic_send_eoi(IRQ0_TIME); //通知芯片，中断已经处理完成了，
    //芯片会继续准备接收下一个中断信号

    task_time_tick();
}
/**
 * 定时器初始化
 */
void timer_init(void)
{
    sys_tick = 0;
    init_pit();
}