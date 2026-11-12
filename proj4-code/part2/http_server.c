#include <errno.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "connection_queue.h"
#include "http.h"

#define BUFSIZE 512
#define LISTEN_QUEUE_LEN 5
#define N_THREADS 5

int keep_going = 1;
const char *serve_dir;

void handle_sigint(int signo) {     //signal handler
    keep_going = 0;
}

// Consumer thread start function
void *consumer_thread_func(void *arg) {     //worker thread function
    connection_queue_t *queue = (connection_queue_t *) arg;     //convert thread input into queue struct
    char buf[BUFSIZE];
    int client_fd;
    while(keep_going!=0 && queue->shutdown == 0){           //continue handling requests while there is no signal to end 
        client_fd = connection_dequeue(queue);              //get request from the queue
        if(client_fd>0){
            if(read_http_request(client_fd, buf)==0){          //read the request and only continue if successful
                char file_path[BUFSIZE];
                if(getcwd(file_path,sizeof(file_path))==NULL){        //set up file path to return
                    perror("getcwd");
                    return NULL;
                }
                strcat(file_path, "/");
                strcat(file_path, serve_dir);
                strcat(file_path, buf);

                if(write_http_response(client_fd, file_path)!=0){       //write response to client
                    fprintf(stderr,"error in write\n");
                }
            }      
        }
    }
    return NULL;
}


int main(int argc, char **argv) {
    // First command is directory to serve, second command is port
    if (argc != 3) {
        printf("Usage: %s <directory> <port>\n", argv[0]);
        return 1;
    }
    serve_dir = argv[1];
    const char *port = argv[2];

    
    connection_queue_t queue;
    if (connection_queue_init(&queue) != 0) {       //initialize the queue
        fprintf(stderr, "Failed to initialize queue\n");
        return 1;
    }

    sigset_t mask;      //set up worker threads to ignore all signals
    sigset_t oldset;
    if(sigfillset(&mask)){
        perror("sigfillset");
        return 1;
    }
    if(sigprocmask(SIG_BLOCK, &mask, &oldset)){
        perror("sigprocmask");
        return 1;
    }

    struct addrinfo hints;          //set up structures for the server
    if(memset(&hints, 0, sizeof(hints))!=&hints){
        perror("memset");
        return 1;
    }
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    struct addrinfo *server;
    int return_val;

    if((return_val = getaddrinfo(NULL, port, &hints, &server))!=0){
        fprintf(stderr,"getaddrinfo failed: %s\n", gai_strerror(return_val));
        return 1;
    }

    int sock_fd = socket(server->ai_family, server->ai_socktype, server->ai_protocol);
    if(sock_fd==-1){
        perror("socket");
        freeaddrinfo(server);
        return 1;

    }
    if(bind(sock_fd, server->ai_addr, server->ai_addrlen)==-1){
        perror("bind");
        freeaddrinfo(server);
        close(sock_fd);
        return 1;
    }
    freeaddrinfo(server);
    if(listen(sock_fd, LISTEN_QUEUE_LEN)==-1){
        perror("listen");
        close(sock_fd);
        return 1;
    }

    int result;

    // Create multiple producer threads
    pthread_t consumer_threads[N_THREADS]; 
    for (int i = 0; i < N_THREADS; i++) {
        if ((result = pthread_create(consumer_threads + i, NULL, consumer_thread_func, &queue)) != 0) {     //give each thread the initiated queue as its arguement
            fprintf(stderr, "pthread_create: %s\n", strerror(result));
            return 1;
        }
    }

    if(sigprocmask(SIG_SETMASK, &oldset, NULL)){        //set up the main producer thread to be able to handle a SIGINT for shutdown
        perror("sigprocmask");
        close(sock_fd);
        return 1;
    }
    struct sigaction sact;
    sact.sa_handler = handle_sigint;
    if(sigfillset(&sact.sa_mask)){
        perror("sigfillset");
        close(sock_fd);
        return 1;
    }
    sact.sa_flags = 0;
    if(sigaction(SIGINT, &sact, NULL)==-1){     //detect SIGINTs
        perror("sigaction");
        close(sock_fd);
        return 1;
    }

    int exit_code = 0;

    while(keep_going !=0){      //while there hasn't been a signal to interupt
        int client_fd = accept(sock_fd,NULL,NULL);      //accpet the new client
        if(client_fd==-1){      //check if a signal was the cause of the interuption
            if(errno!=EINTR){
                perror("accept");
                exit_code=1;
                break;
            }
            else{
                break;
            }
        }
        if(connection_enqueue(&queue, client_fd)){      //otherwise add the new client to the queue
            if(errno!=EINTR){
                fprintf(stderr,"error in connection_enqueue\n");
                exit_code=1;
                break;
            }
        }
    }
    if(connection_queue_shutdown(&queue)){      //shut down the queue
        fprintf(stderr,"error in connection_queue_shutdown\n");
        exit_code=1;
    }
    
    if(close(sock_fd)==-1){     //and close the socket
        perror("close");
        return 1;
    }


    // Join all threads
    // Even if an individual join fails, try to complete remaining joins
    for (int i = 0; i < N_THREADS; i++) {
        if ((result = pthread_join(consumer_threads[i], NULL)) != 0) {
            fprintf(stderr, "pthread_join: %s\n", strerror(result));
            exit_code = 1;
        }
    }
    if(connection_queue_free(&queue)){      //free up any memory and destroy any condition variables
        fprintf(stderr,"error in connection_queue_free\n");
        exit_code=1;
    }
    return exit_code;
}
