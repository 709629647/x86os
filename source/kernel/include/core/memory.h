#ifndef MEMORY_H
#define MEMORY_H

#include "tools/bitmap.h"
#include "comm/boot_info.h"
#include "ipc/mutex.h"


#define MEM_EXT_START               (1024*1024) 
#define MEM_PAGE_SIZE               4096    
#define MEM_EBDA_START              0x00080000
#define MEMORY_TASK_BASE            0x80000000
#define MEM_EXT_END                 (127 * 1024* 1024)
#define MEM_STACK_TOP               0xE0000000
#define MEM_STACK_SIZE              (500 * MEM_PAGE_SIZE)
#define MEM_ARG_SIZE                (4 * MEM_PAGE_SIZE)
typedef struct _addr_alloc_t
{
    mutex_t mutex;
    uint32_t start;
    uint32_t size;
    uint32_t page_size;
    bitmap_t bitmap;
}addr_alloc_t;

typedef struct _memory_map_t
{
    void* vstart;
    void* vend;
    void* pstart;
    uint32_t perm;
}memory_map_t;
int memory_copy_uvm_data(uint32_t to, uint32_t page_dir, uint32_t from, uint32_t size);
void memory_init(boot_info_t* boot_info);
static uint32_t addr_alloc_page(addr_alloc_t* alloc, int page_count);
uint32_t memory_create_uvm();
int memory_alloc_for_page(uint32_t addr, uint32_t size, uint32_t perm);
static void addr_free_page(addr_alloc_t* alloc, uint32_t addr, int page_count);
int memory_alloc_for_page_dir(uint32_t page_dir, uint32_t vaddr, uint32_t size, uint32_t perm);
uint32_t memory_alloc_page(void);
void  memory_free_page(uint32_t addr);
uint32_t curr_page_dir(void);
uint32_t memory_copy_uvm(uint32_t page_dir);
void memory_destroy_uvm(uint32_t page_dir);
uint32_t memory_get_paddr(uint32_t page_dir, uint32_t vaddr);
char * sys_sbrk(int incr);
#endif