#define _GNE_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sched.h>
#include <sys/wait.h>
#include <sys/mount.h>
#include <unistd.h>
#include <cstring>
#include <errno.h>

#define STACK_SIZE (1024 * 1024)
static char child_stack[STACK_SIZE];

typedef struct clone_args{
    char **argv;
} cloneArgs;

const int DEFAULT       = SIGCHLD;
const int PID_Flags     = SIGCHLD | CLONE_NEWPID;
const int NET_Flags     = SIGCHLD | CLONE_NEWNET;
const int USER_Flags    = SIGCHLD | CLONE_NEWUSER;
const int UTS_Flags     = SIGCHLD | CLONE_NEWUTS;
const int IPC_Flags     = SIGCHLD | CLONE_NEWIPC;

// Change these parameters to modify the container type
static int clone_flags = USER_Flags;

static int child_exec(void *child_args) {
    cloneArgs *args = ((cloneArgs *) child_args);

    // Mount proc for new PID
    if (clone_flags == PID_Flags) {
//        if(umount("/proc") != 0){
//            fprintf(stderr, "Failed to unmount /proc.\n");
//            exit(-1);
//        }

        if (mount("proc", "/proc", "proc", 0, "") != 0) {
            fprintf(stderr, "Failed to mount /proc: %s\n", strerror(errno));
            exit(-1);
        }
    }

    // Set uid so child believes it is root
    if (clone_flags == USER_Flags) {

        if (seteuid(0) != 0) {
            fprintf(stderr, "Failed to set euid to 0: %s\n", strerror(errno));
            exit(-1);
        }
    }

    // Set host name for UTS namespace
    if (clone_flags == UTS_Flags) {
        char *name = "myhostnamespace";
        if (sethostname(name, strlen(name)) != 0) {
            fprintf(stderr, "Failed to set host name: %s\n", strerror(errno));
            exit(-1);
        }
    }

    // Exec - should never return.
    if (execvp(args->argv[0], args->argv) != 0) {
        fprintf(stderr, "Failed to execute child: %s\n", strerror(errno));
        exit(-1);
    }

    // Should never reach here
    exit(EXIT_FAILURE);
}

int main(int argc, char* argv[]) {
    struct clone_args args;
    args.argv = &argv[1];

    // clone(2), spawn our child process.
    pid_t pid;
    pid = clone(child_exec, child_stack + STACK_SIZE, clone_flags, &args);

    // Check if child was created
    if(pid < 0){
        fprintf(stderr, "Clone failed. \n");
        exit(EXIT_FAILURE);
    }

    // Wait on child
    if(waitpid(pid, NULL, 0) == -1){
        fprintf(stderr, "Failed to wait on pid %d\n", pid);
        exit(EXIT_FAILURE);
    }
    exit(EXIT_SUCCESS);
}