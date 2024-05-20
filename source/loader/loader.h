#ifndef LOADER_H
#define LOADER_H

#include  "comm/boot_info.h"
#include "comm/types.h"
#include "comm/cpu_instr.h"
void _protect_mode_entry(void);
typedef struct  SMAP_entry
{
    uint32_t BaseL; //base address uint64_t
    uint32_t BaseH;
    uint32_t LengthL;//length uint64_t
    uint32_t LengthH;
    uint32_t Type; //entry type ,值为1表示为我们可用的RAM空间
    uint32_t ACPI; //extended bit0 = 0表示此条目可以被忽略
    /* data */
}__attribute__((packed)) SMAP_entry_t;
extern boot_info_t boot_info;
#endif