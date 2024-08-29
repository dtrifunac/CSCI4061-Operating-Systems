#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define NUM_ITERS 4

int main() {
    printf("START pid: %d parent_pid: %d\n", getpid(), getppid());
    fflush(stdout); // Prevents duplicate I/O after a fork

    for (int i = 0; i < NUM_ITERS; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            // An error occurred
            perror("fork() failed");
            return 1;
        } else if (pid > 0) {
            // Parent process, pid identifies child process
            // TODO wait for this child process to terminate
            int stat = 0;
            pid_t cpid = wait(&stat);
            if(cpid < 0){
                perror("");
                return 1;
            }

        }
    }
    printf("FINISH pid: %d parent_pid: %d\n", getpid(), getppid());
    return 0;
}
