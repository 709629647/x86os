#include "dev/dev.h"
#include "dev/tty.h"
#include "cpu/irq.h"
#include "tools/klib.h"
#define     DEV_TBL_SIZE            128
extern dev_desc_t dev_tty_desc;
extern dev_desc_t dev_disk_desc;
static dev_desc_t* dev_desc_tbl[] = {
    &dev_tty_desc,
    &dev_disk_desc,
};
static device_t dev_tbl[DEV_TBL_SIZE];

static int is_devid_bad (int dev_id) {
    if ((dev_id < 0) || (dev_id >=  sizeof(dev_tbl) / sizeof(dev_tbl[0]))) {
        return 1;
    }

    if (dev_tbl[dev_id].desc == (dev_desc_t *)0) {
        return 1;
    }

    return 0;
}


/**
 * @brief 打开指定的设备
 */
int dev_open (int major, int minor, void * data) {
    device_t* free = (device_t*)0;
    irq_state_t state =  enter_protection();
    for(int i = 0; i < (sizeof(dev_tbl)/ sizeof(device_t)); ++i)
    {
        device_t* device = dev_tbl + i;
        if(device->open_count == 0)
        {
            free = device;
            // break;
        }
        else{
            if(device -> desc->major == major && device->minor == minor)
            {
                device->open_count ++;
                leave_protection(state);
                return i;
            }
        }
    }

    dev_desc_t* desc = (dev_desc_t*)0;
    for(int i = 0; i < (sizeof(dev_desc_tbl)/ sizeof(dev_desc_tbl[0])); ++i)
    {   dev_desc_t* dev_desc = dev_desc_tbl[i];
        if(dev_desc -> major == major)
        {
            desc = dev_desc;
        }
    }
    if(desc && free)
    {
        free -> minor = minor;
        free -> data =data;
        free -> desc = desc;
        int err = desc->open(free);
        if(err == 0)
        {
            free -> open_count = 1;
            leave_protection(state);
            return free - dev_tbl;
        }
        
    }
    leave_protection(state);
    return -1;
}

/**
 * @brief 读取指定字节的数据
 */
int dev_read (int dev_id, int addr, char * buf, int size) {
    if(is_devid_bad(dev_id))
        return -1;
    device_t* device = dev_tbl + dev_id;
    
    return device->desc->read(device, addr, buf, size);
}

/**
 * @brief 写指定字节的数据
 */
int dev_write (int dev_id, int addr, char * buf, int size) {
    if(is_devid_bad(dev_id))
        return -1;
    device_t* device = dev_tbl + dev_id;
    
    return device->desc->write(device, addr, buf, size);
}

/**
 * @brief 发送控制命令
 */

int dev_control (int dev_id, int cmd, int arg0, int arg1) {
    if(is_devid_bad(dev_id))
        return -1;
    device_t* device = dev_tbl + dev_id;
    
    return device->desc->control(device, cmd, arg0, arg1);
}

/**
 * @brief 关闭设备
 */
void dev_close (int dev_id) {
    if(is_devid_bad(dev_id))
        return;
    irq_state_t state = enter_protection();
    device_t* device = dev_tbl + dev_id;;
    if(--device->open_count == 0)
    {
        device->desc->close(device);
        kernel_memset((void*)device, 0, sizeof(device_t));
    }
    leave_protection(state);
    return ;
}


