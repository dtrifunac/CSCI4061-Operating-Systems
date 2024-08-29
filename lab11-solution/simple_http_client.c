#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#define BUFSIZE 4096
#define N_FILES 3
#define HOST "23.23.63.223"
#define PORT "80"

const char *files[] = {
    "index.html",
    "gatsby.txt",
    "ocelot.jpg"
};

// Establish a TCP connection with a server
// host: Name of server to connect to
// port: Port to connect to
// Returns a new TCP socket file descriptor on success or -1 on error
int connect_to_server(const char *host, const char *port) {
    // Set up hints
    struct addrinfo(hints);
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo *server;
    // Set up address info for later system calls
    int ret_val = getaddrinfo(host, port, &hints, &server);
    if (ret_val != 0) {
        fprintf(stderr, "getaddrinfo failed: %s\n", gai_strerror(ret_val));
        return -1;
    }

    // Initialize socket and connection
    int sock_fd = socket(server->ai_family, server->ai_socktype, server->ai_protocol);
    if (sock_fd == -1) {
        perror("socket");
        freeaddrinfo(server);
        return -1;
    }
    if (connect(sock_fd, server->ai_addr, server->ai_addrlen) == -1) {
        perror("socket");
        freeaddrinfo(server);
        return -1;
    }

    freeaddrinfo(server);
    return sock_fd;
}

// Writes an HTTP request to a TCP socket
// sock_fd: File descriptor to write request to
// file: Name of the file to request from server
// Returns 0 on success or -1 on error
int write_http_request(int sock_fd, const char *file) {
    char buf[BUFSIZE];
    snprintf(buf, BUFSIZE, "GET /%s HTTP/1.0\r\n\r\n", file);
    if (write(sock_fd, buf, strlen(buf)) == -1) {
        perror("write");
        return -1;
    }
    return 0;
}

// Reads an HTTP response from a TCP socket
// sock_fd: File descriptor to read response from
// file: Name of file to save response body to
// Returns 0 on success or -1 on error
int read_http_response(int sock_fd, const char *file) {
    // stdio FILE * gives us 'fgets()' to easily read line by line
    int sock_fd_copy = dup(sock_fd);
    if (sock_fd_copy == -1) {
        perror("dup");
        return -1;
    }
    FILE *socket_stream = fdopen(sock_fd_copy, "r");
    if (socket_stream == NULL) {
        perror("fdopen");
        close(sock_fd_copy);
        return -1;
    }
    // Disable the usual stdio buffering
    if (setvbuf(socket_stream, NULL, _IONBF, 0) != 0) {
        perror("setvbuf");
        fclose(socket_stream);
        return -1;
    }

    // Keep consuming lines until we find an empty line
    // This marks the end of the response headers and beginning of actual content
    char buf[BUFSIZE];
    while (fgets(buf, BUFSIZE, socket_stream) != NULL) {
        if (strcmp(buf, "\r\n") == 0) {
            break;
        }
    }

    if (fclose(socket_stream) != 0) {
        perror("fclose");
        return -1;
    }

    // Open destination file for writing
    int file_fd = open(file, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR);
    if (file_fd == -1) {
        perror("open");
        return -1;
    }

    int bytes_read;
    while ((bytes_read = read(sock_fd, buf, BUFSIZE)) > 0) {
        if (write(file_fd, buf, bytes_read) == -1) {
            perror("write");
            close(file_fd);
            return -1;
        }
    }
    if (bytes_read == -1) {
        perror("read");
        close(file_fd);
        return -1;
    }

    if (close(file_fd) != 0) {
        perror("close");
        return -1;
    }

    return 0;
}

int main(int argc, char **argv) {
    // Request each file in above array
    for (int i = 0; i < N_FILES; i++) {
        int sock_fd = connect_to_server(HOST, PORT);
        if (sock_fd == -1) {
            return 1;
        }

        if (write_http_request(sock_fd, files[i]) == -1) {
            close(sock_fd);
            return 1;
        }
        if (read_http_response(sock_fd, files[i]) == -1) {
            close(sock_fd);
            return 1;
        }

        if (close(sock_fd) == -1) {
            return 1;
        }
    }

    return 0;
}
