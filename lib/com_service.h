#ifndef COM_SERVICE_H
#define COM_SERVICE_H

#include <stdbool.h>

typedef struct server {
    int id; // server id
    char *ip; // server ip
    int upt; // server udp port to receive clients requests
    int tpt; // server tcp port to receive connection requests from servers
    bool is_start_server;
    bool is_ds_server;
    bool is_available;
    bool is_ring_available;
    bool is_leaving;
    int service_id; //service id
    char *csip; // central server ip
    int cspt; // central server udp port
    int cs_fd; //UDP client
    int reqserver_fd; //UDP server
    int tcp_sv_ls_fd; //TCP server (listening)
    int tcp_server_fd; //TCP server
    int tcp_client_fd; //TCP client (talk to start server or successor)
    struct sockaddr_in cs_addr;
    struct sockaddr_in reqserver_addr;
    struct sockaddr_in tcp_sv_ls_addr;
    struct sockaddr_in tcp_client_addr;
    int successor_id;
    char *successor_ip;
    int successor_tpt;
} server;

#define NOT_SET -1

server new_server(void);

int reset_server_service(server *s);
int get_start(server *s, int requested_service);
int set_start(server *s, int requested_service);
int withdraw_start(server *s);
int set_ds(server *s, int requested_service);
int withdraw_ds(server *s);
void show_state(server *s);
int response_cserver_service(server *s, int *id, int *id2, char *ip, int *tpt);
int request_service_client(server *s);
int new_start(server *s);
int new(server *s);
void show_state(server *s);
int token(server *s, char type, int server_id, char *server_ip, int server_tpt);
int change_successor(server *s, int recv_id2, char *recv_ip, int recv_tpt);
int join_service(server *s, int requested_service);
int leave_service(server *s, int exit);
int response_cserver_service(server *s, int *id, int *id2, char *ip, int *tpt);
int response_service_service(server *s);

#endif