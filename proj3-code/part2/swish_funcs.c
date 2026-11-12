#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "string_vector.h"
#include "swish_funcs.h"

#define MAX_ARGS 10

/*
 * Helper function to run a single command within a pipeline. You should make
 * make use of the provided 'run_command' function here.
 * tokens: String vector containing the tokens representing the command to be
 * executed, possible redirection, and the command's arguments.
 * pipes: An array of pipe file descriptors.
 * n_pipes: Length of the 'pipes' array
 * in_idx: Index of the file descriptor in the array from which the program
 *         should read its input, or -1 if input should not be read from a pipe.
 * out_idx: Index of the file descriptor in the array to which the program
 *          should write its output, or -1 if output should not be written to
 *          a pipe.
 * Returns 0 on success or -1 on error.
 */
int run_piped_command(strvec_t *tokens, int *pipes, int n_pipes, int in_idx, int out_idx) { //handles running one command in the pipepline
    // TODO Complete this function's implementation
    int return_val = 0;
    pid_t child = fork();       // create child to run program in
    if(child == 0){
        // for(int i =0; i < n_pipes; i++){        // first close all unused pipes
        //     if(i!=in_idx && i != out_idx){
        //         close(pipes[i]);
        //     }
        // }
        if(in_idx>=0){      // set to read in from specified pipe
            if(dup2(pipes[in_idx],STDIN_FILENO)==-1){
                perror("dup2");
                exit(1);
            }      
        }
        if(out_idx>=0){     // set to write out to specified pipe
            if(dup2(pipes[out_idx],STDOUT_FILENO)==-1){
                perror("dup2");
                exit(1);
            }
        }
        if(run_command(tokens)){        // run specified command
            perror("run_command");
            exit(1);
        }
        
        exit(0);
    }
    else if(child < 0){     // check if fork failed
        perror("pipe");
        return_val = -1;
    }
    if(in_idx>=0){      // close pipes that were in use
            if(close(pipes[in_idx])){
                perror("close");
                return_val = -1;
            }      
        }
        if(out_idx>=0 ){        // close pipes that were in use
            if(close(pipes[out_idx])){
                perror("close");
                return_val = -1;
            }
        }
    return return_val;
}

int run_pipelined_commands(strvec_t *tokens) {
    // TODO Complete this function's implementation
    int n = strvec_num_occurrences(tokens, "|");        // how many pipes need to be made
    int first = strvec_find(tokens, "|");       // first appearance of pipe command
    if(first==-1){      // double check there are actually pipes to be made
        return 0;
    }
    int last = strvec_find_last(tokens, "|");       // last appearance of pipe command
    strvec_t dest;      // used to slice and pass into run_piped_commnand
    strvec_t temp;      // used for inner commands to subset tokens 
    int *fds = malloc(2*n*sizeof(int));     // allocate memory for the pipeline
    if(fds==NULL){
        perror("malloc");
        return -1;
    }
    for(int i = 0; i < n; i++){     //initialize all the pipes in the pipeline
        pipe(fds + 2*i);
    }
    for(int i = 0; i < n+1; i++){       //iterate through each command to be run
        if(i==0){       // special case for first command in pipeline
            if(strvec_slice(tokens, &dest, 0, first)){          //subset tokens to dest to only have first command
                perror("strvec_slice");
                free(fds);
                for(int i = 0; i < n; i++){
                    close(fds[2*i+1]);
                    close(fds[2*i]);
                }
                for(int j = 0; j < i; j++){
                    wait(NULL);
                }
                return -1;
            }     
            if(run_piped_command(&dest, fds, n, -1, 1)){        //run first command
                perror("run_piped_command");
                free(fds);
                for(int i = 0; i < n; i++){
                    close(fds[2*i+1]);
                    close(fds[2*i]);
                }
                for(int j = 0; j < i; j++){
                    wait(NULL);
                }
                return -1;
            }        
            strvec_clear(&dest);        //clear any memory in dest
        }
        else if(i==n){      // special case for last command in pipeline
            if(strvec_slice(tokens, &dest, last+1, tokens->length)){          //subset tokens to dest to only have last command
                perror("strvec_slice");
                free(fds);
                for(int i = 0; i < n; i++){
                    close(fds[2*i+1]);
                    close(fds[2*i]);
                }
                for(int j = 0; j < i; j++){
                    wait(NULL);
                }
                return -1;
            }     
            if(run_piped_command(&dest, fds, n, 2*(i-1), -1)){        //run command
                perror("run_piped_command");
                free(fds);
                for(int i = 0; i < n; i++){
                    close(fds[2*i+1]);
                    close(fds[2*i]);
                }
                for(int j = 0; j < i; j++){
                    wait(NULL);
                }
                return -1;
            }      
            strvec_clear(&dest);        // clear any memory used by dest

        }
        else{
            int start = first;
            int end = -1;
            
            for(int j = 0; j < i; j++){
                if(strvec_slice(tokens, &temp, start+1, tokens->length)){       // slice through tokens to find middle commands
                    perror("strvec_slice");
                    free(fds);
                for(int i = 0; i < n; i++){
                    close(fds[2*i+1]);
                    close(fds[2*i]);
                }
                for(int j = 0; j < i; j++){
                    wait(NULL);
                }
                return -1;
                }
                if(j==i-1){
                    end = start+1;
                    end +=strvec_find(&temp, "|");      // set end index if at the end of the loop
                }
                else{
                    start += strvec_find(&temp, "|");       // augment the start index otherwise
                }
                
                strvec_clear(&temp);
            }
            if(strvec_slice(tokens, &dest, start+1, end)){      // set dest now that we have the start and end index of the command
                perror("run_piped_command");
                free(fds);
                for(int i = 0; i < n; i++){
                    close(fds[2*i+1]);
                    close(fds[2*i]);
                }
                for(int j = 0; j < i; j++){
                    wait(NULL);
                }
                return -1;
            }
            if(run_piped_command(&dest, fds, n, 2*(i-1), 2*i+1)){        //run command
                perror("run_piped_command");
                free(fds);
                for(int i = 0; i < n; i++){
                    close(fds[2*i+1]);
                    close(fds[2*i]);
                }
                for(int j = 0; j < i; j++){
                    wait(NULL);
                }
                return -1;
            }   
            strvec_clear(&dest);
        }
    }
    int return_val = 0;

    // don't need to close pipes since they will have been closed by call to run_piplined_command
    for(int i = 0; i < n+1; i++){       // make sure to wait for all children before exiting
        if(wait(NULL)==-1){
            perror("wait");
            return_val= -1;
        }
    }

    free(fds);      // free memory used to make the pipeline
    return return_val;
}
