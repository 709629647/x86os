#ifndef FS_H
#define FS_H
#include "comm/types.h"
#include "comm/cpu_instr.h"
#include "comm/boot_info.h"
#include <sys/stat.h>
#include "ipc/mutex.h"
#include "fatfs/fatfs.h"
#include "applib/lib_syscall.h"
#define FS_MOUNTP_SIZE  		128
struct _fs_t;
typedef struct _fs_op_t {
	int (*mount) (struct _fs_t * fs,int major, int minor);
    void (*unmount) (struct _fs_t * fs);
    int (*open) (struct _fs_t * fs, const char * path, file_t * file);
    int (*read) (char * buf, int size, file_t * file);
    int (*write) (char * buf, int size, file_t * file);
    void (*close) (file_t * file);
    int (*seek) (file_t * file, uint32_t offset, int dir);
    int (*stat)(file_t * file, struct stat *st);
    int (*ioctl)(file_t * file, int cmd, int arg0, int arg1);
    int (*opendir) (struct _fs_t * fs, const char* name, DIR* dir);
    int (*readdir) (struct _fs_t * fs, DIR* dir, struct dirent* dirent);
    int (*closedir) (struct _fs_t * fs, DIR* dir);
    int (*unlink)   (struct _fs_t * fs, const char* name);
}fs_op_t;

typedef enum _fs_type_t{
	FS_DEVFS,
    FS_FAT16,
}fs_type_t;
struct _mutex_t;
typedef struct _fs_t {
    char mount_point[FS_MOUNTP_SIZE];     //挂载点
    fs_type_t type;             		//文件系统类型

    fs_op_t * op;               // 回调函数表的指针
    void * data;              
    int dev_id;               

    list_node_t node;         
    struct _mutex_t*  mutex;
    union{
        fat_t fat_data;
    };                 
}fs_t;


static void read_disk(int sector, int sector_count, uint8_t * buf) {
    outb(0x1F6, (uint8_t) (0xE0));

	outb(0x1F2, (uint8_t) (sector_count >> 8));
    outb(0x1F3, (uint8_t) (sector >> 24));		// LBA参数的24~31位
    outb(0x1F4, (uint8_t) (0));					// LBA参数的32~39位
    outb(0x1F5, (uint8_t) (0));					// LBA参数的40~47位

    outb(0x1F2, (uint8_t) (sector_count));
	outb(0x1F3, (uint8_t) (sector));			// LBA参数的0~7位
	outb(0x1F4, (uint8_t) (sector >> 8));		// LBA参数的8~15位
	outb(0x1F5, (uint8_t) (sector >> 16));		// LBA参数的16~23位

	outb(0x1F7, (uint8_t) 0x24);

	// 读取数据
	uint16_t *data_buf = (uint16_t*) buf;
	while (sector_count-- > 0) {
		// 每次扇区读之前都要检查，等待数据就绪
		while ((inb(0x1F7) & 0x88) != 0x8) {}

		// 读取并将数据写入到缓存中
		for (int i = 0; i < SECTOR_SIZE / 2; i++) {
			*data_buf++ = inw(0x1F0);
		}
	}
}
static int is_file_bad(int file);
int sys_open(char* name, int flag, ...);
int sys_read(int file, char* buf, int size);
int sys_write(int file, char* buf, int size);
int sys_lseek(int file, int ptr, int dir);
int sys_close(int fd);
int sys_close(int file);
int sys_dup (int file);
int sys_isatty(int file);
int sys_fstat(int file, struct stat *st);
void fs_init(void);
int sys_opendir(const char* name, DIR* dir);
int sys_readdir(DIR* dir, struct dirent* dirent);
int sys_closedir(DIR* dir);
int sys_ioctl(int file, int cmd, int arg0, int arg1);
int path_to_num (const char * path, int * num);
const char * path_next_child (const char * path);
int sys_unlink(const char* name);
#endif