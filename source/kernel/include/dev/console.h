#ifndef CONSOLE_H
#define CONSOLE_H
#include "comm/types.h"
#include "ipc/mutex.h"
#include "dev/tty.h"
#define CONSOLE_BASE        0xb8000
#define CONSOLE_END         (0xb8000 + 32*1024)
#define CONSOLE_ROW         25
#define CONSOLE_COL         80
#define ASC_ESC             0x1b
#define ESC_PARAM_MAX       10
typedef enum _cclor_t {
    COLOR_Black			= 0,
    COLOR_Blue			= 1,
    COLOR_Green			= 2,
    COLOR_Cyan			= 3,
    COLOR_Red			= 4,
    COLOR_Magenta		= 5,
    COLOR_Brown			= 6,
    COLOR_Gray			= 7,
    COLOR_Dark_Gray 	= 8,
    COLOR_Light_Blue	= 9,
    COLOR_Light_Green	= 10,
    COLOR_Light_Cyan	= 11,
    COLOR_Light_Red		= 12,
    COLOR_Light_Magenta	= 13,
    COLOR_Yellow		= 14,
    COLOR_White			= 15
}cclor_t;
typedef struct _disp_char_t
{
    uint8_t value;
    uint8_t color;
     // 第0个字节表示字符，第一个字节的前四位表示前景色，后三位表示背景色
}disp_char_t;

typedef struct _console_t
{
    
    disp_char_t* base;
    enum {
        CONSOLE_WRITE_NORMAL,			
        CONSOLE_WRITE_ESC,
        CONSOLE_WRITE_ESC_SQUARE,				
    }write_state;
    int disp_row, disp_col;
    int cur_row, cur_col;
    cclor_t foreground, background;
    int save_cur_col, save_cur_row;
    int esc_param[ESC_PARAM_MAX];
    int curr_param_index;
    mutex_t mutex;
}console_t;
void console_select(int idx);
int console_init(int idx);
int console_write (tty_t * tty);
int console_close(int console_index);
static void scroll_up(console_t * console, int lines);
void set_col0(console_t* console);
void  move_forward(console_t* console, int step);
void add_row1(console_t* console);
static void erase_rows (console_t * console, int start, int end) ;
int cursor_backword(console_t* console, int len);
void erase_backword(console_t* console);
void console_clear(console_t* console);
#endif