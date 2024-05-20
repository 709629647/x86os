#include <stdio.h>
#include <string.h>
#include "applib/lib_syscall.h"
#include "main.h"
#include <getopt.h>
#include "dev/tty.h"
#include "fs/file.h"

int main(int argc, char **argv) 
{
    if(argc == 1)
    {
        char buf[128];
        fgets(buf, 128 - 1, stdin);
        buf[sizeof(buf) - 1] = '\0';
        puts(buf);
        return 0;
    }
    int count = 1;
    int ch;
    optind = 1;
    while((ch = getopt(argc, argv, "n:h")) != -1)
    {
        switch (ch)
        {
        case 'h':
            puts("echo -- echo something");
            puts("echo [-n count] ");
            return 0;
        case 'n':
            count = atoi(optarg);
            break;
        case '?':
        if(optarg)
            fprintf(stderr, "unknown option %s", optarg);
        default:
            break;
        }
    }
    if(optind > argc - 1)
        fprintf(stderr, "err ");
     char * msg = argv[optind];
    for (int i = 0; i < count; i++) {
        puts(msg);
    }
    optind = 1;  
    return 0;
}
