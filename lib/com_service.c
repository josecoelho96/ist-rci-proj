#include <arpa/inet.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "com_service.h"
#include "udp.h"
#include "tcp.h"

server new_server(void) {
    server s;
    s.id = NOT_SET;
    s.ip = NULL;
    s.upt = NOT_SET;
    s.tpt = NOT_SET;
    s.is_start_server = false;
    s.is_ds_server = false;
    s.is_available = false;
    s.is_ring_available = false;
    s.is_leaving = false;
    s.service_id = NOT_SET;
    s.csip = NULL;
    s.cspt = NOT_SET;
    s.cs_fd = NOT_SET;
    s.reqserver_fd = NOT_SET;
    s.tcp_sv_ls_fd = NOT_SET;
    s.tcp_server_fd = NOT_SET;
    s.tcp_client_fd = NOT_SET;
    s.successor_id = NOT_SET;
    s.successor_ip = (char *)calloc(16, sizeof(char));
    s.successor_tpt = NOT_SET;
    return s;
}

int reset_server_service(server *s) {
    s->is_start_server = false;
    s->is_ds_server = false;
    s->is_available = false;
    s->is_ring_available = false;
    s->is_leaving = false;
    s->service_id = NOT_SET;
    if (tcp_close(&(s->tcp_server_fd)) == -1 ||
        tcp_close(&(s->tcp_client_fd)) == -1
    ) {
        return -1;
    }
    memset((void*) &(s->tcp_client_addr), (int)'\0', sizeof(s->tcp_client_addr));
    s->successor_id = NOT_SET;
    memset(s->successor_ip, '\0', 16 * sizeof(char));
    s->successor_tpt = NOT_SET;

    return 0;
}

int get_start(server *s, int requested_service) {
    char msg[64];
    sprintf(msg, "GET_START %d;%d", requested_service, s->id);
    return udp_send_str(s->cs_fd, &(s->cs_addr), msg);
}

int set_start(server *s, int requested_service) {
    char msg[64];
    sprintf(msg, "SET_START %d;%d;%s;%d", requested_service, s->id, s->ip, s->tpt);
    return udp_send_str(s->cs_fd, &(s->cs_addr), msg);
}

int withdraw_start(server *s) {
    char msg[64];
    sprintf(msg, "WITHDRAW_START %d;%d", s->service_id, s->id);
    return udp_send_str(s->cs_fd, &(s->cs_addr), msg);
}

int set_ds(server *s, int requested_service) {
    char msg[64];
    sprintf(msg, "SET_DS %d;%d;%s;%d", requested_service, s->id, s->ip, s->upt);
    return udp_send_str(s->cs_fd, &(s->cs_addr), msg);
}

int withdraw_ds(server *s) {
    char msg[64];
    sprintf(msg, "WITHDRAW_DS %d;%d", s->service_id, s->id);
    return udp_send_str(s->cs_fd, &(s->cs_addr), msg);
}

int new_start(server *s) {
    return tcp_write_str(s->tcp_client_fd, "NEW_START");
}

int new(server *s) {
    char msg[64];
    sprintf(msg, "NEW %d;%s;%d", s->id, s->ip, s->tpt);
    return tcp_write_str(s->tcp_client_fd, msg);
}

void show_state(server *s) {
    printf("Is DS: %d.\n", s->is_ds_server);
    printf("Is SS: %d.\n", s->is_start_server);
    printf("Own availability: %d.\n", s->is_available);
    printf("Ring availability: %d.\n", s->is_ring_available);
    printf("Successor Id: %d.\n", s->successor_id);
}

int token(server *s, char type, int server_id, char *server_ip, int server_tpt) {
    char msg[64];

    switch(type) {
        case 'N':
        case 'O':
            sprintf(msg, "TOKEN %d;%c;%d;%s;%d", s->id, type, server_id, server_ip, server_tpt);
            return tcp_write_str(s->tcp_client_fd, msg);
        case 'S':
        case 'T':
        case 'I':
        case 'D':
            sprintf(msg, "TOKEN %d;%c", server_id, type);
            return tcp_write_str(s->tcp_client_fd, msg);
        default:
            return -2;
    }
}

int change_successor(server *s, int successor_id, char *successor_ip, int successor_tpt) {
    if(s->successor_id != NOT_SET && tcp_close(&(s->tcp_client_fd)) == -1) {
        printf("ERROR: Failure closing connection to previous successor.\n");
        return -1;
    }
    if (s->id != successor_id) {
        // Successor isn't myself
        if (tcp_init(&(s->tcp_client_fd), &(s->tcp_client_addr), successor_ip, successor_tpt) == -1) {
            printf("ERROR: Failure connecting to new successor.\n");
            return -1;
        }
        s->successor_id = successor_id;
        strcpy(s->successor_ip, successor_ip);
        s->successor_tpt = successor_tpt;
    } else {
        s->successor_id = NOT_SET;
        memset(s->successor_ip, '\0', 16 * sizeof(char));
        s->successor_tpt = NOT_SET;
    }

    return 0;
}

int join_service(server *s, int requested_service) {
    int recv_id, recv_id2, recv_tpt;
    char recv_ip[16];

    // Get start server from central server
    if (get_start(s, requested_service) == -1 ||
        response_cserver_service(s, &recv_id, &recv_id2, recv_ip, &recv_tpt) == -1
    ) {
        return -1;
    }

    if (recv_id2 < 0) {
        printf("ERROR: Received invalid successor: %d\n", recv_id2);
        return -1;
    } else if (recv_id2 == 0) {
        // Set as start server
        if (set_start(s, requested_service) == -1 ||
            response_cserver_service(s, &recv_id, &recv_id2, recv_ip, &recv_tpt) == -1
        ) {
            return -1;
        }
        s->is_start_server = true;

        // Set as dispatch server
        if (set_ds(s, requested_service) == -1 ||
            response_cserver_service(s, &recv_id, &recv_id2, recv_ip, &recv_tpt) == -1
        ) {
            return -1;
        }
        s->is_ds_server = true;
    } else {
        // Join an existing ring
        if (change_successor(s, recv_id2, recv_ip, recv_tpt) == -1 || new(s) == -1) {
            return -1;
        }

        if (token(s, 'D', s->id, recv_ip, recv_tpt) == -1) {
            return -1;
        }
    }

    s->is_available = true;
    s->is_ring_available = true;
    s->service_id = requested_service;

    return 0;
}

int leave_service(server *s, int exit) {
    int recv_id, recv_tpt, recv_id2;
    char recv_ip[16];

    if (s->service_id == NOT_SET) {
        if (!exit) {
            // Not in ring -> cannot leave
            printf("ERROR: Can only leave if on a ring.\n");
        }
    } else if (!s->is_available) {
        // Not available -> cannot leave
        return -2;
    } else {
        s->is_leaving = true;
        if (s->successor_id == NOT_SET) {
            if (withdraw_start(s) == -1 ||
                response_cserver_service(s, &recv_id, &recv_id2, recv_ip, &recv_tpt) == -1 ||
                withdraw_ds(s) == -1 ||
                response_cserver_service(s, &recv_id, &recv_id2, recv_ip, &recv_tpt) == -1 ||
                reset_server_service(s) == -1
            ) {
                return -1;
            }
        } else {
            if (s->is_start_server) {
                if (withdraw_start(s) == -1 ||
                    response_cserver_service(s, &recv_id, &recv_id2, recv_ip, &recv_tpt) == -1 ||
                    new_start(s) == -1
                ) {
                    return -1;
                }
                s->is_start_server = false;
            }
            if (s->is_ds_server) {
                if (withdraw_ds(s) == -1 ||
                    response_cserver_service(s, &recv_id, &recv_id2, recv_ip, &recv_tpt) == -1
                ) {
                    return -1;
                }
                s->is_ds_server = false;
                // Send TOKEN to successor
                if (token(s, 'S', s->id, 0, 0) == -1) {
                    return -1;
                }
            } else {
                // Send TOKEN to successor
                if (token(s, 'O', s->successor_id, s->successor_ip, s->successor_tpt)) {
                    return -1;
                }
            }
        }
    }
    return 0;
}

int request_service_client(server *s) {
    int recv_buf_len = 128;
    char recv_buf[recv_buf_len];
    int recv_id, recv_id2, recv_tpt;
    char recv_ip[16];

    if (udp_recv_str(s->reqserver_fd, &(s->reqserver_addr), recv_buf, recv_buf_len) == -1) {
        return -1;
    }

    if (strcmp(recv_buf, "MY_SERVICE ON") == 0) {
        // Withdraw DS
        if (withdraw_ds(s) == -1 ||
            response_cserver_service(s, &recv_id, &recv_id2, recv_ip, &recv_tpt) == -1
        ) {
            printf("ERROR: Failure while removing server as dispatch server.\n");
            if (udp_send_str(s->reqserver_fd, &(s->reqserver_addr), "YOUR_SERVICE OFF") == -1) {
                return -1;
            }
            return -1;
        }
        s->is_ds_server = false;

        // Send information to client
        s->is_available = false;
        if (udp_send_str(s->reqserver_fd, &(s->reqserver_addr), "YOUR_SERVICE ON") == -1) {
            return -1;
        }

        if (s->successor_id != NOT_SET) {
            // Send S token to next server
            if (token(s, 'S', recv_id, recv_ip, recv_tpt) == -1) {
                return -1;
            }
        } else {
            // Server is alone on ring
            s->is_ring_available = false;
        }
    } else if (strcmp(recv_buf, "MY_SERVICE OFF") == 0) {
        s->is_available = true;
        // Send D token if ring was unavailable
        if (!s->is_ring_available) {
            if (s->successor_id == NOT_SET) {
                // Set as dispatch server
                if (set_ds(s, s->service_id) == -1 ||
                    response_cserver_service(s, &recv_id, &recv_id2, recv_ip, &recv_tpt) == -1
                ) {
                    return -1;
                }
                s->is_ds_server = true;
            } else {
                if (token(s, 'D', s->id, recv_ip, recv_tpt) == -1) {
                    return -1;
                }
            }

            s->is_ring_available = true;
        }
        return udp_send_str(s->reqserver_fd, &(s->reqserver_addr), "YOUR_SERVICE OFF");
    } else {
        return -1;
    }

    return 0;
}

int response_cserver_service(server *s, int *id, int *id2, char *ip, int *tpt) {
    int recv_buf_len = 128;
    char recv_buf[recv_buf_len];

    if (udp_recv_str(s->cs_fd, &(s->cs_addr), recv_buf, recv_buf_len) == -1) {
        return -1;
    }

    // OK id;id2;ip;tpt
    if (sscanf(recv_buf, "OK %d;%d;%16[^;];%d", id, id2, ip, tpt) != 4) {
        return -1;
    }

    if (*id == 0) {
        printf("ERROR: Central server wasn't able to process the request made.\n");
        return -1;
    }

    return 0;
}

int response_service_service(server *s) {
    int recv_buf_len = 128;
    char recv_buf[recv_buf_len];
    char code[30];
    char values[50];
    int recv_id, recv_tpt, recv_id2, recv_id_cs, recv_tpt_cs, recv_id2_cs;
    int tok_read;
    char recv_ip[16], recv_ip_cs[16];
    char recv_type;

    switch(tcp_read_str(s->tcp_server_fd, recv_buf, recv_buf_len)) {
        case -1: return -1;
        case -2: return 0;
    }

    if (sscanf(recv_buf, "%s %s", code, values) == 2) {
        if (strcmp(code, "NEW") == 0) {
            //NEW id;ip;tpt
            if (sscanf(recv_buf, "NEW %d;%16[^;];%d", &recv_id, recv_ip, &recv_tpt) != 3) {
                return -1;
            }
            if (s->successor_id == NOT_SET) {
                // No successor was found
                change_successor(s, recv_id, recv_ip, recv_tpt);
            } else {
                // Successor was found -> send TOKEN
                if (token(s, 'N', recv_id, recv_ip, recv_tpt) == -1) {
                    return -1;
                }
            }
        } else if (strcmp(code, "TOKEN") == 0) {
            // TOKEN id;type
            // TOKEN id;type;id2;ip;tpt
            tok_read = sscanf(recv_buf, "TOKEN %d;%c;%d;%16[^;];%d", &recv_id, &recv_type, &recv_id2, recv_ip, &recv_tpt);
            if (tok_read != 2 && tok_read != 5) {
                return -1;
            }

            if (recv_type == 'N') {
                // Add server
                if (s->successor_id == recv_id) {
                    // Successor is start server
                    change_successor(s, recv_id2, recv_ip, recv_tpt);
                } else {
                    if (tcp_write_str(s->tcp_client_fd, recv_buf) == -1) {
                        return -1;
                    }
                }
            } else if (recv_type == 'O') {
                // Remove server
                if (s->is_leaving && s->id == recv_id) {
                    // Leaving server
                    if (reset_server_service(s) == -1) {
                        printf("ERROR: Failure while leaving service.\n");
                        return -1;
                    }
                } else {
                    // Forward token to successor
                    if (tcp_write_str(s->tcp_client_fd, recv_buf) == -1) {
                        return -1;
                    }
                    if (s->successor_id == recv_id) {
                        // Predecessor of leaving server and not start server
                        change_successor(s, recv_id2, recv_ip, recv_tpt);
                    }
                }
            } else if (recv_type == 'S') {
                if (s->id == recv_id) {
                    // Received my token back. All servers are unavailable. Send token I
                    s->is_ring_available = false;
                    if (token(s, 'I', recv_id, recv_ip, recv_tpt) == -1) {
                        return -1;
                    }
                } else {
                    if (s->is_available) {
                        // Set as new dispatch server
                        if (set_ds(s, s->service_id) == -1 ||
                            response_cserver_service(s, &recv_id_cs, &recv_id2_cs, recv_ip_cs, &recv_tpt_cs) == -1
                        ) {
                            return -1;
                        }
                        s->is_ds_server = true;

                        // Change token to type T
                        if (token(s, 'T', recv_id, recv_ip, recv_tpt) == -1) {
                            return -1;
                        }
                    } else {
                        // Forward token S to successor
                        if (tcp_write_str(s->tcp_client_fd, recv_buf) == -1) {
                            return -1;
                        }
                    }
                }
            } else if (recv_type == 'T') {
                // Forward token until it reaches the server who launched it
                if (s->id != recv_id) {
                    if (tcp_write_str(s->tcp_client_fd, recv_buf) == -1) {
                        return -1;
                    }
                } else if (s->is_leaving) {
                    // Send TOKEN to successor
                    if (token(s, 'O', s->successor_id, s->successor_ip, s->successor_tpt)) {
                        return -1;
                    }
                }
            } else if (recv_type == 'I') {
                // Forward token until it reaches the server who launched it
                s->is_ring_available = false;
                if (s->id != recv_id) {
                    if (tcp_write_str(s->tcp_client_fd, recv_buf) == -1) {
                        return -1;
                    }
                } else if (s->is_leaving) {
                    // Send TOKEN to successor
                    if (token(s, 'O', s->successor_id, s->successor_ip, s->successor_tpt)) {
                        return -1;
                    }
                }
            } else if (recv_type == 'D') {
                if (!s->is_available || s->id > recv_id) {
                    // Not the one who sent the token -> Forward
                    if (tcp_write_str(s->tcp_client_fd, recv_buf) == -1) {
                        return -1;
                    }
                }

                if (s->id == recv_id) {
                    // New DS
                    // Set as dispatch server
                    if (set_ds(s, s->service_id) == -1 ||
                        response_cserver_service(s, &recv_id, &recv_id2, recv_ip, &recv_tpt) == -1
                    ) {
                        return -1;
                    }
                    s->is_ds_server = true;
                }
                s->is_ring_available = true;
            }
        } else {
            return -1;
        }
    } else {
        if (strcmp(code, "NEW_START") == 0) {
            if (set_start(s, s->service_id) == -1 ||
                response_cserver_service(s, &recv_id, &recv_id2, recv_ip, &recv_tpt) == -1
            ) {
                return -1;
            }
            s->is_start_server = true;
        } else {
            return -1;
        }
    }

    return 0;
}