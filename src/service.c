#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
#include "../lib/com_service.h"
#include "../lib/misc.h"
#include "../lib/tcp.h"
#include "../lib/udp.h"

int parse_args(int argc, char **argv, server *s);

int main(int argc, char **argv) {
    server s;
    fd_set rfds;
    int max_a, max_b, max_fd;
    int cmdline_len = 128;
    char cmdline_buf[cmdline_len];
    char opcode[20];
    int requested_service;
    int ret;
    bool has_error = false;

    // Another server shutdown. Already handled via tcp_read return = -1
    signal(SIGPIPE, SIG_IGN);

    s = new_server();
    if (parse_args(argc, argv, &s) == -1) {
        print_usage_service();
        printf("ERROR: Could not parse cli arguments.\n");
        has_error = true;
    } else {
        // Start UDP with central server
        // Start UDP server for clients
        // Start TCP server (listening for a new connection)
        if (udp_init(&(s.cs_fd), &(s.cs_addr), s.csip, s.cspt) == -1 ||
            udp_init_server(&(s.reqserver_fd), &(s.reqserver_addr), s.upt) == -1 ||
            tcp_init_server(&(s.tcp_sv_ls_fd), &(s.tcp_sv_ls_addr), s.tpt) == -1
        ) {
            has_error = true;
        }
    }

    while (!has_error) {
        FD_ZERO(&rfds);
        FD_SET(STDIN_FILENO, &rfds);
        FD_SET(s.reqserver_fd, &rfds);
        FD_SET(s.tcp_sv_ls_fd, &rfds);

        if (s.tcp_server_fd != NOT_SET) {
            FD_SET(s.tcp_server_fd, &rfds);
            max_a = STDIN_FILENO > s.reqserver_fd ? STDIN_FILENO : s.reqserver_fd;
            max_b = s.tcp_sv_ls_fd > s.tcp_server_fd ? s.tcp_sv_ls_fd : s.tcp_server_fd;
            max_fd = max_a > max_b ? max_a : max_b;
        } else {
            max_a = STDIN_FILENO > s.reqserver_fd ? STDIN_FILENO : s.reqserver_fd;
            max_fd = max_a > s.tcp_sv_ls_fd ? max_a : s.tcp_sv_ls_fd;
        }

        if (select(max_fd + 1, &rfds, NULL, NULL, (struct timeval *) NULL) <= 0) {
            has_error = true;
            break;
        }

        // Process input from stdin (if any)
        if (FD_ISSET(STDIN_FILENO, &rfds)) {
            fgets(cmdline_buf, cmdline_len, stdin);
            if (sscanf(cmdline_buf, "%s %d", opcode, &requested_service) == 2) {
                // Opcode must be "join"
                if (strcmp(opcode, "join") != 0) {
                    print_command_not_found(opcode);
                } else {
                    if (s.service_id == NOT_SET) {
                        if (join_service(&s, requested_service) == -1) {
                            has_error = true;
                            break;
                        }
                    } else {
                        printf("ERROR: Server has already a service (%d).\n", s.service_id);
                    }
                }
            } else {
                // Opcode must be "show_state", "leave", "exit" or "help"
                if (strcmp(opcode, "show_state") == 0) {
                    show_state(&s);
                } else if (strcmp(opcode, "leave") == 0) {
                    ret = leave_service(&s, false);
                    if (ret == -1) {
                        has_error = true;
                        break;
                    } else if (ret == -2) {
                        printf("ERROR: Can only leave if available.\n");
                    }
                } else if (strcmp(opcode, "exit") == 0) {
                    ret = leave_service(&s, true);
                    if (ret == -1) {
                        has_error = true;
                        break;
                    } else if (ret == -2) {
                        printf("ERROR: Can only leave if available.\n");
                    } else {
                        break;
                    }
                } else if (strcmp(opcode, "help") == 0) {
                    print_command_help_service();
                } else {
                    print_command_not_found(opcode);
                }
            }
        }

        // Process input from a client (reqserver)
        if (FD_ISSET(s.reqserver_fd, &rfds)) {
            if (request_service_client(&s) == -1) {
                has_error = true;
                break;
            }
        }

        // Process input from other server (service)
        if (FD_ISSET(s.tcp_sv_ls_fd, &rfds)) {
            // Create new socket, which must be included on fd_set. Close old fd
            if (s.tcp_server_fd != NOT_SET) {
                if (tcp_close(&(s.tcp_server_fd)) == -1) {
                    has_error = true;
                    break;
                }
            }
            if ((s.tcp_server_fd = tcp_accept(s.tcp_sv_ls_fd, &(s.tcp_sv_ls_addr))) == -1) {
                has_error = true;
                break;
            }
            continue;
        }

        if (s.tcp_server_fd != NOT_SET && FD_ISSET(s.tcp_server_fd, &rfds)) {
            if (response_service_service(&s) == -1) {
                has_error = true;
                break;
            }
        }
    }

    if (udp_close(&(s.cs_fd)) == -1 ||
        udp_close(&(s.reqserver_fd)) == -1 ||
        tcp_close(&(s.tcp_sv_ls_fd)) == -1 ||
        tcp_close(&(s.tcp_server_fd)) == -1 ||
        tcp_close(&(s.tcp_client_fd))
    ){
        return -1;
    }

    return has_error ? -1 : 0;
}

int parse_args(int argc, char **argv, server *s) {
    int opt = -1;
    while ((opt = getopt(argc, argv, "n:j:u:t:i:p:")) != -1) {
        switch(opt) {
            case 'n':
                s->id = atoi(optarg);
                break;
            case 'j':
                s->ip = optarg;
                break;
            case 'u':
                s->upt = atoi(optarg);
                break;
            case 't':
                s->tpt = atoi(optarg);
                break;
            case 'i':
                s->csip = optarg;
                break;
            case 'p':
                s->cspt = atoi(optarg);
                break;
            default:
                return -1;
        }
    }

    if (s->id == -1 || s->ip == NULL || s->upt == -1 || s->tpt == -1) {
        return -1;
    }

    return set_default_cs_props(&(s->csip), &(s->cspt));
}