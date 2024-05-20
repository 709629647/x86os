#include "dev/console.h"
#include "tools/klib.h"
#include "dev/tty.h"
#include "cpu/irq.h"
#define CONSOLE_NUM         8
static console_t console_buf[CONSOLE_NUM];
static int curr_console_idx = 0;
static int read_cursor_pos (void) {
    int pos;
    irq_state_t state = enter_protection();
 	outb(0x3D4, 0x0F);		
	pos = inb(0x3D5);
	outb(0x3D4, 0x0E);		
	pos |= inb(0x3D5) << 8;
    leave_protection(state);   
    return pos;
}

static void update_cursor_pos (console_t * console) {
    uint16_t pos = (console - console_buf)*console->disp_col*console->disp_row;
	pos += console->cur_row *  console->disp_col + console->cur_col;
    irq_state_t state = enter_protection();
   
	outb(0x3D4, 0x0F);		
	outb(0x3D5, (uint8_t) (pos & 0xFF));
	outb(0x3D4, 0x0E);		
	outb(0x3D5, (uint8_t) ((pos >> 8) & 0xFF));
    leave_protection(state);
}
void console_clear(console_t* console)
{
    int size = console->disp_col * console->disp_row;
    disp_char_t* start = console->base;
    for(int i = 0; i < size; ++i, ++start)
    {
        start->value = ' ';
        start->color = (console->foreground) | (console -> background << 4);
    }
}
int console_init(int idx)
{
    
    console_t* console = console_buf + idx;
    console -> disp_row = CONSOLE_ROW;
    console -> disp_col = CONSOLE_COL;
    console->foreground = COLOR_White;
    console->background = COLOR_Black;
    mutex_init(&console->mutex);
    if(idx == 0)
    {int cursor_pos = read_cursor_pos();
    console->cur_row = cursor_pos / console->disp_col;
    console->cur_col = cursor_pos % console->disp_col;}
    else{
        console->cur_row = 0;
        console->cur_col = 0;
        console_clear(console);
    }
    console ->curr_param_index = 0;
    console -> base = (disp_char_t*)CONSOLE_BASE + (idx*CONSOLE_ROW * CONSOLE_COL);
    // console_clear(console);
    console->save_cur_col = console->cur_col;
    console->save_cur_row = console->cur_row;
    //  update_cursor_pos(console);

    return 0;
}
void  move_forward(console_t* console, int step)
{
    for(int i = 0; i < step; ++i)
    {
        if((++console->cur_col) >= console->disp_col)
        {
            console->cur_row +=1;
            console->cur_col = 0;
        }
        if((console->cur_row) >= console->disp_row)
        {
            scroll_up(console, 1);
        }
    }
}
void show_console(console_t* console, const char c)
{
    int offset = console->cur_col + console->cur_row * console->disp_col;
    disp_char_t *p = console->base + offset;
    
    p ->value =  c;
    p ->color =  (console->foreground) | (console -> background << 4);
    move_forward(console, 1);
}

void erase_backword(console_t* console)
{
    if(cursor_backword(console, 1) == 0)
    {
        show_console(console, ' ');
        cursor_backword(console, 1);
    }
    
}
void console_select(int idx) {
    console_t * console = console_buf + idx;
    if (console->base == (disp_char_t*)0) {

        console_init(idx);
    }

	uint16_t pos = idx * console->disp_col * console->disp_row;
	outb(0x3D4, 0xC);		
	outb(0x3D5, (uint8_t) ((pos >> 8) & 0xFF));
	outb(0x3D4, 0xD);	
	outb(0x3D5, (uint8_t) (pos & 0xFF));
    curr_console_idx = idx;
    update_cursor_pos(console);
}

int cursor_backword(console_t* console, int len)
{
    int status = -1;
    for(int i = 0; i < len; ++i)
    {
        if(console->cur_col > 0)
        {
            console->cur_col -- ;
            status = 0;
        }
        else if(console->cur_row > 0)
        {
            console -> cur_col = console->disp_col - 1;
            console -> cur_row -=1;
            status = 0;
        }
    }
    return status;
}
void write_normal(console_t* console, char c)
{
    switch (c)
        {
        case ASC_ESC:
            console->write_state = CONSOLE_WRITE_ESC;
            break; 
        case 0x7F:
            erase_backword(console);
            break;
        case '\b':
            cursor_backword(console, 1);
            break;
        case '\r': 
            set_col0(console);
            break;
        case '\n':
            // set_col0(console);
            add_row1(console);
            break;
        
        default:
            if(c >=' ' && c <= '~')
                show_console(console, c);
            break;
        }    
}
void save_cursor(console_t* console)
{
    console->save_cur_col = console->cur_col;
    console->save_cur_row = console->cur_row;
}
void load_cursor(console_t* console)
{
    console->cur_col = console->save_cur_col;
    console->cur_row = console->save_cur_row;
}
void clear_esc_param(console_t* console)
{
    kernel_memset((void*)console->esc_param, 0, sizeof(console->esc_param));
    console->curr_param_index = 0;
}
void write_esc(console_t* console, char c)
{
    switch (c)
        {
        case '7':
            save_cursor(console);
            console ->write_state = CONSOLE_WRITE_NORMAL;
            break;
        case '8':
            load_cursor(console);
            console ->write_state = CONSOLE_WRITE_NORMAL;
            break;
        case '[':
            clear_esc_param(console);
            console ->write_state = CONSOLE_WRITE_ESC_SQUARE;
            break;
        default:
            console ->write_state = CONSOLE_WRITE_NORMAL;
            break;
        }    
}
static void set_font_style(console_t* console)
{
    static const cclor_t color_table[] = {
			COLOR_Black, COLOR_Red, COLOR_Green, COLOR_Yellow, // 0-3
			COLOR_Blue, COLOR_Magenta, COLOR_Cyan, COLOR_White, // 4-7
	};

	for (int i = 0; i < console->curr_param_index; i++) {
		int param = console->esc_param[i];
		if ((param >= 30) && (param <= 37)) {  
			console->foreground = color_table[param - 30];
		} else if ((param >= 40) && (param <= 47)) {
			console->background = color_table[param - 40];
		} else if (param == 39) { 
			console->foreground = COLOR_White;
		} else if (param == 49) { 
			console->background = COLOR_Black;
		}
	}
}
static void move_right (console_t * console, int n) {
    if (n == 0) {
        n = 1;
    }

    int col = console->cur_col + n;
    if (col >= console->disp_col) {
        console->cur_col = console->disp_col - 1;
    } else {
        console->cur_col = col;
    }
}
static void move_left (console_t * console, int n) {
    if (n == 0) {
        n = 1;
    }

    int col = console->cur_col - n;
    console->cur_col = (col >= 0) ? col : 0;
}
static void move_cursor(console_t * console) {
    console->cur_row = console->esc_param[0];
    console->cur_col = console->esc_param[1];
}
static void erase_in_display(console_t * console) {
	if (console->curr_param_index < 0) {
		return;
	}

	int param = console->esc_param[0];
	if (param == 2) {
		erase_rows(console, 0, console->disp_row - 1);
        console->cur_col = console->cur_row = 0;
	}
}
static void write_esc_square (console_t * console, char c) {

    if ((c >= '0') && (c <= '9')) {
 
        int * param = &console->esc_param[console->curr_param_index];
        *param = *param * 10 + c - '0';
    } else if ((c == ';') && console->curr_param_index < ESC_PARAM_MAX) {
        console->curr_param_index++;
    } else {

        console->curr_param_index++;

 
        switch (c) {
        case 'm': 
            set_font_style(console);
            break;
        case 'D':	
            move_left(console, console->esc_param[0]);
            break;
        case 'C':
            move_right(console, console->esc_param[0]);
            break;
        case 'H':
        case 'f':
            move_cursor(console);
            break;
        case 'J':
            erase_in_display(console);
            break;
        }
        console->write_state = CONSOLE_WRITE_NORMAL;
    }
}

int console_write (tty_t * tty) {
	console_t * console = console_buf + tty->console_idx;

    int len = 0;
    mutex_lock(&console->mutex);
    do {
        char c;

        int err = tty_fifo_get(&tty->ofifo, &c);
        if (err < 0) {
            break;
        }
        sem_notify(&tty->osem);
        switch (console->write_state) {
            case CONSOLE_WRITE_NORMAL: {
                write_normal(console, c);
                break;
            }
            case CONSOLE_WRITE_ESC:
                write_esc(console, c);
                break;
            case CONSOLE_WRITE_ESC_SQUARE:
                write_esc_square(console, c);
                break;
        }
        len++;
    }while (1);
    mutex_unlock(&console->mutex);
    if(tty->console_idx == curr_console_idx)
        update_cursor_pos(console);
    return len;
}
int console_close(int console)
{
    return 0;
}
void set_col0(console_t* console)
{
    console->cur_col = 0;
}

void add_row1(console_t* console)
{
    if((++console->cur_row) >= console->disp_row)
    {
        scroll_up(console, 1);
    }
}
static void erase_rows (console_t * console, int start, int end) {
    volatile disp_char_t * disp_start = console->base + console->disp_col * start;
    volatile disp_char_t * disp_end = console->base + console->disp_col * (end + 1);

    while (disp_start < disp_end) {
        disp_start->value = ' ';
        disp_start->color = (console->foreground) | (console -> background << 4);

        disp_start++;
    }
}
static void scroll_up(console_t * console, int lines) {
    disp_char_t * dest = console->base;
    disp_char_t * src = console->base + console->disp_col * lines;
    uint32_t size = (console->disp_row - lines) * console->disp_col * sizeof(disp_char_t);
    kernel_memcpy((void*)dest, (void*)src, size);

    erase_rows(console, console->disp_row - lines, console->disp_row - 1);

    console->cur_row -= lines;
}