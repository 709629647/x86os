#ifndef TIME_H
#define TIME_H

#define PIT_OSC_FREQ            1193182
#define PIT_COMMAND_MODE_PORT   0X43    //定时器命令模式端口
#define PIT_CHANNEL0_DATA_PORT  0X40    //定时器0数据端口

#define PIT_CHANNEL             (0 << 6)    //D6和D7选择片
#define PIT_LOAD_LOHI           (3 << 4)    //16位
#define PIT_MODE3               (3 << 1)    //时钟的输出波形相关

void exception_handler_time(void);

void timer_init(void);
#endif