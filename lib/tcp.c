#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "tcp.h"
#include "misc.h"

int tcp_init(int *fd, struct sockaddr_in *addr, char *ip, int port) {
    if ((*fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        return -1;
    }

    memset((void*) addr, (int)'\0', sizeof(*addr));
    if (inet_pton(AF_INET, ip, &(addr->sin_addr)) != 1) {
        return -1;
    }
    addr->sin_family = AF_INET;
    addr->sin_port = htons(port);

    return connect(*fd, (struct sockaddr*)addr, sizeof(*addr));
}

int tcp_init_server(int *fd, struct sockaddr_in *addr, int port) {
    if ((*fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        return -1;
    }

    memset((void*) addr, (int)'\0', sizeof(*addr));
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = htonl(INADDR_ANY);
    addr->sin_port = htons(port);

    if (bind(*fd, (struct sockaddr*) addr, sizeof(*addr)) == -1) {
        return -1;
    }

    if (listen(*fd, 5) == -1) {
        return -1;
    }

    return 0;
}

int tcp_close(int *fd) {
    if (*fd != NOT_SET) {
        if (close(*fd) == -1) {
            return -1;
        }
        *fd = NOT_SET;
    }
    return 0;
}

int tcp_write_str(int fd, char *msg) {
    // Newly allocated message is no longer a string (replaced \0 by \n)
    int msg_term_len = strlen(msg) + 1;
    char *msg_term = (char *) calloc((msg_term_len), sizeof(char));
    strcpy(msg_term, msg);
    msg_term[msg_term_len - 1] = '\n';

    for (int n, n_left = msg_term_len; n_left > 0; n_left -= n, msg += n) {
        n = write(fd, msg_term, n_left);
        if (n <= 0) {
            return -1;
        }
    }

    free(msg_term);
    return 0;
}

int tcp_read_str(int fd, char *buf, int buf_len) {
    int n_read = 0;
    char c_read;

    for (int n; buf_len > n_read; ++n_read) {
        n = read(fd, &c_read, 1);
        if (n == 1 && c_read != '\n') {
            // Message not finished
            buf[n_read] = c_read;
        } else if (n == 1 && c_read == '\n') {
            // Message finished
            break;
        } else if (n == 0) {
            // Server closed
            return -2;
        } else {
            return -1;
        }
    }

    // Make sure our buffer is a valid string (terminated by a \0)
    if (n_read + 1 < buf_len) {
        buf[n_read] = '\0';
    } else {
        // Buffer is too small
        return -1;
    }

    return 0;
}

int tcp_accept(int fd, struct sockaddr_in *addr) {
    socklen_t addrlen = sizeof(*addr);

    return accept(fd, (struct sockaddr*)addr, &addrlen);
}