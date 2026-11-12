#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "job_list.h"
#include "string_vector.h"
#include "swish_funcs.h"

#define CMD_LEN 512
#define PROMPT "@> "

int main(int argc, char **argv) {
    struct sigaction sac;
    sac.sa_handler = SIG_IGN;
    if (sigfillset(&sac.sa_mask) == -1) {
        perror("sigfillset");
        return 1;
    }
    sac.sa_flags = 0;
    if (sigaction(SIGTTIN, &sac, NULL) == -1 || sigaction(SIGTTOU, &sac, NULL) == -1) {
        perror("sigaction");
        return 1;
    }

    strvec_t tokens;
    strvec_init(&tokens);
    job_list_t jobs;
    job_list_init(&jobs);
    char cmd[CMD_LEN];

    printf("%s", PROMPT);
    while (fgets(cmd, CMD_LEN, stdin) != NULL) {
        // Need to remove trailing '\n' from cmd. There are fancier ways.
        int i = 0;
        while (cmd[i] != '\n') {
            i++;
        }
        cmd[i] = '\0';

        if (tokenize(cmd, &tokens) != 0) {
            printf("Failed to parse command\n");
            strvec_clear(&tokens);
            job_list_free(&jobs);
            return 1;
        }
        if (tokens.length == 0) {
            printf("%s", PROMPT);
            continue;
        }
        const char *first_token = strvec_get(&tokens, 0);

        if (strcmp(first_token, "pwd") == 0) {
            char cwd[CMD_LEN];
            if(getcwd(cwd,sizeof(cwd))==NULL){      //get the current working directory
                perror("getcwd");
                return -1;
            }
            printf("%s\n", cwd);        //then print it to the screen
        }

        else if (strcmp(first_token, "cd") == 0) {
            const char *second_token = strvec_get(&tokens, 1);      // get the directory to move to
            if(second_token== NULL){            //if no specified directory is provided cd into home directory
                char *wd;
                wd = getenv("HOME");        // get the home directory
                if(wd == NULL){
                    printf("Error changing to home directory\n");
                    return -1;
                }
                if(chdir(wd)){              // and change into the home directory
                    perror("chdir");
                    return -1;
                }
            }
            else{           // else cd into specified folder
                char cwd[CMD_LEN];
                if(second_token[0] == '/'){     //if specified path is an absolute path
                    if(chdir(second_token)){        // change into it
                        perror("chdir");
                        return -1;
                    }
                }
                    
                else {          // otherwise assume new directory is a relative path
                    if(getcwd(cwd,sizeof(cwd))==NULL){     // get the current working directory
                        perror("getcwd");
                        return -1;
                    }
                    strcat(cwd,"/");
                    strcat(cwd,second_token);       // add the specified directory to the end of current working directory
                    if(!access(cwd,F_OK)){      // learned about access() from qnx.com. If folder exists and is accessible
                        if(chdir(cwd)){         // then change into it
                            perror("chdir");
                            return -1;
                        }
                    }
                    else{
                        perror("chdir");
                    } 
                }
            }
        }

        else if (strcmp(first_token, "exit") == 0) {
            strvec_clear(&tokens);
            break;
        }

        // Task 5: Print out current list of pending jobs
        else if (strcmp(first_token, "jobs") == 0) {
            int i = 0;
            job_t *current = jobs.head;
            while (current != NULL) {
                char *status_desc;
                if (current->status == JOB_BACKGROUND) {
                    status_desc = "background";
                } else {
                    status_desc = "stopped";
                }
                printf("%d: %s (%s)\n", i, current->name, status_desc);
                i++;
                current = current->next;
            }
        }

        // Task 5: Move stopped job into foreground
        else if (strcmp(first_token, "fg") == 0) {
            if (resume_job(&tokens, &jobs, 1) == -1) {
                printf("Failed to resume job in foreground\n");
            }
        }

        // Task 6: Move stopped job into background
        else if (strcmp(first_token, "bg") == 0) {
            if (resume_job(&tokens, &jobs, 0) == -1) {
                printf("Failed to resume job in background\n");
            }
        }

        // Task 6: Wait for a specific job identified by its index in job list
        else if (strcmp(first_token, "wait-for") == 0) {
            if (await_background_job(&tokens, &jobs) == -1) {
                printf("Failed to wait for background job\n");
            }
        }

        // Task 6: Wait for all background jobs
        else if (strcmp(first_token, "wait-all") == 0) {
            if (await_all_background_jobs(&jobs) == -1) {
                printf("Failed to wait for all background jobs\n");
            }
        }

        else {
            int flag = 0;       //used to keep track if task should be run in the background
            char * l = strvec_get(&tokens, tokens.length-1);        // get last input from command line
            if(l == NULL){
                fprintf(stderr, "strvet_get\n");
                return -1;
            }
            if(strcmp(l,"&")==0){        // check if command is to be run in the background
                strvec_take(&tokens, tokens.length-1);
                flag = 1;
            }


            int status = 1;     // used to store child exit status
            pid_t id = fork();      // create child process to run specified task
            if(id==0){          // if process is child process, run command.
                run_command(&tokens);     // if there is an error in run_command it will already be handled, no need to do anything
                exit(0);
            }
            else if(id>0){      // otherwise if process is the parent
                if(flag == 1){      // what to do if task is to be run in the background
                    if(job_list_add(&jobs, id, first_token, JOB_BACKGROUND)==-1){       // add job to the jobs list
                        perror("job_list_add");
                        return -1;
                    }
                }
                else{       // what to do if task is to be run in the foreground
                    if(tcsetpgrp(STDIN_FILENO,id)){     // set the child process to be in the foreground
                        perror("tcsetpgrp");
                        return -1;
                    }
                    if(waitpid(id,&status,WUNTRACED)==-1){      // wait for the child process
                        perror("wait");
                        return -1;
                    }
                    
                    pid_t parent = getpid();
                    tcsetpgrp(STDIN_FILENO,parent);         // set parent back into foreground. note that getpid() doesn't throw errors
                    
                    if(WIFSTOPPED(status)){     // check if child process is stopped
                        if(job_list_add(&jobs, id, first_token,JOB_STOPPED)==-1){       // if it was stopped, add it to jobs list
                            perror("job_list_add");
                            return -1;
                        }
                    }
                }
            }
            else{       // otherwise fork failed, throw error
                perror("Fork failed\n");
                return -1;
            }
        }
        
        strvec_clear(&tokens);
        printf("%s", PROMPT);
    }
    job_list_free(&jobs);       // free memory used to store jobs vector
    return 0;
}
