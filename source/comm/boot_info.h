#ifndef BOOT_INFO_H
#define BOOT_INFO_H
#include "types.h"
#define BOOT_RAM_REGION_MAX 10
typedef struct _boot_info_t{
    struct 
    {
       uint32_t start;
       uint32_t size;
    }ram_region_cfg[BOOT_RAM_REGION_MAX];//描述每一块内存的信息
    
    int ram_region_count;//指示ram_region_cfg有多少个项目是有效的
    
}boot_info_t;

#define SECTOR_SIZE 512
#define SYS_KERNEL_LOAD_ADDR (1024*1024)

#endif