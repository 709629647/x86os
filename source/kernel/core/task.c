#include "core/task.h"
#include "os_cfg.h"
#include "tools/klib.h"
#include "cpu/cpu.h"
#include "comm/cpu_instr.h"
#include "cpu/irq.h"
#include "core/memory.h"
#include "cpu/mmu.h"
#include "ipc/mutex.h"
#include "core/syscall.h"
#include "fs/fs.h"
static uint32_t* idle_stack[IDLE_STACK_SIZE];
static mutex_t task_table_mutex;
static task_t task_table[TASK_NUM];

file_t * task_file (int fd) {
    if ((fd >= 0) && (fd < TASK_OFILE_NR)) {
        file_t * file =  task_manager.curr_task->file_table[fd];
        return file;
    }

    return (file_t *)0;
}


int task_alloc_fd (file_t * file) {
    task_t * task = task_manager.curr_task;

    for (int i = 0; i < TASK_OFILE_NR; i++) {
        file_t * p = task->file_table[i];
        if (p == (file_t *)0) {
            task->file_table[i] = file;
            return i;
        }
    }

    return -1;
}


void task_remove_fd (int fd) {
    if ((fd >= 0) && (fd < TASK_OFILE_NR)) {
         task_manager.curr_task->file_table[fd] = (file_t *)0;
    }
}
static int  tss_init(task_t* task, uint32_t entry, uint32_t esp, int flag)
{
    
    kernel_memset(&(task -> tss), 0, sizeof(tss_t));
    int selector = gdt_alloc_desc();
    if (selector < 0) {
        log_printf("alloc tss failed.\n");
        return -1;
    }
    segment_desc_set(selector, (uint32_t)&task->tss, sizeof(tss_t), 
    (SEG_P_PRESENT| SEG_DPL0 | SEG_TYPE_TSS));
    kernel_memset(&task->tss, 0, sizeof(tss_t));
    uint32_t kernel_page = memory_alloc_page();
    if(kernel_page == 0)
        goto tss_init_fault;
    int code_sel, data_sel;
    if(flag & TASK_SYSTEM_FLAG)
    {
        code_sel = KERNEL_SELECTOR_CS;
        data_sel = KERNEL_SELECTOR_DS;
    }
    else {
        code_sel = task_manager.app_code_sel | SEG_CPL3; //设置选择子的 RPL位
        data_sel = task_manager.app_data_sel | SEG_CPL3;
    }
    
    task -> tss.esp =  esp;
    task -> tss.esp0 = kernel_page + MEM_PAGE_SIZE;
    task -> tss.ds  = task -> tss.es = task -> tss.fs = task -> tss.gs = data_sel;
    task -> tss.eip = entry;
    task -> tss.eflags =  EFLAGS_DEFAULT | EFLAGS_IF;
    task -> tss.ss =  data_sel;
    task -> tss.ss0 = KERNEL_SELECTOR_DS;
    task -> tss.cs = code_sel;
    task -> tss_selector = selector;
    uint32_t page_dir = memory_create_uvm();
    if(page_dir == 0)
    {
        goto tss_init_fault;
    }
    task-> tss.cr3 = page_dir;
    return 0;
tss_init_fault:
    gdt_free_tss(selector);
    if(kernel_page)
        memory_free_page(kernel_page);
    return -1;
    
}

int  task_init(task_t* task, const char* name, uint32_t entry, uint32_t esp, int flag)
{
    int err = tss_init(task, entry, esp, flag);
    if (err < 0) {
        log_printf("init task failed.\n");
        return err;
    }
    task->pid = (uint32_t) task;
    task->heap_start = 0;
    task->heap_end = 0;
    task->parent = (task_t*)0;
    task->sleep_ticks = 0;
    task->time_ticks = TASK_TIME_TICKS;
    task->slice_ticks = task->time_ticks;
    task->status = 0;
    kernel_strncpy(task->name, name, TASK_NAME_SIZE);
    task->state = CREATE;
    list_node_init(&task->all_node);
    list_node_init(&task->wait_node);
    list_node_init(&task->run_node);
    irq_state_t state =  enter_protection();
    // 插入就绪队列中和所有的任务队列中
    
    list_insert_last(&task_manager.task_list, &task->all_node);
    leave_protection(state);
    return 0;
    // return;
    // uint32_t* pesp = (uint32_t*)esp;
    // if(pesp){
    //     *(--pesp) = entry;
    //     *(--pesp) = 0;
    //     *(--pesp) = 0;
    //     *(--pesp) = 0;
    //     *(--pesp) = 0;
    //     task->stack = pesp;
    // }
}

void task_start(task_t* task)
{
    irq_state_t state =  enter_protection();
    set_ready(task);
    leave_protection(state);
}
void task_uninit(task_t* task)
{
    if(task->tss.esp0)
        memory_free_page(task->tss.esp0 - MEM_PAGE_SIZE);
    if(task->tss_selector)
        gdt_free_tss(task->tss_selector);
    if(task->tss.cr3)
        memory_destroy_uvm(task->tss.cr3);
    kernel_memset((void*)task, 0, sizeof(task_t));
    

}

void task_switch_from_to(task_t* from, task_t* to){
    // log_printf("task switch -----\r\n");
    far_jump(to->tss_selector, 0);
    // simple_switch(&from->stack, to->stack);
}

void idle_task_func(void){
    for(;;){
        hlt();
    }
}
void task_manager_init(void)
{
    kernel_memset((void*)task_table, 0, sizeof(task_table));
    mutex_init(&task_table_mutex);
    uint32_t sel =  gdt_alloc_desc();
    segment_desc_set(sel, (uint32_t)0, 0xFFFFFFFF, 
    SEG_DPL3|SEG_P_PRESENT|SEG_S_NORMAL|SEG_TYPE_CODE|SEG_TYPE_RW|SEG_D);
    task_manager.app_code_sel = sel;
    sel = gdt_alloc_desc();
    segment_desc_set(sel, (uint32_t)0, 0xFFFFFFFF, 
    SEG_DPL3|SEG_P_PRESENT|SEG_S_NORMAL|SEG_TYPE_DATA|SEG_TYPE_RW|SEG_D);
    task_manager.app_data_sel = sel;
    list_init(&(task_manager.ready_list));
    list_init(&(task_manager.task_list));
    list_init(&task_manager.sleep_list);
    task_manager.curr_task = (task_t*) 0;
    task_init(&task_manager.idle_task, 
    "idle_task", (uint32_t)idle_task_func, (uint32_t)&idle_stack[1023], TASK_SYSTEM_FLAG);
    task_start(&task_manager.idle_task);
}


void first_task_init(void)
{
    extern void _first_task_entry(void);
    extern uint8_t s_first_task[], e_first_task[];
    uint32_t first_task = (uint32_t)_first_task_entry;
    uint32_t size = (uint32_t)(e_first_task - s_first_task);
    uint32_t alloc_size = 10 * MEM_PAGE_SIZE;
    Assert(alloc_size > size);
    task_init(&task_manager.first_task, "init_main", first_task, first_task + alloc_size, 0);
    task_manager.first_task.heap_start = (uint32_t)e_first_task;
    task_manager.first_task.heap_end = (uint32_t)e_first_task;
    task_manager.curr_task = &task_manager.first_task;
    mmu_set_page_dir(task_manager.curr_task->tss.cr3);
    memory_alloc_for_page(first_task, alloc_size, PTE_P|PTE_W|PTE_U);
    kernel_memcpy((void*)first_task, (void*)s_first_task, size);
    ltr(task_manager.first_task.tss_selector);
    task_start(&task_manager.first_task);
}

task_t* task_first_task(void)
{
    return &task_manager.first_task;
}

void set_ready(task_t* task){
    if(task == &task_manager.idle_task)
        return;
    list_insert_last(&task_manager.ready_list, &task->run_node);
    task->state = READY;
}

void set_block(task_t* task)
{
    if(task == &task_manager.idle_task)
        return;
    list_remove(&task_manager.ready_list, &task->run_node);
}
/**
 * @brief 当前任务主动放弃CPU
 */
void sys_shed_yield(void)
{
    irq_state_t state =  enter_protection();
    if(list_count(&task_manager.ready_list) > 1){
        task_t * curr = current_task();
        // 如果队列中还有其它任务，则将当前任务移入到队列尾部
        set_block(curr);
        set_ready(curr);
        // 切换至下一个任务，在切换完成前要保护，不然可能下一任务
        // 由于某些原因运行后阻塞或删除，再回到这里切换将发生问题
        task_dispatch();
        
    }
    leave_protection(state);

    
}
task_t* task_next_run(void){
    if(list_count(&task_manager.ready_list) == 0)
        return &task_manager.idle_task;

    return LIST_NODE_PARENT(list_first(&task_manager.ready_list), task_t, run_node);
}
void task_dispatch(void){
    // irq_state_t state =  enter_protection();
    task_t* to = task_next_run();
    if(to != task_manager.curr_task){
        task_t * from = task_manager.curr_task;
        task_manager.curr_task = to;
        to -> state = RUNNING;
        // leave_protection(state);
        task_switch_from_to(from, to);
        
    }
    // leave_protection(state);
}
task_t * current_task(void){
    return task_manager.curr_task;
}

void task_time_tick(void)
{
    
    task_t* curr_task = current_task();
    irq_state_t state =  enter_protection();
    if(--curr_task->slice_ticks == 0){
        curr_task->slice_ticks = curr_task->time_ticks;
        // 调整队列的位置到尾部，不用直接操作队列
        set_block(curr_task);
        set_ready(curr_task);
    }

    
    list_node_t* curr = list_first(&task_manager.sleep_list);
    while(curr){
        list_node_t* next = curr->next;
        task_t* temp =  LIST_NODE_PARENT(curr, task_t, run_node);
        if((--temp->sleep_ticks == 0))
        {
            set_wake(temp);
            set_ready(temp);
        }
        curr = next;
    }
    leave_protection(state);
    task_dispatch();
}

void set_sleep(task_t* task, int ticks)
{
    if(ticks == 0)
        return;
    task->state = SLEEPING;
    task->sleep_ticks = ticks;
    list_insert_last(&task_manager.sleep_list, &task->run_node);
    
}
void set_wake(task_t* task)
{
    list_remove(&task_manager.sleep_list, &task->run_node);
}
void sys_sleep(uint32_t ms)
{
    uint32_t state =  enter_protection();
    set_block(task_manager.curr_task);
    set_sleep(task_manager.curr_task, (ms + TASK_TIME_TICKS - 1)/ TASK_TIME_TICKS);
    
    task_dispatch();
    leave_protection(state);
    
}

uint32_t sys_getpid(void)
{
    return task_manager.curr_task->pid;
}

int sys_print_msg (char * fmt, int arg) {
	log_printf(fmt, arg);
}

void copy_open_files(task_t* child)
{
    task_t* task = task_manager.curr_task;
    mutex_lock(&task_table_mutex);
    for(int i = 0; i < TASK_OFILE_NR; ++i)
    {   file_t * file = task->file_table[i];
        if(file)
        {
            child->file_table[i] = file;
            ref_inc(file);
        }
    }
    mutex_unlock(&task_table_mutex);
}

int sys_fork (void) {
    task_t * parent_task = task_manager.curr_task;

    // 分配任务结构
    task_t * child_task = alloc_task();
    if (child_task == (task_t *)0) {
        goto fork_failed;
    }

    syscall_frame_t * frame = (syscall_frame_t *)(parent_task->tss.esp0 - sizeof(syscall_frame_t));

    // 对子进程进行初始化，并对必要的字段进行调整
    // 其中esp要减去系统调用的总参数字节大小，因为其是通过正常的ret返回, 而没有走系统调用处理的ret(参数个数返回)
    int err = task_init(child_task,  parent_task->name, frame->eip,
                        frame->esp + sizeof(uint32_t)*SYSCALL_PARAM, 0);
    if (err < 0) {
        goto fork_failed;
    }
    copy_open_files(child_task);
    // 从父进程的栈中取部分状态，然后写入tss。
    // 注意检查esp, eip等是否在用户空间范围内，不然会造成page_fault
    tss_t * tss = &child_task->tss;
    tss->eax = 0;                       // 子进程返回0
    tss->ebx = frame->ebx;
    tss->ecx = frame->ecx;
    tss->edx = frame->edx;
    tss->esi = frame->esi;
    tss->edi = frame->edi;
    tss->ebp = frame->ebp;

    tss->cs = frame->cs;
    tss->ds = frame->ds;
    tss->es = frame->es;
    tss->fs = frame->fs;
    tss->gs = frame->gs;
    tss->eflags = frame->eflags;

    child_task->parent = parent_task;
    
    // 复制父进程的内存空间到子进程
    if ((child_task->tss.cr3 = memory_copy_uvm(parent_task->tss.cr3)) < 0) {
        goto fork_failed;
    }
    task_start(child_task);
    // 创建成功，返回子进程的pid
    return child_task->pid;
fork_failed:
    if (child_task) {
        task_uninit (child_task);
        free_task(child_task);
    }
    return -1;
}

int sys_execve(const char*name , char* const* argv, char* const* env)
{
    task_t* task = task_manager.curr_task;
    kernel_strncpy(task->name, get_filename(name), TASK_NAME_SIZE);
    uint32_t old_page_dir = task->tss.cr3;
    uint32_t new_page_dir = memory_create_uvm();
    
    if(!new_page_dir)
        goto execve_failed;
    uint32_t entry = load_elf_file(task, name, new_page_dir);
    if(entry == 0)
        goto execve_failed;
    uint32_t stack_top = MEM_STACK_TOP - MEM_ARG_SIZE;
    int err = memory_alloc_for_page_dir(new_page_dir, MEM_STACK_TOP - MEM_STACK_SIZE, MEM_STACK_SIZE, PTE_P|PTE_U|PTE_W);
    if(err == -1)
    {
        log_printf("alloc err");
        goto execve_failed;
    }
    int argc = count_string(argv);
    err = cpy_to_stack((char*)stack_top, new_page_dir, argc, argv);
    if(err < 0)
    {
        log_printf("cpy args err");
        goto execve_failed;
    }
    syscall_frame_t* frame = (syscall_frame_t*)(task->tss.esp0 - sizeof(syscall_frame_t));
    frame->eip = entry;
    frame->eax = frame->ebx = frame->ecx = frame->edx = 0;
    frame->esi = frame->edi = frame->ebp = 0;
    frame->eflags = EFLAGS_DEFAULT| EFLAGS_IF;
    frame->esp = stack_top - sizeof(uint32_t)*SYSCALL_PARAM;
    task->tss.cr3 = new_page_dir;
    mmu_set_page_dir(new_page_dir);
    memory_destroy_uvm(old_page_dir);
    return 0;
execve_failed:
    if (new_page_dir) {
        task->tss.cr3 = old_page_dir;
        mmu_set_page_dir(old_page_dir);
        memory_destroy_uvm(new_page_dir);
    }
    return -1;
}
int cpy_to_stack(char* to, uint32_t page_dir, int argc, char* const* argv)
{
    task_arg_t task_arg;
    task_arg.argc = argc;
    task_arg.argv = (char **)(to + sizeof(task_arg_t));
    char * dest_arg = to + sizeof(task_arg_t) + sizeof(char *) * (argc + 1);
    char ** dest_argv_tb = (char **)memory_get_paddr(page_dir, (uint32_t)(to + sizeof(task_arg_t)));
    Assert(dest_argv_tb != 0);
    for(int i = 0; i < argc; ++i)
    {
        char* from = argv[i];
        int len = kernel_strlen(from) + 1;
        int err = memory_copy_uvm_data((uint32_t)dest_arg, page_dir, (uint32_t)from, len);
        Assert(err >= 0);
        dest_argv_tb[i] = dest_arg;
        dest_arg += len;
    }
    if(argc)
        dest_argv_tb[argc] = '\0';
    return memory_copy_uvm_data((uint32_t)to, page_dir, (uint32_t)&task_arg, sizeof(task_arg_t));
}
static task_t * alloc_task (void) {
    task_t * task = (task_t *)0;

    mutex_lock(&task_table_mutex);
    for (int i = 0; i < TASK_NUM; i++) {
        task_t * curr = task_table + i;
        if (curr->name[0] == 0) {
            task = curr;
            break;
        }
    }
    mutex_unlock(&task_table_mutex);

    return task;
}

static void free_task (task_t * task) {
    mutex_lock(&task_table_mutex);
    task->name[0] = 0;
    mutex_unlock(&task_table_mutex);
}

static uint32_t load_elf_file (task_t * task, const char * name, uint32_t page_dir) {
    Elf32_Ehdr elf_hdr;
    Elf32_Phdr elf_phdr;

    // 以只读方式打开
    int file = sys_open(name, 0);   // todo: flags暂时用0替代
    if (file < 0) {
        log_printf("open file failed.%s", name);
        goto load_failed;
    }

    // 先读取文件头
    int cnt = sys_read(file, (char *)&elf_hdr, sizeof(Elf32_Ehdr));
    if (cnt < sizeof(Elf32_Ehdr)) {
        log_printf("elf hdr too small. size=%d", cnt);
        goto load_failed;
    }

    // 做点必要性的检查。当然可以再做其它检查
    if ((elf_hdr.e_ident[0] != ELF_MAGIC) || (elf_hdr.e_ident[1] != 'E')
        || (elf_hdr.e_ident[2] != 'L') || (elf_hdr.e_ident[3] != 'F')) {
        log_printf("check elf indent failed.");
        goto load_failed;
    }


    // 必须有程序头部
    if ((elf_hdr.e_phentsize == 0) || (elf_hdr.e_phoff == 0)) {
        log_printf("none programe header");
        goto load_failed;
    }

    // 然后从中加载程序头，将内容拷贝到相应的位置
    uint32_t e_phoff = elf_hdr.e_phoff;
    for (int i = 0; i < elf_hdr.e_phnum; i++, e_phoff += elf_hdr.e_phentsize) {
        if (sys_lseek(file, e_phoff, 0) < 0) {
            log_printf("read file failed");
            goto load_failed;
        }

        // 读取程序头后解析，这里不用读取到新进程的页表中，因为只是临时使用下
        cnt = sys_read(file, (char *)&elf_phdr, sizeof(Elf32_Phdr));
        if (cnt < sizeof(Elf32_Phdr)) {
            log_printf("read file failed");
            goto load_failed;
        }

        // 简单做一些检查，如有必要，可自行加更多
        // 主要判断是否是可加载的类型，并且要求加载的地址必须是用户空间
        if ((elf_phdr.p_type != PT_LOAD) || (elf_phdr.p_vaddr < MEMORY_TASK_BASE)) {
           continue;
        }

        // 加载当前程序头
        int err = load_phdr(file, &elf_phdr, page_dir);
        if (err < 0) {
            log_printf("load program hdr failed");
            goto load_failed;
        }

        // 简单起见，不检查了，以最后的地址为bss的地址
        task->heap_start = elf_phdr.p_vaddr + elf_phdr.p_memsz;
        task->heap_end = task->heap_start;
   }

    sys_close(file);
    return elf_hdr.e_entry;

load_failed:
    if (file >= 0) {
        sys_close(file);
    }

    return 0;
}

int load_phdr(int fd, Elf32_Phdr* elf_phdr, uint32_t page_dir)
{

    int err = memory_alloc_for_page_dir(page_dir, elf_phdr->p_vaddr, elf_phdr->p_memsz, PTE_P|PTE_U|PTE_W);
    if(err == -1)
    {
        log_printf("memory alloc err");
        goto loadphdr_fault;
    }
    sys_lseek(fd, elf_phdr->p_offset, 0);
    uint32_t vaddr = elf_phdr->p_vaddr;
    uint32_t size = elf_phdr->p_filesz;
    while(size > 0)
    {
        int curr_size = size > MEM_PAGE_SIZE ? MEM_PAGE_SIZE : size;
        uint32_t paddr = memory_get_paddr(page_dir, vaddr);
        sys_read(fd, (char*)paddr, curr_size);
        size -= curr_size;
        vaddr += curr_size;
    }
    return 0;
loadphdr_fault:
    return -1;
}

int sys_wait(int* status) {
    task_t* task = task_manager.curr_task;
    for(;;)
    {
        mutex_lock(&task_table_mutex);
        for(int i = 0; i < TASK_NUM; ++i)
        {
            if(task_table[i].parent == task)
            {
                if(task_table[i].state == ZOMBIE)
                {
                    int pid = task_table[i].pid;
                    *status = task_table[i].status;
                    memory_destroy_uvm(task_table[i].tss.cr3);
                    memory_free_page(task_table[i].tss.esp0 - MEM_PAGE_SIZE);
                    kernel_memset((void*)&task_table[i], 0, sizeof(task_t));
                    mutex_unlock(&task_table_mutex);
                    return pid;
                }
            }
        }
        mutex_unlock(&task_table_mutex);
        // irq_state_t state = enter_protection();
        task->state = WAITING;
        set_block(task);
        task_dispatch();
        // leave_protection(state);
        
    }
	return 0;
}
void sys_exit(int status) {
	task_t* task = task_manager.curr_task;
    for(int i = 0; i < TASK_OFILE_NR; ++i)
    {
        file_t* file = task_file(i);
        if(file)
        {
            sys_close(i);
            task->file_table[i] = (file_t*)0;
        }
    }

    mutex_lock(&task_table_mutex);
    int move_child = 0;
    for(int i = 0; i < TASK_NUM;++i)
    {
        if(task_table[i].parent == task)
        {
            task_table[i].parent = &task_manager.first_task;
            if(task_table[i].state == ZOMBIE)
            {
                move_child = 1;
            }
        }
    }
    mutex_unlock(&task_table_mutex);
    irq_state_t state = enter_protection();
    task_t* parent = task->parent;
    if(move_child && (parent != &task_manager.first_task))
    {
        if(task_manager.first_task.state == WAITING)
            set_ready(&task_manager.first_task);
    }
    if(parent->state == WAITING)
        set_ready(parent);
    task->status = status;
    task->state = ZOMBIE;
    set_block(task);
    task_dispatch();
    leave_protection(state);
    
    return;

}