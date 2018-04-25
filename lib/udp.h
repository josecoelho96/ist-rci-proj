#ifndef UDP_H
#define UDP_H

#include "misc.h"

int udp_init(int *fd, struct sockaddr_in *addr, char *ip, int port);
int udp_init_server(int *fd, struct sockaddr_in *addr, int port);
int udp_send_str(int fd, struct sockaddr_in *addr, char *msg);
int udp_recv_str(int fd, struct sockaddr_in *addr, char *buf, int buf_len);
int udp_close(int *fd);

#endif