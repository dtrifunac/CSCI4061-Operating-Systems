#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define MAX_WORD_LEN 25

/*
 * Counts the number of occurrences of words of different lengths in a text
 * file and stores the results in an array.
 * file_name: The name of the text file from which to read words
 * counts: An array of integers storing the number of words of each possible
 *     length.  counts[0] is the number of 1-character words, counts [1] is the
 *     number of 2-character words, and so on.
 * Returns 0 on success or -1 on error.
 */
int count_word_lengths(const char *file_name, int *counts) {
    FILE* fp = fopen(file_name, "r");
    if(fp == NULL){     //check that file was correctly opened
        perror("fopen");
        return -1;
    }
    char c;
    int s = fread(&c, sizeof(char),1, fp);      //start reading char by char
    if(s == -1){
        perror("fread");
        if(fclose(fp)){
            perror("fclose");
        }
        return -1;
    }
    int size = 0;   
    while(s > 0){       //iterate char by char through the file
        if(isspace(c) && size>0){       //if it is a space then we have reached the end of a string
            counts[size-1]++;
            size = 0;
        }
        else if(c != '\n'){     //otherwise continue augmenting the lenght of the string
            size++;
        }
        s = fread(&c, sizeof(char),1, fp);      //ge teh next char
        if(s == -1){
            perror("fread");
            if(fclose(fp)){
                perror("fclose");
            }
            return -1;
        }
    }


    if(fclose(fp)){     //close file before exiting
        perror("fclose");
        return -1;
    }
            
    return 0;
}

/*
 * Processes a particular file (counting the number of words of each length)
 * and writes the results to a file descriptor.
 * This function should be called in child processes.
 * file_name: The name of the file to analyze.
 * out_fd: The file descriptor to which results are written
 * Returns 0 on success or -1 on error
 */
int process_file(const char *file_name, int out_fd) {
    int *counts = malloc(sizeof(int)*MAX_WORD_LEN);
    memset(counts,0,sizeof(int)*MAX_WORD_LEN);
    count_word_lengths(file_name, counts);
    write(out_fd, counts, sizeof(int)*MAX_WORD_LEN);
    return 0;
}

int main(int argc, char **argv) {
    if (argc == 1) {
        // No files to consume, return immediately
        return 0;
    }

    int pip[2];
    if(pipe(pip)){      //initialize the pipe used for children to right their array to
        perror("pipe");
        return -1;
    }
    int status;

    for(int i = 1; i < argc; i++){      // iterate through the number of files to make children to count each one
        pid_t child = fork();
        if(child==0){
            if(close(pip[0])){      //make sure the close read end because it won't be used
                perror("close");
                if(close(pip[1])){
                    perror("close");
                }
                exit(1);
            }
            if(process_file(argv[i],pip[1])){       //process file and write counts array to write end of the pipe
                perror("process_file");
                if(close(pip[1])){
                    perror("close");
                    exit(1);
                }
            }
            if(close(pip[1])){      //close write end of hte pipe before exiting
                perror("close");
                exit(1);
            }
            exit(0);
        }
        else if(child==-1){     //error check fork
            perror("fork");
            for(int j = 1; j < i; j++){
                wait(&status);
                if(WEXITSTATUS(status)){
                    perror("wait");
                }
            }
            if(close(pip[0])){
                perror("close");
                if(close(pip[1])){
                    perror("close");
                }
                return -1;
            }
            if(close(pip[1])){
                perror("close");
                if(close(pip[0])){
                    perror("close");
                }
                return -1;
            }
            return -1;
        }
    }

    int arr[MAX_WORD_LEN];
    int temp[MAX_WORD_LEN];
    if(memset(arr, 0, sizeof(int)*MAX_WORD_LEN)!=arr){      //initialize an array for the total sums of counts
        perror("memset");
        if(close(pip[0])){
            perror("close");
            if(close(pip[1])){
                perror("close");
            }
            return -1;
        }
        if(close(pip[1])){
            perror("close");
            if(close(pip[0])){
                perror("close");
            }
            return -1;
        }
        return -1;
    }
    for(int i = 1; i < argc; i++){      //iterate through the arrays written by the children to add their counts to the master count array
        if(read(pip[0], &temp, MAX_WORD_LEN*sizeof(int))==0){
            perror("read");
            if(close(pip[0])){
                perror("close");
                if(close(pip[1])){
                    perror("close");
                }
                return -1;
            }
            if(close(pip[1])){
                perror("close");
                if(close(pip[0])){
                    perror("close");
                }
                return -1;
            }
            return -1;
        }
        for(int j = 0; j < MAX_WORD_LEN; j++){      //for each index in the master count array add however many of each length were found that the file completed by the child process
            arr[j] += temp[j];
        }
    }


    for (int i = 1; i <= MAX_WORD_LEN; i++) {       // iterate through the master array to print out the result to the screen
        if(printf("%d-Character Words: %d\n", i, arr[i-1])==-1){
            perror("printf");
            if(close(pip[0])){
                perror("close");
                if(close(pip[1])){
                    perror("close");
                }
                return -1;
            }
            if(close(pip[1])){
                perror("close");
                if(close(pip[0])){
                    perror("close");
                }
            return -1;
            }
        return -1;
        }
    }
    if(close(pip[0])){      //finally close the read end of the pipe in the parent
        perror("close");
        if(close(pip[1])){
            perror("close");
        }
        return -1;
    }
    if(close(pip[1])){      //close the write end of hte pipe in the parent
        perror("close");
        if(close(pip[0])){
            perror("close");
        }
        return -1;
    }
    return 0;
}
