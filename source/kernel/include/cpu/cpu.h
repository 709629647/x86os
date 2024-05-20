#ifndef CPU_H
#define CPU_H
#include "comm/types.h"
#include "os_cfg.h"
#include "comm/cpu_instr.h"

#define EFLAGS_DEFAULT      (1 << 1)
#define EFLAGS_IF           (1  << 9)
#pragma pack(1)
typedef struct _segment_desc
{
    /* data */
    uint16_t limit15_0;
    uint16_t base15_0;
    uint8_t base23_16;
    uint16_t attr;
    uint8_t base31_24;
} segment_desc_t;

typedef struct _gate_desc
{
    /* data */
    uint16_t offset15_0;
    uint16_t selector;
    uint16_t attr;
    uint16_t offset31_16;
} gate_desc_t;

typedef struct _tss_t {
    uint32_t pre_link;
    uint32_t esp0, ss0, esp1, ss1, esp2, ss2;
    uint32_t cr3;
    uint32_t eip, eflags, eax, ecx, edx, ebx, esp, ebp, esi, edi;
    uint32_t es, cs, ss, ds, fs, gs;
    uint32_t ldt;
    uint32_t iomap;
}tss_t;

#pragma pack()

#define SEG_G           (1 << 15)   //G =1 limit以4kB为单位
#define SEG_D           (1 << 14)   //与32位和16位有关
#define SEG_P_PRESENT   (1 << 7)    //段是否存在
#define SEG_DPL0        (0 << 5)    //DPL = 0
#define SEG_DPL3        (3 << 5) //DPL = 3
#define SEG_CPL0        (0 << 0)
#define SEG_CPL3        (3 << 0)
#define SEG_S_SYSTEM    (0 << 4)    //系统段
#define SEG_S_NORMAL    (1 << 4)    //代码段或数据段
#define SEG_TYPE_CODE   (1 << 3)    //代码段
#define SEG_TYPE_DATA  (0 << 3)    //数据段
#define SEG_TYPE_RW     (1 << 1)    //权限可读写
#define SEG_TYPE_TSS    (9 << 0)
void segment_desc_set(int selector, uint32_t base, uint32_t limit, uint16_t attr);
void cpu_init(void);
void gate_desc_set(gate_desc_t* desc, uint16_t selector, uint32_t offset, uint16_t attr);
int gdt_alloc_desc(void);
void gdt_free_tss(int selector);
#endif