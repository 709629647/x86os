#ifndef DEV_H
#define DEV_H
#define DEV_MAX_SIZE            32
enum{
    UNKNOWN,
    DEV_TTY,
    DEV_DISK,
    
};
struct _dev_desc_t;
typedef struct _device_t
{
    struct _dev_desc_t* desc;
    int minor;
    int mode;
    void* data;
    int open_count;
}device_t;

typedef struct _dev_desc_t
{
    char name[DEV_MAX_SIZE];
    int major;
    int (*open)(device_t* dev);
    int (*read)(device_t* dev, int adrr, char* buf, int size);
    int (*write)(device_t* dev, int adrr, char* buf, int size);
    void (*close)(device_t* dev);
    int (*control)(device_t* dev, int cmd, int arg0, int arg1);
}dev_desc_t;

int dev_open (int major, int minor, void * data) ;
/**
 * @brief 读取指定字节的数据
 */
int dev_read (int dev_id, int addr, char * buf, int size) ;
int dev_write (int dev_id, int addr, char * buf, int size) ;
int dev_control (int dev_id, int cmd, int arg0, int arg1) ;
void dev_close (int dev_id) ;

#endif