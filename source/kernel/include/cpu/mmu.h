#include "comm/types.h"
#include "comm/cpu_instr.h"
#define PTE_CNT       1024
#define PDE_CNT       1024
#define PTE_P       (1 << 0)    //该表项是否可用
#define PDE_P       (1 << 0)
#define PDE_W       (1 << 1)    //4MB是否可写
#define PTE_W       (1 << 1)    //4KB是否可写
#define PDE_U       (1 << 2)    //PDE_u =1代表，4MB可用户模式访问
#define PTE_U       (1 << 2)    //PTE_U = 1代表，4KB可用户模式访问
#pragma pack(1)
/**
 * @brief Page-Table Entry
 */
typedef union _pde_t {
    uint32_t v;
    struct {
        uint32_t present : 1;                   // 0 (P) Present; must be 1 to map a 4-KByte page
        uint32_t write_disable : 1;             // 1 (R/W) Read/write, if 0, writes may not be allowe
        uint32_t user_mode_acc : 1;             // 2 (U/S) if 0, user-mode accesses are not allowed t
        uint32_t write_through : 1;             // 3 (PWT) Page-level write-through
        uint32_t cache_disable : 1;             // 4 (PCD) Page-level cache disable
        uint32_t accessed : 1;                  // 5 (A) Accessed
        uint32_t : 1;                           // 6 Ignored;
        uint32_t ps : 1;                        // 7 (PS)
        uint32_t : 4;                           // 11:8 Ignored
        uint32_t phy_pt_addr : 20;              // 高20位page table物理地址
    };
}pde_t;

/**
 * @brief Page-Table Entry
 */
typedef union _pte_t {
    uint32_t v;
    struct {
        uint32_t present : 1;                   // 0 (P) Present; must be 1 to map a 4-KByte page
        uint32_t write_disable : 1;             // 1 (R/W) Read/write, if 0, writes may not be allowe
        uint32_t user_mode_acc : 1;             // 2 (U/S) if 0, user-mode accesses are not allowed t
        uint32_t write_through : 1;             // 3 (PWT) Page-level write-through
        uint32_t cache_disable : 1;             // 4 (PCD) Page-level cache disable
        uint32_t accessed : 1;                  // 5 (A) Accessed;
        uint32_t dirty : 1;                     // 6 (D) Dirty
        uint32_t pat : 1;                       // 7 PAT
        uint32_t global : 1;                    // 8 (G) Global
        uint32_t : 3;                           // Ignored
        uint32_t phy_page_addr : 20;            // 高20位物理地址
    };
}pte_t;

#pragma pack()

static inline void mmu_set_page_dir(uint32_t page_dir)
{
    write_cr3(page_dir);
}

static inline uint32_t pde_index(uint32_t vaddr){
    return  (vaddr >> 22);
}

static inline uint32_t pte_index(uint32_t vaddr){
    return  (vaddr >> 12) & 0x3ff;
}
static inline uint32_t pde_paddr(pde_t* desc)
{
    return (desc->phy_pt_addr << 12);
}
static inline uint32_t pte_paddr(pte_t* desc)
{
    return (desc -> phy_page_addr << 12);
}

static inline uint32_t get_pte_perm(pte_t* desc)
{
    return (desc->v & 0x3FF);
}