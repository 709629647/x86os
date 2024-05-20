#include "tools/klib.h"
#include "tools/log.h"
#include "core/memory.h"
#include "tools/klib.h"
#include "cpu/mmu.h"
#include "dev/console.h"
static addr_alloc_t paddr_alloc; 
static pde_t kernel_page_dir[PDE_CNT] __attribute__((aligned(4096)));
static void addr_alloc_init(addr_alloc_t * alloc, uint8_t* bits, uint32_t start,
uint32_t size, uint32_t page_size) 
{
    mutex_init(&alloc->mutex);
    alloc -> start = start;
    alloc -> size = size;
    alloc -> page_size = page_size;
    bitmap_init(&alloc->bitmap, bits, alloc->size / page_size, 0);
}
static uint32_t addr_alloc_page(addr_alloc_t* alloc, int page_count){
    uint32_t addr = 0;
    mutex_lock(&alloc->mutex);
    int page_index = bitmap_alloc_nbit(&alloc->bitmap, page_count, 0);
    if(page_index != -1)
        addr = alloc->start + page_index * alloc->page_size;
    
    mutex_unlock(&alloc->mutex);
    return addr;
    
}

static void addr_free_page(addr_alloc_t* alloc, uint32_t addr, int page_count)
{
    mutex_lock(&alloc->mutex);
    bitmap_set_nbit(&alloc->bitmap, (addr - alloc->start)/alloc->page_size, page_count, 0);
    mutex_unlock(&alloc->mutex);
}
static void show_mem_info (boot_info_t * boot_info) {
    log_printf("mem region:");
    for (int i = 0; i < boot_info->ram_region_count; i++) {
        log_printf("[%d]: 0x%x - 0x%x", i, 
                    boot_info->ram_region_cfg[i].start,
                    boot_info->ram_region_cfg[i].size);
    }
    log_printf("\n");
}
static uint32_t total_mem_size(boot_info_t * boot_info) {
    int mem_size = 0;

    // 简单起见，暂不考虑中间有空洞的情况
    for (int i = 0; i < boot_info->ram_region_count; i++) {
        mem_size += boot_info->ram_region_cfg[i].size;
    }
    return mem_size;
}

pte_t* find_pte(pde_t* page_dir, uint32_t vstart, int alloc)
{
    pte_t * page_table;

    pde_t *pde = page_dir + pde_index(vstart);
    if (pde->present) {
        page_table = (pte_t *)pde_paddr(pde);
    } else {
        // 如果不存在，则考虑分配一个
        if (alloc == 0) {
            return (pte_t *)0;
        }

        // 分配一个物理页表
        uint32_t pg_paddr = addr_alloc_page(&paddr_alloc, 1);
        if (pg_paddr == 0) {
            return (pte_t *)0;
        }

        // 设置为用户可读写，将被pte中设置所覆盖
        pde->v = pg_paddr | PDE_P | PDE_W | PDE_U;

        // 为物理页表绑定虚拟地址的映射，这样下面就可以计算出虚拟地址了
        //kernel_pg_last[pde_index(vaddr)].v = pg_paddr | PTE_P | PTE_W;

        // 清空页表，防止出现异常
        // 这里虚拟地址和物理地址一一映射，所以直接写入
        page_table = (pte_t *)(pg_paddr);
        kernel_memset(page_table, 0, MEM_PAGE_SIZE);
    }

    return page_table + pte_index(vstart);
}

int memory_create_map(pde_t* page_dir, uint32_t vstart, uint32_t pstart, uint32_t count, uint32_t perm)
{
    
    for (int i = 0; i < count; i++) {
        // log_printf("create map: v-0x%x p-0x%x, perm: 0x%x", vstart, pstart, perm);

        pte_t * pte = find_pte(page_dir, vstart, 1);  //虚拟地址在的二级页表地址
        if (pte == (pte_t *)0) {
            log_printf("create pte failed. pte == 0");
            return -1;
        }

        // 创建映射的时候，这条pte应当是不存在的。
        // 如果存在，说明可能有问题
        // log_printf("\tpte addr: 0x%x", (uint32_t)pte);
        Assert(pte->present == 0);

        pte->v = pstart | perm | PTE_P;

        vstart += MEM_PAGE_SIZE;
        pstart += MEM_PAGE_SIZE;
    }

    return 0;
}
void create_kernel_table(void){
    extern uint8_t s_text[], e_text[], s_data[]; 
    static memory_map_t kernel_map[] = {
        {0, s_text, 0, PTE_W},
        {s_text, e_text, s_text, 0},
        {s_data, (void*)(MEM_EBDA_START - 1), s_data, PTE_W},
        {(void*)CONSOLE_BASE, (void*)CONSOLE_END, (void*)CONSOLE_BASE, PTE_W},
        {(void*)MEM_EXT_START, (void*)MEM_EXT_END, (void*)MEM_EXT_START, PTE_W}
    };
    for(int i = 0; i < sizeof(kernel_map)/sizeof(memory_map_t); ++i){
        memory_map_t* desc = kernel_map + i;

        uint32_t vstart = down2((uint32_t)desc -> vstart, MEM_PAGE_SIZE);
        uint32_t vend = up2((uint32_t)desc -> vend, MEM_PAGE_SIZE);
        uint32_t pstart = down2((uint32_t)desc -> pstart, MEM_PAGE_SIZE);
        uint32_t page_count = (vend - vstart) / MEM_PAGE_SIZE;
        memory_create_map(kernel_page_dir, vstart, pstart, page_count, desc->perm);
    
    }
}
/**
 * @brief 初始化内存管理系统
 * 该函数的主要任务：
 * 1、初始化物理内存分配器：将所有物理内存管理起来. 在1MB内存中分配物理位图
 * 2、重新创建内核页表：原loader中创建的页表已经不再合适
 */
void memory_init (boot_info_t * boot_info) {
    // 1MB内存空间起始，在链接脚本中定义
    extern uint8_t * mem_free_start; 

    log_printf("mem init.");
    show_mem_info(boot_info);

    // 在内核数据后面放物理页位图
    uint8_t * mem_free = (uint8_t *)&mem_free_start;   // 2022年-10-1 经同学反馈，发现这里有点bug，改了下

    // 计算1MB以上空间的空闲内存容量，并对齐的页边界
    uint32_t mem_up1MB_free = total_mem_size(boot_info) - MEM_EXT_START;
    mem_up1MB_free = down2(mem_up1MB_free, MEM_PAGE_SIZE);   // 对齐到4KB页
    // log_printf("Free memory: 0x%x, size: 0x%x", MEM_EXT_START, mem_up1MB_free);

    // 4GB大小需要总共4*1024*1024*1024/4096/8=128KB的位图, 使用低1MB的RAM空间中足够
    // 该部分的内存仅跟在mem_free_start开始放置
    addr_alloc_init(&paddr_alloc, mem_free, MEM_EXT_START, mem_up1MB_free, MEM_PAGE_SIZE);
    mem_free += bit_count(paddr_alloc.size / MEM_PAGE_SIZE);

    // 到这里，mem_free应该比EBDA地址要小
    Assert(mem_free < (uint8_t *)MEM_EBDA_START);
    create_kernel_table();
    mmu_set_page_dir((uint32_t)kernel_page_dir);
}
//这个函数用于将不同进程的前0x80000000的虚拟内存与内核的虚拟内存共享
uint32_t memory_create_uvm()
{
    pde_t* page_dir = (pde_t*)addr_alloc_page(&paddr_alloc, 1);
    if(page_dir == (pde_t*)0)
        return 0;
    kernel_memset((void*)page_dir, 0, MEM_PAGE_SIZE);
    uint32_t user_pd_index = pde_index(MEMORY_TASK_BASE);
    for(uint32_t i = 0; i < user_pd_index; ++i)
    {
        (page_dir + i)->v = (kernel_page_dir + i)->v;
    }
    return (uint32_t)page_dir;
}
//分配进程使用的虚拟地址
int memory_alloc_for_page(uint32_t addr, uint32_t size, uint32_t perm)
{
    return memory_alloc_for_page_dir(task_manager.curr_task->tss.cr3, addr, size, perm);
}

int memory_alloc_for_page_dir(uint32_t page_dir, uint32_t vaddr, uint32_t size, uint32_t perm)
{
    uint32_t curr_vaddr = vaddr;
    uint32_t page_count = up2(size, MEM_PAGE_SIZE) / MEM_PAGE_SIZE;
    for(int i = 0; i < page_count; ++i){
        uint32_t paddr = addr_alloc_page(&paddr_alloc, 1);
        if(paddr == 0){
            log_printf("no mem");
            return 0;
        }
        int err = memory_create_map((pde_t*)page_dir, vaddr, paddr, 1, perm);
        if(err < 0){
            log_printf("cannot create map");
            // addr_free_page(&paddr, paddr, 1);
            return 0;
        }
        vaddr += MEM_PAGE_SIZE;
    } 
    return 0;
}
//用于分配0X80000000以下的地址，一一映射。
uint32_t memory_alloc_page(void)
{
    return addr_alloc_page(&paddr_alloc, 1);
}

void  memory_free_page(uint32_t addr)
{
    if(addr < MEMORY_TASK_BASE){
        addr_free_page(&paddr_alloc, addr, 1);
    }
    else{
        pte_t* pg_table = find_pte((pde_t*)curr_page_dir(), addr, 0);
        Assert(pg_table != 0 && pg_table->present);
        addr_free_page(&paddr_alloc, pte_paddr(pg_table), 1);
        pg_table->v = 0;
    }
}

uint32_t curr_page_dir(void){
    return task_manager.curr_task->tss.cr3;
}

uint32_t memory_copy_uvm (uint32_t page_dir) {
    uint32_t to_page_dir = memory_create_uvm();
    if (to_page_dir == 0) {
        goto copy_uvm_failed;
    }

    uint32_t user_pde_start = pde_index(MEMORY_TASK_BASE);
    pde_t * pde = (pde_t *)page_dir + user_pde_start;

    for (int i = user_pde_start; i < PDE_CNT; i++, pde++) {
        if (!pde->present) {
            continue;
        }

        pte_t * pte = (pte_t *)pde_paddr(pde);
        for (int j = 0; j < PTE_CNT; j++, pte++) {
            if (!pte->present) {
                continue;
            }

            uint32_t page = addr_alloc_page(&paddr_alloc, 1);
            if (page == 0) {
                goto copy_uvm_failed;
            }

            uint32_t vaddr = (i << 22) | (j << 12);
            int err = memory_create_map((pde_t *)to_page_dir, vaddr, page, 1, get_pte_perm(pte));
            if (err < 0) {
                goto copy_uvm_failed;
            }

            kernel_memcpy((void *)page, (void *)vaddr, MEM_PAGE_SIZE);
        }
    }
    return to_page_dir;

copy_uvm_failed:
    if (to_page_dir) {
        memory_destroy_uvm(to_page_dir);
    }
    return -1;
}

void memory_destroy_uvm(uint32_t page_dir)
{
    uint32_t to_page_dir = memory_create_uvm();
    pde_t* pde = (pde_t*)page_dir  + pde_index(MEMORY_TASK_BASE);
    for(int i = pde_index(MEMORY_TASK_BASE); i < PDE_CNT; ++i, ++pde)
    {
        if(!pde -> present)
            continue;
        pte_t* pte = (pte_t*)pde_paddr(pde);
        for(int j = 0; j < PTE_CNT; ++j, ++pte)
        {
            if(!pte -> present)
                continue;
            addr_free_page(&paddr_alloc, (uint32_t)pte_paddr(pte), 1);
        }
        addr_free_page(&paddr_alloc, (uint32_t)pde_paddr(pde), 1);
    }
    addr_free_page(&paddr_alloc, page_dir, 1);
}

uint32_t memory_get_paddr(uint32_t page_dir, uint32_t vaddr)
{
    pte_t* pte = find_pte((pde_t*)page_dir, vaddr, 0);
    if (pte == (pte_t *)0) {
        return 0;
    }
    return (pte_paddr(pte) + (vaddr & (MEM_PAGE_SIZE - 1)));
}

int memory_copy_uvm_data(uint32_t to, uint32_t page_dir, uint32_t from, uint32_t size)
{
    
    while(size > 0)
    {
        uint32_t paddr = memory_get_paddr(page_dir, to);
        if(paddr == 0) 
            return -1;
        uint32_t offset_in_page = paddr & (MEM_PAGE_SIZE - 1);
        uint32_t curr_size = MEM_PAGE_SIZE - offset_in_page;
        if(curr_size > size)
            curr_size = size;
        kernel_memcpy((void*)paddr, (void*)from, curr_size);
        size -= curr_size;
        to += curr_size;
        from += curr_size;
    }
    return 0;
}

char * sys_sbrk(int incr) {
    task_t * task = task_manager.curr_task;
    char * pre_heap_end = (char * )task->heap_end;
    int pre_incr = incr;

    Assert(incr >= 0);

    if (incr == 0) {
        log_printf("sbrk(0): end = 0x%x", pre_heap_end);
        return pre_heap_end;
    } 
    
    uint32_t start = task->heap_end;
    uint32_t end = start + incr;

    int start_offset = start % MEM_PAGE_SIZE;
    if (start_offset) {

        if (start_offset + incr <= MEM_PAGE_SIZE) {
            task->heap_end = end;
            return pre_heap_end;
        } else {

            uint32_t curr_size = MEM_PAGE_SIZE - start_offset;
            start += curr_size;
            incr -= curr_size;
        }
    }

    if (incr) {
        uint32_t curr_size = end - start;
        int err = memory_alloc_for_page (start, curr_size, PTE_P | PTE_U | PTE_W);
        if (err < 0) {
            log_printf("sbrk: alloc mem failed.");
            return (char *)-1;
        }
    }

    // log_printf("sbrk(%d): end = 0x%x", pre_incr, end);
    task->heap_end = end;
    return (char * )pre_heap_end;        
}

