#include <stdio.h>
#include <string.h>
#include "applib/lib_syscall.h"
#include "main.h"
#include <getopt.h>
#include "dev/tty.h"
#include "fs/file.h"
static cli_t client;
static const char * promot = "sh >>";       
#define MAX_ARGS_COUNT          10
#define ESC_CMD2(Pn, cmd)		    "\x1b["#Pn#cmd
#define	ESC_COLOR_ERROR			    ESC_CMD2(31, m)	
#define	ESC_COLOR_DEFAULT		    ESC_CMD2(39, m)	
#define ESC_CLEAR_SCREEN		    ESC_CMD2(2, J)	
#define	ESC_MOVE_CURSOR(row, col)  "\x1b["#row";"#col"H"
static void show_promot(void) {
    printf("%s", client.promot);
    fflush(stdout);
}
static int do_clear (int argc, char ** argv) {
    printf("%s", ESC_CLEAR_SCREEN);
    printf("%s", ESC_MOVE_CURSOR(0, 0));
    return 0;
}

/**
 * help鍛戒护
 */
static int do_help(int argc, char **argv) {
    const cli_cmd_t* start = client.cmd_start;
    while(start != client.cmd_end)
    {
        printf("%s   %s\n", start->name, start->useage);
        start++;
    }
    return 0;
}

static int do_echo(int argc, char **argv) 
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

static int do_less(int argc, char **argv) 
{
    int line_mode = 0;
    int ch;
    while ((ch = getopt(argc, argv, "lh")) != -1) {
        switch (ch) {
            case 'l':
                line_mode = 1;
                break;
            case 'h':
                puts("show file content");
                puts("less [-l] file");
                puts("-l show file line by line.");
                break;
            case '?':
                if (optarg) {
                    fprintf(stderr, "Unknown option: -%s\n", optarg);
                }
                optind = 1;        
                return -1;
        }
    }
    if (optind > argc - 1) {
        fprintf(stderr, "no file\n");
        optind = 1;        
        return -1;
    }
        
    FILE* file = fopen(argv[optind], "r");
    char buf[255];
    if(!line_mode)
    {
        while(fgets(buf, 255, file) != NULL)
        {
            fputs(buf, stdout);
        }
    }
    else
    {
        setvbuf(stdin, NULL, _IONBF, 0);
        ioctl(0, TTY_CMD_ECHO, 0, 0);
        while(1)
        {
            char* b = fgets(buf, 255, file);
            if(b == NULL)
                break;
            fputs(buf, stdout);
            int ch;
            while ((ch = fgetc(stdin)) != 'n') {
                if (ch == 'q') {
                    goto less_quit;
                }
            }
        }
less_quit:
    setvbuf(stdin, NULL, _IOLBF, BUFSIZ);
    ioctl(0, TTY_CMD_ECHO, 1, 0);
    }

    optind = 1;
    return 0;
}
static int do_exit(int argc, char** argv)
{
     exit(0);
     return 0;
}
static int do_cp(int argc, char** argv)
{
    if(argc < 3)
    {
    fprintf(stderr, "src or dest not open\n");
    }
    FILE* from, *to;
    from = fopen(argv[1], "rb");
    to = fopen(argv[2], "wb");
    char buf[255];
    int size = 0;
    while((size = fread(buf, 1, 255, from)) > 0)
    {
        fwrite(buf, 1, size, to);
    }
    // free(buf);
    if(from)
        fclose(from);
    if(to)
        fclose(to);
    return 0;
}
static int do_ls(int argc, char** argv)
{
    DIR* dir = opendir("temp");
    if(dir == (DIR*)0)
    {
        fprintf(stderr, "open dir failed\n");
        return -1;
    }
    struct dirent* dirent ;
    while((dirent = readdir(dir)) != NULL){
        printf("%c %s %d\n", dirent->type == FILE_DIR ? 'd': 'f', dirent->name, dirent->size);

    }
    closedir(dir);
    return 0;
}
static int do_rm(int argc, char** argv)
{
    if(argc < 2)
    {
    fprintf(stderr, "no path\n");
    }
    unlink(argv[1]);
}
static const cli_cmd_t cmd_list[] = {
    {
        .name = "help",
		.useage = "help -- list support command",
		.do_func = do_help,
    },
    {
        .name = "clear",
		.useage = "clear -- clear the screen",
		.do_func = do_clear,
    },
    {
        .name = "echo",
        .useage = "echo -- echo something",
        .do_func = do_echo,
    },
    {
        .name = "ls",
        .useage = "list director",
        .do_func = do_ls,
    },
    {
        .name = "less",
        .useage = "less [-l] file",
        .do_func = do_less,
    },
    {
        .name = "cp",
        .useage = "cp src dest",
        .do_func = do_cp,
    },
    {
        .name = "rm",
        .useage = "remove file",
        .do_func = do_rm,
    },
    {
        .name = "quit",
        .useage = "echo -- echo something",
        .do_func = do_exit,
    }
};

static void run_exec_file (const char * path, int argc, char ** argv) {
    int pid = fork();
    if (pid < 0) {
        fprintf(stderr, "fork failed: %s", path);
    } else if (pid == 0) {
        int err = execve(path, argv, (char * const *)0);
        if (err < 0) {
            fprintf(stderr, "exec failed: %s", path);
        }
        exit(-1);
    } else {
        int status;
        int pid = wait(&status);
        fprintf(stderr, "cmd %s result: %d, pid = %d\n", path, status, pid);
    }
}
static void cli_init(const char * promot, const cli_cmd_t * cmd_list, int cnt) {
    client.promot = promot;
    
    memset(client.curr_input, 0, CLI_INPUT_SIZE);
    
    client.cmd_start = cmd_list;
    client.cmd_end = cmd_list + cnt;
}
static const char * find_exec_path (const char * file_name) {
    int fd = open(file_name, 0);
    static char path[255];
    if (fd < 0) {
        sprintf(path, "%s.elf", file_name);
        fd = open(path, 0);
        if(fd < 0)
        {
            return (const char*)0;
        }
        close(fd);
        return path;
    }

    close(fd);
    return file_name;
}
cli_cmd_t* find_buildin(char* s){
    for(cli_cmd_t* start = client.cmd_start; start != client.cmd_end; ++start)
    {
        if(strcmp(s, start->name) == 0)
            return start;
    }
    return (cli_cmd_t*) 0;   
}

int  run_buildin(cli_cmd_t* cmd, int argc, char**argv)
{
    int ret = cmd->do_func(argc, argv);
    if(ret < 0)
        fprintf(stderr, "err : %d\n", ret);
}
int main (int argc, char **argv) {
	open(argv[0], 0);
    dup(0);     
    dup(0);     

   	cli_init(promot, cmd_list, sizeof(cmd_list) / sizeof(cli_cmd_t));

    for (;;) {
        show_promot();
        char* str = fgets(client.curr_input, CLI_INPUT_SIZE, stdin);
        if (str == (char *)0) {
            break;
        }
        char* c = strchr(client.curr_input, '\n');
        if(c)
            *c = '\0';
        c = strchr(client.curr_input, '\r');
        if(c)
            *c = '\0';
        const char* space = " ";
        int argc = 0;
        char* argv[MAX_ARGS_COUNT];
        memset(argv, 0, sizeof(argv));
        char* token = strtok(client.curr_input, space);
        while(token)
        {
            argv[argc++] = token;
            token = strtok(NULL, space);
        }
        cli_cmd_t* cmd = find_buildin(argv[0]);
        if(cmd)
        {
            run_buildin(cmd, argc, argv);
            continue;
        }
        else{
            const char * path = find_exec_path(argv[0]);
            if (path) {
                run_exec_file(path, argc, argv);
                continue;
            }
        }
        
        fprintf(stderr, ESC_COLOR_ERROR"unknown err %s\n"ESC_COLOR_DEFAULT, argv[0]);
    }

    return 0;
}