#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

#define DEPT_LEN 5
#define NAME_LEN 128

// Define the structs for the request and response messages
struct request {
    char department[5];
    int number;
};

struct response {
    int status;
    char name[NAME_LEN];
};

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <Department> <Number>\n", argv[0]);
        return 1;
    }

    char *department = argv[1];
    int number = atoi(argv[2]);

    int sockfd;
    struct addrinfo hints, *serverinfo,*p;
    memset(&hints,0,sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    

    if(getaddrinfo("23.23.63.223", "4061", &hints, &serverinfo)){
        perror("getaddrinfo");
        // return -1;
    }
    // printf("here");
    p = serverinfo;
    if((sockfd = socket(p->ai_family, p->ai_socktype,p->ai_protocol))==-1){
        perror("socket");
        // return -1;
    }

    struct request req;
    strncpy(req.department, department, 4);
    req.department[4] = '\0';
    req.number = htonl(number);

    if(sendto(sockfd, &req, sizeof req, 0, p->ai_addr, p->ai_addrlen)==-1){
        perror("sendto");
        // return -1;
    }

    struct response res;
    socklen_t addr_len = sizeof(res);
    if(recvfrom(sockfd, &res, sizeof res, 0, p->ai_addr, &addr_len)==-1){
        perror("recvfrom");
        // return -1;
    }

    int status = ntohl(res.status);

    if (status == 0) {
        printf("Course Title: %s\n", res.name);
    } else {
        printf("Course Not Found\n");
    }

    freeaddrinfo(p);
    return 0;
}