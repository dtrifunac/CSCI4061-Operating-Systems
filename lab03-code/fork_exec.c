#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main() {
    int child_status = -1;

    // TODO Fork a child process to run the command "cat sample.txt"
    // The parent should wait and print out the child's exit code
    pid_t pid = fork();
    if (pid < 0) {
            // An error occurred
            perror("fork() failed");
            return 1;
        }
    else if(pid == 0){
        execlp("cat","cat","sample.txt" ,NULL);
        perror("failed to execute");
    }
    else{
        wait(&child_status);
        printf("Child exited with status %d\n", child_status);
        
    }
    
    return 0;
}
