#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "udp.h"

int udp_init(int *fd, struct sockaddr_in *addr, char *ip, int port) {
    if ((*fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        return -1;
    }

    memset((void*) addr, (int)'\0', sizeof(*addr));
    if (inet_pton(AF_INET, ip, &(addr->sin_addr)) != 1) {
        return -1;
    }
    addr->sin_family = AF_INET;
    addr->sin_port = htons(port);

    return 0;
}

int udp_init_server(int *fd, struct sockaddr_in *addr, int port) {
    if ((*fd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
        return -1;
    }

    memset((void*) addr, (int)'\0', sizeof(*addr));
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = htonl(INADDR_ANY);
    addr->sin_port = htons(port);

    if (bind(*fd, (struct sockaddr*) addr, sizeof(*addr)) == -1) {
        return -1;
    }
    return 0;
}

int udp_close(int *fd) {
    if (*fd != NOT_SET) {
        if (close(*fd) == -1) {
            return -1;
        }
        *fd = NOT_SET;
    }
    return 0;
}

int udp_send_str(int fd, struct sockaddr_in *addr, char *msg) {
    int n;
    n = sendto(fd, msg, strlen(msg), 0, (struct sockaddr*) addr, sizeof(*addr));
    return (n != -1 && n == strlen(msg)) ? 0 : -1;
}

int udp_recv_str(int fd, struct sockaddr_in *addr, char *buf, int buf_len) {
    int n;
    socklen_t addrlen = sizeof(*addr);

    switch(n = recvfrom(fd, buf, buf_len, 0, (struct sockaddr*) addr, &addrlen)) {
        case -1:
            return -1;
        case 0:
            // Server terminated connection
            return -2;
    }
    if (n + 1 < buf_len) {
        buf[n] = '\0';
        return n;
    } else {
        // Buffer is too small
        return -1;
    }
}