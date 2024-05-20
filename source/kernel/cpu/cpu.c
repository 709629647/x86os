#include "cpu/cpu.h"
#include "cpu/irq.h"
#include "ipc/mutex.h"
#include "core/syscall.h"
static segment_desc_t gdt_table[GDT_TABLE_SIZE];
static mutex_t mutex;
void segment_desc_set(int selector, uint32_t base, uint32_t limit, uint16_t attr){
    segment_desc_t* desc = gdt_table + (selector >> 3); //segment_desc_t是8个字节
    if(limit > 0xfffff){
        attr |= 0x8000;
        limit /= 0x1000;
    }
    desc -> limit15_0 = limit & 0xFFFF;
    desc -> base15_0 = base & 0xFFFF;
    desc -> base23_16 = (base >> 16) & 0xFF;
    desc -> attr = attr |(((limit >> 16) &0xF) << 8);
    desc -> base31_24 = (base >> 24) & 0xFF;
}
void gate_desc_set(gate_desc_t* desc, uint16_t selector, uint32_t offset, uint16_t attr)
{
    desc -> selector = selector;
    desc -> offset15_0 = offset & 0xffff;
    desc -> attr = attr;
    desc -> offset31_16 = (offset >> 16) &0xFFFF;
}
void init_gdt(void){
    for(int i = 0; i < GDT_TABLE_SIZE; ++i)
        segment_desc_set(i * sizeof(segment_desc_set), 0, 0, 0);

    segment_desc_set(KERNEL_SELECTOR_CS, 0x00000000, 0xFFFFFFFF, 
    (SEG_P_PRESENT | SEG_DPL0 | SEG_S_NORMAL | SEG_TYPE_CODE
                     | SEG_TYPE_RW | SEG_D | SEG_G)
    );
    segment_desc_set(KERNEL_SELECTOR_DS, 0x00000000, 0xffffffff, 
    (SEG_P_PRESENT | SEG_DPL0 | SEG_S_NORMAL | SEG_TYPE_DATA
                     | SEG_TYPE_RW | SEG_D | SEG_G)
    );
    gate_desc_set((gate_desc_t*)(gdt_table + (SYSTEM_SELECTOR >> 3)), KERNEL_SELECTOR_CS,
    (uint32_t)exception_handler_syscall,
    GATE_P_PRESENT | GATE_DPL3 |  GATE_TYPE_SYSCALL | SYSCALL_PARAM
    );

    
    lgdt((uint32_t)gdt_table, sizeof(gdt_table));
}

void cpu_init(void){
    mutex_init(&mutex);
    init_gdt();
}

int gdt_alloc_desc(void){
    mutex_lock(&mutex);
    for(int i = 1; i < GDT_TABLE_SIZE; ++i){
        segment_desc_t * desc = gdt_table;
        if((desc + i) -> attr == 0)
        {
            mutex_unlock(&mutex);
            return i * sizeof(segment_desc_t);
        }
            
    }
    mutex_unlock(&mutex);
}

void gdt_free_tss(int selector)
{
    mutex_lock(&mutex);
    gdt_table[selector / sizeof(segment_desc_t)].attr = 0;
    mutex_unlock(&mutex);
}