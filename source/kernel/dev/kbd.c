/**
 * 键盘设备处理
 *
 * 作者：李述铜
 * 联系邮箱: 527676163@qq.com
 */
#include "comm/cpu_instr.h"
#include "cpu/irq.h"
#include "dev/kbd.h"
#include "tools/klib.h"
#include "tools/log.h"
#include "dev/tty.h"
kbd_state_t kbd_state;
static const key_map_t map_table[256] = {
        [0x2] = {'1', '!'},
        [0x3] = {'2', '@'},
        [0x4] = {'3', '#'},
        [0x5] = {'4', '$'},
        [0x6] = {'5', '%'},
        [0x7] = {'6', '^'},
        [0x08] = {'7', '&'},
        [0x09] = {'8', '*' },
        [0x0A] = {'9', '('},
        [0x0B] = {'0', ')'},
        [0x0C] = {'-', '_'},
        [0x0D] = {'=', '+'},
        [0x0E] = {ASCII_DEL, ASCII_DEL},
        [0x0F] = {'\t', '\t'},
        [0x10] = {'q', 'Q'},
        [0x11] = {'w', 'W'},
        [0x12] = {'e', 'E'},
        [0x13] = {'r', 'R'},
        [0x14] = {'t', 'T'},
        [0x15] = {'y', 'Y'},
        [0x16] = {'u', 'U'},
        [0x17] = {'i', 'I'},
        [0x18] = {'o', 'O'},
        [0x19] = {'p', 'P'},
        [0x1A] = {'[', '{'},
        [0x1B] = {']', '}'},
        [0x1C] = {'\n', '\n'},
        [0x1E] = {'a', 'A'},
        [0x1F] = {'s', 'S'},
        [0x20] = {'d',  'D'},
        [0x21] = {'f', 'F'},
        [0x22] = {'g', 'G'},
        [0x23] = {'h', 'H'},
        [0x24] = {'j', 'J'},
        [0x25] = {'k', 'K'},
        [0x26] = {'l', 'L'},
        [0x27] = {';', ':'},
        [0x28] = {'\'', '"'},
        [0x29] = {'`', '~'},
        [0x2B] = {'\\', '|'},
        [0x2C] = {'z', 'Z'},
        [0x2D] = {'x', 'X'},
        [0x2E] = {'c', 'C'},
        [0x2F] = {'v', 'V'},
        [0x30] = {'b', 'B'},
        [0x31] = {'n', 'N'},
        [0x32] = {'m', 'M'},
        [0x33] = {',', '<'},
        [0x34] = {'.', '>'},
        [0x35] = {'/', '?'},
        [0x39] = {' ', ' '},
};

int is_makecode(uint8_t raw_code)
{
    return !(raw_code & 0x80);
}

uint8_t get_key(uint8_t raw_code)
{
    return raw_code & 0x7F;
}
static void do_fx_key (int key) {
    int index = key - KEY_F1;
    if (kbd_state.lctrl_press || kbd_state.rctrl_press) {
        tty_select(index);
    }
}
/**
 * @brief 按键中断处理程序
 */
static void do_normal_key (uint8_t raw_code)
{
    int make_code = is_makecode(raw_code);
    uint8_t key = get_key(raw_code);
    switch (key)
    {
    case KEY_LSHIFT:
        kbd_state.lshift_press = make_code;
        break;
    case KEY_RSHIFT:
        kbd_state.rshift_press = make_code;
        break;
    case KEY_CAPS:
        if(make_code)
        kbd_state.caps_loc = ~kbd_state.caps_loc;
        break;
    case KEY_ALT:
        kbd_state.lalt_press = make_code;  
        break;
    case KEY_CTRL:
        kbd_state.lctrl_press = make_code;  
        break;
    case KEY_F1:
    case KEY_F2:
    case KEY_F3:
    case KEY_F4:
    case KEY_F5:
    case KEY_F6:
    case KEY_F7:
    case KEY_F8:
        do_fx_key(key);
        break;
    case KEY_F9:
    case KEY_F10:
    case KEY_F11:
    case KEY_F12:
    case KEY_SCROLL_LOCK:
    default:
        if(make_code)
        {   
            if(kbd_state.lshift_press || kbd_state.rshift_press)
            {
                key = map_table[key].func;
             
            }
            else 
                key = map_table[key].normal;
            
            if(kbd_state.caps_loc)
            {
                if(key >= 'A' && key <= 'Z')
                {
                    key = key - 'A' + 'a';
                }
                else if(key >= 'a' && key <='z')
                {
                    key = key - 'a' + 'A';
                }
            }
            tty_in(key);
        }
       
        break;
    }
}
void do_handler_kbd(exception_frame_t *frame) {
     static enum {
    	NORMAL,				
		BEGIN_E0,			
		BEGIN_E1,			
    }recv_state = NORMAL;
	// 检查是否有数据，无数据则退出
	uint8_t status = inb(KBD_PORT_STAT);
	if (!(status & KBD_STAT_RECV_READY)) {
        pic_send_eoi(IRQ1_KEYBOARD);
		return;
	}

	// 读取键值
    uint8_t raw_code = inb(KBD_PORT_DATA);
    
    
    
    pic_send_eoi(IRQ1_KEYBOARD);
    if (raw_code == KEY_E0) {
		recv_state = BEGIN_E0;
	} else if (raw_code == KEY_E1) {
		recv_state = BEGIN_E1;
	} else {
		switch (recv_state) {
		case NORMAL:
			do_normal_key(raw_code);
			break;
		case BEGIN_E0: // 涓嶅鐞唒rint scr
			do_e0_key(raw_code);
			recv_state = NORMAL;
			break;
		case BEGIN_E1:  // 涓嶅鐞唒ause
			recv_state = NORMAL;
			break;
		}
	}
}

void do_e0_key (uint8_t raw_code) {
    int key = get_key(raw_code);			
    int make_code = is_makecode(raw_code);	

    
    switch (key) {
        case KEY_CTRL:		
            kbd_state.rctrl_press = make_code; 
            break;
        case KEY_ALT:
            kbd_state.ralt_press = make_code;  
            break;
    }
}

/**
 * 键盘硬件初始化
 */
void kbd_init(void) {
    static int init = 0;
    if(!init)
    {
        kernel_memset((void*)&kbd_state, 0, sizeof(kbd_state));
        irq_install(IRQ1_KEYBOARD, (irq_handler_t)exception_handler_kbd);
        irq_enable(IRQ1_KEYBOARD);
        init = 1;
    }
}
