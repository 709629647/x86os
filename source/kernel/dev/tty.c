/**
 * 终端tty
 * 目前只考虑处理cooked模式的处理
 *
 * 作者：李述铜
 * 联系邮箱: 527676163@qq.com
 */
#include "dev/tty.h"
#include "dev/console.h"
#include "dev/kbd.h"
#include "tools/log.h"
#include "dev/dev.h"
#include "cpu/irq.h"
#define 	TTY_NUM			8
static tty_t tty_devs[TTY_NUM];
static int curr_tty = 0;
/**
 * @brief 打开tty设备
 */
void fifo_init(tty_fifo_t* fifo, char* buf, int size){
	fifo-> buf = buf;
	fifo->size = size;
	fifo->count = 0;
	fifo->read	= fifo->write = 0;
}
int tty_open (device_t * dev)  {
	
	int idx = dev -> minor;
	if(idx < 0 || idx >= TTY_NUM)
		return -1;
	tty_t* tty = tty_devs + idx;
	sem_init(&tty->osem, TTY_OBUF_SIZE);
	sem_init(&tty->isem, 0);
	fifo_init(&tty->ififo, tty->ibuf, TTY_IBUF_SIZE);
	fifo_init(&tty->ofifo, tty->obuf, TTY_OBUF_SIZE);
	tty->oflags = TTY_OCRLF;
	tty->iflags = TTY_INCLR | TTY_IECHO;
	tty->console_idx = idx;
	console_init(idx);
	kbd_init();
	return 0;
}
int tty_fifo_get (tty_fifo_t * fifo, char * c) {
	
	if (fifo->count <= 0) {
		return -1;
	}
	irq_state_t state = enter_protection();
	*c = fifo->buf[fifo->read++];
	if (fifo->read >= fifo->size) {
		fifo->read = 0;
	}
	fifo->count--;
	leave_protection(state);

	return 0;
}
int tty_fifo_put (tty_fifo_t * fifo, char c) {
	irq_state_t state = enter_protection();
	if (fifo->count >= fifo->size) {
		leave_protection(state);
		return -1;
	}

	fifo->buf[fifo->write++] = c;
	if (fifo->write >= fifo->size) {
		fifo->write = 0;
	}
	fifo->count++;
	leave_protection(state);
	return 0;
}
static inline tty_t * get_tty (device_t * dev) {
	int tty = dev->minor;
	if ((tty < 0) || (tty >= TTY_NR) || (!dev->open_count)) {
		log_printf("tty is not opened. tty = %d", tty);
		return (tty_t *)0;
	}

	return tty_devs + tty;
}
/**
 * @brief 从tty读取数据
 */
int tty_read (device_t * dev, int addr, char * buf, int size) {
	tty_t* tty = get_tty(dev);
	if(tty == (tty_t *)0)
		return -1;
	int len = 0;
	char* pbuf = buf;
	while(len < size)
	{
		char c ;
		sem_wait(&tty->isem);
		tty_fifo_get(&tty->ififo, &c);
		switch(c)
		{
			case 0x7F:
			if (len == 0) {
				continue;
			}
			len--;
			pbuf--;
			break;
			case '\n':
			if((tty->iflags & TTY_INCLR) && len < size - 1)
			{
				*pbuf++ = '\r';
				len++;
			}
			*pbuf++ = '\n';
			len++;
			break;
			default:
				*pbuf++ = c;
				len++;
				break;
		}
		if(tty->iflags & TTY_IECHO)
			tty_write(dev, addr, &c, 1);
		if(c =='\n' || c =='\r')
			break;
	}
	return len;
}

/**
 * @brief 向tty写入数据
 */
int tty_write (device_t * dev, int addr, char * buf, int size) {
	tty_t* tty = get_tty(dev);
	if(tty == (tty_t *)0)
		return -1;
	int len = 0;
	while (size > 0)
	{
		char c = *buf++;
		if(c == '\n' && (tty->oflags & TTY_OCRLF))
		{
			sem_wait(&tty->osem);
			int err = tty_fifo_put(&tty->ofifo, '\r');
			if(err < 0)
				return -1;
		}
		sem_wait(&tty->osem);
		int err = tty_fifo_put(&tty->ofifo, c);
		if(err < 0)
			return -1;
		++len;
		--size;
		console_write(tty);
	} 
	
	return len;
}

void tty_in(char c)
{
	tty_t* tty = tty_devs + curr_tty;
	if(sem_count(&tty->isem) >= TTY_IBUF_SIZE ){
		return;
	}
	tty_fifo_put(&tty->ififo, c);
	sem_notify(&tty->isem);
}
/**
 * @brief 向tty设备发送命令
 */
int tty_control (device_t * dev, int cmd, int arg0, int arg1) {
	tty_t* tty = get_tty(dev);
	switch (cmd)
	{
	case TTY_CMD_ECHO:
		if(arg0)
			tty->iflags |= TTY_IECHO;
		else	
			tty->iflags &= ~TTY_IECHO;
		break;
	
	default:
		break;
	}
}

/**
 * @brief 关闭tty设备
 */
void tty_close (device_t * dev) {

}

void tty_select (int tty) {
	if (tty != curr_tty) {
		console_select(tty);
		curr_tty = tty;
	}
}
// 设备描述表: 描述一个设备所具备的特性
dev_desc_t dev_tty_desc = {
	.name = "tty",
	.major = DEV_TTY,
	.open = tty_open,
	.read = tty_read,
	.write = tty_write,
	.control = tty_control,
	.close = tty_close,
};