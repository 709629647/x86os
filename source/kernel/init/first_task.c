#include "core/task.h"
#include "tools/log.h"
#include "applib/lib_syscall.h"
#include "dev/tty.h"
void first_task_main(void)
{
    // int count = 0;
    // int pid = getpid();
    // mprint_msg("pid is %d", pid);
    // pid = fork();
    // if (pid < 0) {
    //     mprint_msg("create child proc failed.", 0);
    // } else if (pid == 0) {
    //     mprint_msg("child: %d", count);

    //     char * argv[] = {"arg0", "arg1", "arg2", "arg3"};
    //     execve("/shell.elf", argv, (char **)0);
    // } else {
    //     mprint_msg("child task id=%d", pid);
    //     mprint_msg("parent: %d", count);
    // }

    // pid = getpid();

        for (int i = 0; i < 4; i++) {
        int pid = fork();
        if (pid < 0) {
            mprint_msg("create shell proc failed", 0);
            break;
        } else if (pid == 0) {
            char tty_num[] = "/dev/tty?";
            tty_num[sizeof(tty_num) - 2] = i + '0';
            char * argv[] = {tty_num, (char *)0};
            execve("shell.elf", argv, (char **)0);
            mprint_msg("create shell proc failed", 0);
            while (1) {
                sleep(10000);
            }
        }
    }
    for (;;) {
        int status;
        wait(&status);
    }
}