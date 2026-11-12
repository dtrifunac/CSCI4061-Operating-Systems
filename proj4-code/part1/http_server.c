#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "http.h"

#define BUFSIZE 512
#define LISTEN_QUEUE_LEN 5

int keep_going = 1;

void handle_sigint(int signo) {
    keep_going = 0;
}

int main(int argc, char **argv) {
    // First command is directory to serve, second command is port
    if (argc != 3) {
        printf("Usage: %s <directory> <port>\n", argv[0]);
        return 1;
    }
    // Uncomment the lines below to use these definitions:
    const char *serve_dir = argv[1];
    const char *port = argv[2];

    struct sigaction sact;      //set up the signal handler
    sact.sa_handler = handle_sigint;
    if(sigfillset(&sact.sa_mask)){
        perror("sigfillset");
        return 1;
    }
    sact.sa_flags = 0;
    if(sigaction(SIGINT, &sact, NULL)==-1){
        perror("sigaction");
        return 1;
    }

    struct addrinfo hints;              //start setting up the socket and such for the server
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
    char buf[BUFSIZE];
    while(keep_going !=0){      //while there hasn't been a signal to interupt
        // printf("Waiting for a client to connect\n");
        int client_fd = accept(sock_fd,NULL,NULL);
        if(client_fd==-1){
            if(errno!=EINTR){
                perror("accept");
                close(sock_fd);
                return 1;
            }
            else{break;}
        }

        if(read_http_request(client_fd, buf)==0){          //read the request and only continue if successful
            char file_path[BUFSIZE];
            if(getcwd(file_path,sizeof(file_path))==NULL){        //set up file path to return
            if(errno!=EINTR){
                    perror("getcwd");
                    close(sock_fd);
                    return 1;
                }
                else{break;}
            }
            strcat(file_path, "/");
            strcat(file_path, serve_dir);
            strcat(file_path, buf);

            if(write_http_response(client_fd, file_path)!=0){       //write response to client
                if(errno!=EINTR){
                    fprintf(stderr,"error in write\n");
                    close(sock_fd);
                    return 1;
                }
                else{break;}
            }
        } 
        else{
            if(errno!=EINTR){
                fprintf(stderr,"read_http_request\n");
                close(sock_fd);
                return 1;
            }
            else{break;}
        }
    }

    if(close(sock_fd)==-1){
        perror("close");
        return 1;
    }

    return 0;
}
