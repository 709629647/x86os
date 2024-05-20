SECTIONS{
    . = 0x10000;
    PROVIDE(s_text = .);
    .text : {
        *(EXCLUDE_FILE(*first_task* *lib_syscall*) .text)
    }
    .rodata : {
        *(EXCLUDE_FILE(*first_task* *lib_syscall*) .rodata)
    }
    PROVIDE(e_text = .);

    . = ALIGN(4096);
    
    PROVIDE(s_data = .);
    .data : {
        *(EXCLUDE_FILE(*first_task* *lib_syscall*) .data)
    }
    .bss : {
        *(EXCLUDE_FILE(*first_task* *lib_syscall*) .bss)
    }
    e_data = .;
    

    . = 0x80000000;
    PROVIDE(s_first_task = LOADADDR(.first_task));
    .first_task : AT(e_data){
        *first_task_entry*(.text .rodata .data .bss)
        *first_task*(.text .rodata .data .bss)
        *lib_syscall*(.text .rodata .data .bss)
    }
    PROVIDE(e_first_task = LOADADDR(.first_task) + SIZEOF(.first_task));
    PROVIDE(mem_free_start = e_first_task); 
    
    
}