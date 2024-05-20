#include "comm/cpu_instr.h"
#include "tools/log.h"
#include "os_cfg.h"
#include "tools/klib.h"
#include <stdarg.h>
#include "cpu/irq.h"
#include "ipc/mutex.h"
#include "dev/console.h"
#include "dev/dev.h"
#define USE_LOG             0
static mutex_t mutex;
#define COM1_PORT           0x3F8  
static int log_dev_id;
void log_init (void) {
    mutex_init(&mutex);
    log_dev_id = dev_open(DEV_TTY, 0, 0);
#if USE_LOG
    outb(COM1_PORT + 1, 0x00);    // Disable all interrupts
    outb(COM1_PORT + 3, 0x80);    // Enable DLAB (set baud rate divisor)
    outb(COM1_PORT + 0, 0x03);    // Set divisor to 3 (lo byte) 38400 baud
    outb(COM1_PORT + 1, 0x00);    //                  (hi byte)
    outb(COM1_PORT + 3, 0x03);    // 8 bits, no parity, one stop bit
    outb(COM1_PORT + 2, 0xC7);    // Enable FIFO, clear them, with 14-byte threshold

    // If serial is not faulty set it in normal operation mode
    // (not-loopback with IRQs enabled and OUT#1 and OUT#2 bits enabled)
    outb(COM1_PORT + 4, 0x0F);
#endif 
}

void log_printf(const char * fmt, ...) {
    
    char buf[128];
    kernel_memset(buf, '\0', sizeof(buf));
    va_list args;
    va_start(args, fmt);
    kernel_vsprintf(buf, fmt, args);
    va_end(args);
    mutex_lock(&mutex);
#if USE_LOG
    const char* p = buf;
    while (*p != '\0') {
        while ((inb(COM1_PORT + 5) & (1 << 6)) == 0);
        outb(COM1_PORT, *p++);
    }

    outb(COM1_PORT, '\r');
    outb(COM1_PORT, '\n');
#else
    dev_write(log_dev_id, 0, "log:", 4);
    dev_write(log_dev_id, 0, buf, kernel_strlen(buf));
    // console_write(0, buf, kernel_strlen(buf));
    char c = '\n';
    dev_write(log_dev_id, 0, &c, 1);
    // console_write(0, &c, 1);
#endif
    mutex_unlock(&mutex);
}