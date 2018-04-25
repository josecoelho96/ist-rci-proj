#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "com_reqserver.h"
#include "udp.h"
#include "tcp.h"

int get_ds_server(int fd, struct sockaddr_in *addr, int service) {
    char msg[30];
    sprintf(msg, "GET_DS_SERVER %d", service);
    return udp_send_str(fd, addr, msg);
}

int my_service(int fd, struct sockaddr_in *addr, bool status) {
    return udp_send_str(fd, addr, (status) ? "MY_SERVICE ON" : "MY_SERVICE OFF");
}

int request_service(int cs_fd, struct sockaddr_in *cs_addr, int service_id, int *s_fd, struct sockaddr_in *s_addr, bool *has_service_requested) {
    int ret_val;
    int recv_id, recv_upt;
    char *recv_ip = calloc(20, sizeof(char));

    if (*has_service_requested) {
        printf("ERROR: You already requested a service.\n");
        return 0;
    }

    // Communication with central server -> GET_DS_SERVER $service_id
    if (get_ds_server(cs_fd, cs_addr, service_id) == -1) {
        return -1;
    }
    // Read and validate output
    ret_val = response_cserver_client(cs_fd, cs_addr, &recv_id, &recv_ip, &recv_upt);
    if (ret_val != 0) {
        return ret_val;
    }

    // Start communication with dispatch server
    if (udp_init(s_fd, s_addr, recv_ip, recv_upt) == -1) {
        printf("ERROR: Cannot start UDP client to communicate with dispatch server.\n");
        return -1;
    }
    // Start service -> MY_SERVICE ON
    if (my_service(*s_fd, s_addr, true) == -1) {
        return -1;
    }

    ret_val = response_service_client(*s_fd, s_addr, true);
    if (ret_val != 0) {
        return ret_val;
    }

    *has_service_requested = true;

    free(recv_ip);

    return 0;
}

int terminate_service(int s_fd, struct sockaddr_in *s_addr, bool *has_service_requested) {
    int ret_val;

    if (!*has_service_requested) {
        printf("ERROR: You need to have requested a service.\n");
        return 0;
    }

    // End service
    if (my_service(s_fd, s_addr, false) == -1) {
        return -1;
    }

    // Check if service ended
    ret_val = response_service_client(s_fd, s_addr, false);
    if (ret_val != 0) {
        return ret_val;
    }

    *has_service_requested = false;

    return 0;
}

int response_cserver_client(int fd, struct sockaddr_in *addr, int *id, char **ip, int *upt) {
    int recv_buf_len = 128;
    char recv_buf[recv_buf_len];

    if (udp_recv_str(fd, addr, recv_buf, recv_buf_len) == -1) {
        return -1;
    }

    // OK id;ip;upt
    if (sscanf(recv_buf, "OK %d;%16[^;];%d", id, *ip, upt) != 3) {
        return -1;
    }

    // Since all ids are positive integers, we only need to check if the id is 0
    if (*id == 0) {
        printf("ERROR: No dispatch server is available. Try again later.\n");
        return -2;
    }

    return 0;
}

int response_service_client(int fd, struct sockaddr_in *addr, bool expected_status) {
    int recv_buf_len = 128;
    char recv_buf[recv_buf_len];

    if (udp_recv_str(fd, addr, recv_buf, recv_buf_len) == -1) {
        return -1;
    }

    if (expected_status) {
        if (strcmp(recv_buf, "YOUR_SERVICE ON") == 0) {
            return 0;
        } else {
            printf("ERROR: Server couldn't start the requested service.\n");
            return -2;
        }
    } else {
        if (strcmp(recv_buf, "YOUR_SERVICE OFF") == 0) {
            return 0;
        } else {
            printf("ERROR: Server couldn't stop the requested service.\n");
            return -2;
        }
    }
}