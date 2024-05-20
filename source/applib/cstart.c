#include "comm/types.h"
#include "lib_syscall.h"
#include <stdlib.h>
int main(int argc, char** argv);

extern uint8_t s_bss[], e_bss[];
void cstart(int argc, char** argv)
{
    
    uint8_t * start = s_bss;
    while (start < e_bss) {
        *start++ = 0;
    }

    exit(main(argc, argv));
}