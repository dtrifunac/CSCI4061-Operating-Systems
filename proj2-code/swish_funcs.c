#include <assert.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "job_list.h"
#include "string_vector.h"
#include "swish_funcs.h"

#define MAX_ARGS 10

int tokenize(char *s, strvec_t *tokens) {
    if(s == NULL || tokens == NULL){        //double checks for valid arguements
        fprintf(stderr, "Invalid arguements passed into tokenize()\n");
        return -1;
    }
    char *tok = strtok(s, " ");     //get first token
    while(tok!=NULL){       //parse through s until all strings are added to tokens
        if(strvec_add(tokens, tok)){        //checks for any errors from strved_add func
            fprintf(stderr, "Error in strvec_add\n");
            return -1;
        }

        tok = strtok(NULL, " ");        //if no errors, get next token
    }
    return 0;
}

int run_command(strvec_t *tokens) {
    pid_t id = getpid();        // get pid of current process
    setpgid(id, STDIN_FILENO);  // assign current process to the foreground

    struct sigaction sac;       // define a signal handler so process can receive signals
    sac.sa_handler = SIG_DFL;   // 
    if (sigfillset(&sac.sa_mask) == -1) {
        perror("sigfillset");
        return 1;
    }
    sac.sa_flags = 0;
    if (sigaction(SIGTTIN, &sac, NULL) == -1 || sigaction(SIGTTOU, &sac, NULL) == -1) {     //set up to revieve SIGTTINT and SIGTTOU
        perror("sigaction");
        return 1;
    }


    char* func = strvec_get(tokens, 0);     // function to be called
    if(func == NULL){
        fprintf(stderr, "Error in strvec_get\n");
        return -1;
    }
    char* tmp = func;      // file to run
    int n = 0;      // used later on as an index to parse through the entries in tokens
    char* out;      // file to write to
    int o = -1;     // flag to check if there is an output file to be closed
    int in = 0;         // will be used for input file
    int a = strvec_find(tokens, ">");
    if(a!=-1){      //see if > is in the command
        out = strvec_get(tokens, a+1);      // get file to write to
        if(out == NULL){        //check for error in strvec_get
            fprintf(stderr, "Errer getting output file\n");
            return -1;
        }
        o = open(out, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR);       // open file to write to
        if(o < 0){      // error check file was opened correctly
            perror("Failed to open output file");
            return -1;
        }
        if(dup2(o,STDOUT_FILENO) < 0){      // direct output to output file and check for errors
            perror("dup2");
            if(close(o)){
                perror("close");
            }
            return -1;
        }
        strvec_take(tokens, a);     //concatonate off anything relating to > in command line as it will not be needed anymore
    }
    int b = strvec_find(tokens, ">>");
    if(b!=-1){      //see if >> is in the command
        out = strvec_get(tokens, b+1);          // repeats the same thing for > as both commands are similar
        if(out == NULL){
            fprintf(stderr, "Errer getting output file\n");
            return -1;
        }
        o = open(out, O_APPEND|O_CREAT, S_IRUSR|S_IWUSR);       // open file making sure not to overwrite
        if(o < 0){
            perror("Failed to open output file");
            return -1;
        }
        if(dup2(o,STDOUT_FILENO) < 0){
            perror("dup2");
            if(close(o)){
                perror("close");
            }
            return -1;
        }
        strvec_take(tokens, b);
    }
    int c = strvec_find(tokens, "<");
    if(c!=-1){      //see if < is in the command
        char* input = strvec_get(tokens, c+1);      // the name of the file to feed commands from
        strvec_take(tokens, c);
        if(input == NULL){
            fprintf(stderr, "Errer getting input file\n");
            return -1;
        }
        in = open(input, O_RDONLY);     // open file to read commands from
        if(in < 0){     // check file was opened correctly
            perror("Failed to open input file");
            if(o != -1){
                if(close(o)){
                    perror("close");
                }
            }
            return -1;
        }
        if(dup2(in,STDIN_FILENO) < 0){      // check that command input was properly redirected to come from input file
            perror("dup2");
            if(close(in)){
                perror("close");
                if(o != -1){
                    if(close(o)){
                        perror("close");
                    }
                }
            }
            if(o != -1){
                if(close(o)){
                    perror("close");
                }
            }
            return -1;
        }
    }

    unsigned int len = tokens->length;      // get length of tokens vector for while loop below
    char *args[MAX_ARGS];       // char array of arguments that will be passed as command line arguements
    while(n < len && n < MAX_ARGS){
        tmp = strvec_get(tokens, n);
        if(tmp == NULL){        // error check for strvec_get
            if(c!=-1){
                if(close(in)){
                    perror("close");
                    if(o != -1){
                        if(close(o)){
                            perror("close");
                        }
                    }
                }
            }
            if(o != -1){
                if(close(o)){
                    perror("close");
                }
            }
            return -1;
        }

        args[n] = tmp;      // add entry from tokens to char array
        n++;
    }
    args[n] = NULL;     //add NULL to show end of command line arguements
    if(execvp(func,args)==-1){      // finally run command line after everything is set up. error check
        perror("exec");
        if(c!=-1){
            if(close(in)){
                perror("close");
                if(o != -1){
                    if(close(o)){
                        perror("close");
                    }
                }
            }
        }
        if(o != -1){
            if(close(o)){
                perror("close");
            }
        }
        return -1;
    }
    return 0;
}

int resume_job(strvec_t *tokens, job_list_t *jobs, int is_foreground) {
    char* s = strvec_get(tokens, 1);        //char index of process to work with
    if(s == NULL){
        fprintf(stderr, "Strvec_get error\n");
        return -1;
    }
    int idx = atoi(s);      // index of job in the list we want to manipulate. can't error check since 0 is returned on failure, but 0 is also a valid index
    job_t *job;
    int status;
    if(idx < 0 || idx >= jobs->length){     // make sure given index is within bounds
        fprintf(stderr, "Job index out of bounds\n");
        return -1;
    }
    job = job_list_get(jobs, idx);      // get the job we want to change from jobs list
    if(job == NULL){        // error check that job was successfully gotten
        fprintf(stderr, "job_list_get error\n");
        return -1;
    }

    if(is_foreground == 1){     // what to do if process is in foreground
        if(tcsetpgrp(STDIN_FILENO, job->pid)){      // add process to foreground process group
            perror("tcsetpgrp");
            return -1;
        }
        if(kill(job->pid,SIGCONT)){     // give job signal to continue
            perror("kill");
            return -1;
        }
        if(waitpid(job->pid,&status,WUNTRACED)==-1){        // wait for process running job to finish
            perror("waitpid\n");
            return -1;
        }
        if(!WIFSTOPPED(status)){     // check if child process finished successfully, otherwise don't do anything
            if(job_list_remove(jobs, idx)==-1){     //if so, remove from job list
                perror("job_list_remove");
                return -1;
            }
        }

        pid_t parent = getpid();        //get current process id and set it back to forground process
        tcsetpgrp(STDIN_FILENO,parent);
    }
    else if (is_foreground == 0){       //what to do if process is in background
        if(kill(job->pid,SIGCONT)){     // give job signal to continue
            perror("kill");
            return -1;
        }
        job->status = JOB_BACKGROUND;       // amd set statis tp JOB_BACKGROUND instead of JOB_STOPPED
    }
    else{       // error check that is_foreground got passed in correctly
        fprintf(stderr, "is_foreground error\n");
        return -1;
    }

    return 0;
}

int await_background_job(strvec_t *tokens, job_list_t *jobs) {
    char* s = strvec_get(tokens, 1);        //char index of process to work with
    if(s == NULL){
        fprintf(stderr, "Strvec_get error\n");
        return -1;
    }
    int idx = atoi(s);
    job_t *job;     // job pointer that will be used for the job operated on
    int status;
    if(idx < 0 || idx >= jobs->length){     //check that index is valid
        fprintf(stderr, "Job index out of bounds\n");
        return -1;
    }
    job = job_list_get(jobs, idx);
    if(job == NULL){        // error check that job was successfully gotten
        fprintf(stderr, "job_list_get error\n");
        return -1;
    }
    if(job->status != JOB_BACKGROUND){      // check that desired job is indeed in background
        fprintf(stderr, "Job index is for stopped process not background process\n");
        return -1;
    }

    if(tcsetpgrp(STDIN_FILENO, job->pid)){      // set job to be waited for to be a foreground process
        perror("tcsetpgrp");
        return -1;
    }


    if(kill(job->pid,SIGCONT)){     // give job signal to continue
        perror("kill");
        return -1;
    }
    if(waitpid(job->pid,&status,WUNTRACED)==-1){        // wait for that process
        perror("Wait failed\n");
        return -1;
    }
    if(!WIFSTOPPED(status)){     // check if child process finished successfully
        if(job_list_remove(jobs, idx)==-1){     // and remove that job from job list if successfully finished
            perror("job_list_remove");
            return -1;
        }
    }

    pid_t parent = getpid();
    tcsetpgrp(STDIN_FILENO,parent);     // set current process back into the foreground
    return 0;
}

int await_all_background_jobs(job_list_t *jobs) {
    job_t *job;     // used to point to job that needs to be run
    int status;     // used to store exit status of job that was just run
    for(int i = 0; i < jobs->length; i++){      // iterate through all jobs in jobs list to wait for them
        job = job_list_get(jobs, i);
        if(job == NULL){        // make sure job was successfully gotten
            fprintf(stderr, "job_list_get error\n");
        }
        if(job->status == JOB_BACKGROUND){      // only do something if job is in the background
            if(tcsetpgrp(STDIN_FILENO, job->pid)){      // set process to be waited for into the foreground
                perror("tcsetpgrp");
                return -1;
            }
            if(kill(job->pid,SIGCONT)){     // let this process continue
                perror("kill");
                return -1;
            }
            if(waitpid(job->pid,&status,WUNTRACED)==-1){        // wait for the specified process
                perror("waitpid");
                return -1;
            }
            if(!WIFSTOPPED(status)){     // check if child process finished successfully
                if(job_list_remove(jobs, i)==-1){       //if so, remove it from the list
                    fprintf(stderr, "job_list_remove error\n");
                    return -1;
                }
            }
            else{       // otherwise job got stopped. note that in its status
                job->status = JOB_STOPPED;
            }

            pid_t parent = getpid();
            tcsetpgrp(STDIN_FILENO,parent);     // set current process back into foreground
        }
    }
    job_list_remove_by_status(jobs, JOB_BACKGROUND);        //remove all processes that from jobs list that have just been completed
    return 0;
}
