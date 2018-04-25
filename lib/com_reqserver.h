#ifndef COM_REQSERVER_H
#define COM_REQSERVER_H

#include <stdbool.h>

#define NOT_SET -1

int get_ds_server(int fd, struct sockaddr_in *addr, int service);
int my_service(int fd, struct sockaddr_in *addr, bool status);
int request_service(int cs_fd, struct sockaddr_in *cs_addr, int service_id, int *s_fd, struct sockaddr_in *s_addr, bool *has_service_requested);
int terminate_service(int s_fd, struct sockaddr_in *s_addr, bool *has_service_requested);
int response_cserver_client(int fd, struct sockaddr_in *addr, int *id, char **ip, int *upt);
int response_service_client(int fd, struct sockaddr_in *addr, bool expected_status);

#endif