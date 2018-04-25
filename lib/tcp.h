#ifndef TCP_H
#define TCP_H

int tcp_init(int *fd, struct sockaddr_in *addr, char *ip, int port);
int tcp_init_server(int *fd, struct sockaddr_in *addr, int port);
int tcp_close(int *fd);
int tcp_write_str(int fd, char *msg);
int tcp_read_str(int fd, char *buf, int buf_len);
int tcp_accept(int fd, struct sockaddr_in *addr);

#endif