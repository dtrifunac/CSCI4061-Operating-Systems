#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include "http.h"

#define BUFSIZE 512

const char *get_mime_type(const char *file_extension) {
    if (strcmp(".txt", file_extension) == 0) {
        return "text/plain";
    } else if (strcmp(".html", file_extension) == 0) {
        return "text/html";
    } else if (strcmp(".jpg", file_extension) == 0) {
        return "image/jpeg";
    } else if (strcmp(".png", file_extension) == 0) {
        return "image/png";
    } else if (strcmp(".pdf", file_extension) == 0) {
        return "application/pdf";
    }

    return NULL;
}

int read_http_request(int fd, char *resource_name) {            //read a request
    int sock_fd_copy = dup(fd);     //create a copy to use higher lever functions
    if (sock_fd_copy == -1) {
        perror("dup");
        return -1;
    }
    FILE *socket_stream = fdopen(sock_fd_copy, "r");        //open the new copy
    if (socket_stream == NULL) {
        perror("fdopen");
        if(close(sock_fd_copy)){
            perror("close");
        }
        return -1;
    }
    // Disable the usual stdio buffering
    if (setvbuf(socket_stream, NULL, _IONBF, 0) != 0) {
        perror("setvbuf");
        if (fclose(socket_stream) != 0) {
            perror("fclose");
        }
        return -1;
    }

    char buf[BUFSIZE];
    if(fgets(buf, BUFSIZE, socket_stream)==NULL){     //get the first line of the request to get needed info
        perror("fgets");
        if (fclose(socket_stream) != 0) {
            perror("fclose");
        }
        return -1;
    }
    char *out;
    out = strtok(buf, " ");
    if(strcmp(out, "GET")!=0){      //make sure it is a GET request
        printf("GET response invalid\n");
        if (fclose(socket_stream) != 0) {
            perror("fclose");
        }
        return -1;
    }
    out = strtok(NULL, " ");        //get the requested file name
    out = strcpy(resource_name, out);     //copy the name to the input param
    if(strcmp(out,resource_name)){
        fprintf(stderr,"strcmp error\n");
        if (fclose(socket_stream) != 0) {
            perror("fclose");
        }
        return -1;
    }

    // Keep consuming lines until we find an empty line
    // This marks the end of the response headers and beginning of actual content
    while (fgets(buf, BUFSIZE, socket_stream) != NULL) {
        if (strcmp(buf, "\r\n") == 0) {
            break;
        }
    }

    if (fclose(socket_stream) != 0) {
        perror("fclose");
        return -1;
    }
    return 0;
}

int write_http_response(int fd, const char *resource_path) {        //write back response to client
    struct stat st;
    char buf[BUFSIZE] = {0};
    
    if(stat(resource_path, &st)==-1){       //get file info and write response if file doesn't exist
        write(fd, "HTTP/1.0 404 Not Found\r\nContent-Length: 0\r\n\r\n",sizeof("HTTP/1.0 404 Not Found\r\nContent-Length: 0\r\n\r\n"));
        fprintf(stderr,"file not found\n");
        return 0;
    }
    int file_size = st.st_size;
    int fp = open(resource_path,O_RDONLY);      //otherwise open the file to read and write its contents to the client
    if(fp==-1){
        perror("open");
        return -1;
    }
    
    if(sprintf(buf, "HTTP/1.0 200 OK\r\n")<0){           //fill in the HTTP header
        perror("sprintf");
        if(close(fp)){
            perror("close");
        }
        return -1;
    }           
    if(write(fd, buf, strlen(buf))<0){
        perror("write");
        if(close(fp)){
            perror("close");
        }
        return -1;
    }
    if(sprintf(buf, "Content-Length: %d\r\n", file_size)<0){
        perror("sprintf");
        if(close(fp)){
            perror("close");
        }
        return -1;
    }
    if(write(fd, buf, strlen(buf))<0){
        perror("write");
        if(close(fp)){
            perror("close");
        }
        return -1;
    }
    const char* mim = get_mime_type(strchr(resource_path,'.'));
    if(mim==NULL){
        fprintf(stderr,"get_mim_type error\n");
        if(close(fp)){
            perror("close");
        }
        return -1;
    }
    if(sprintf(buf, "Content-Type: %s\r\n", mim)<0){
        perror("sprintf");
        if(close(fp)){
            perror("close");
        }
        return -1;
    }
    if(write(fd, buf, strlen(buf))<0){
        perror("write");
        if(close(fp)){
            perror("close");
        }
        return -1;
    }
    if(sprintf(buf, "\r\n")<0){
        perror("sprintf");
        if(close(fp)){
            perror("close");
        }
        return -1;
    }
    if(write(fd, buf, strlen(buf))<0){
        perror("write");
        if(close(fp)){
            perror("close");
        }
        return -1;
    }

    int bytes_read;
    while((bytes_read = read(fp,buf,sizeof(buf)))>0){       //write the contents of the file to the client
        if(bytes_read<0){
            perror("read");
            if(close(fp)){
                perror("close");
            }
            return -1;
        }
        if(write(fd, buf, bytes_read)!=bytes_read){
            perror("write");
            if(close(fp)){
                perror("close");
            }
            return -1;
        }
    }

    if(close(fp)){
        perror("close");
        return -1;
    }
    return 0;
}
