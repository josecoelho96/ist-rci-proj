#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "../lib/com_reqserver.h"
#include "../lib/misc.h"
#include "../lib/udp.h"

int parse_args(int argc, char **argv, char **csip, int *cspt);

int main(int argc, char **argv) {
    int cspt;
    char *csip;
    int cs_fd, s_fd;
    struct sockaddr_in cs_addr, s_addr;
    int cmdline_len = 128;
    char *cmdline_buf = calloc(cmdline_len, sizeof(char));
    char opcode[20];
    int service_id;
    bool has_error = false;
    bool has_service_requested = false;

    cspt = -1;
    csip = NULL;

    if (parse_args(argc, argv, &csip, &cspt) == -1) {
        printf("ERROR: Could not parse cli arguments.\n");
        print_usage_reqserver();
        has_error = true;
    }

    if (udp_init(&cs_fd, &cs_addr, csip, cspt) == -1) {
        printf("ERROR: Cannot start UDP client to communicate with central server.\n");
        has_error = true;
    }

    while (!has_error) {
        fgets(cmdline_buf, cmdline_len, stdin);

        if (sscanf(cmdline_buf, "%s %d", opcode, &service_id) == 2) {
            //operation must be request_service (rs)
            if (strcmp(opcode, "request_service") != 0 && strcmp(opcode, "rs") != 0) {
                print_command_not_found(opcode);
            } else {
                if (request_service(cs_fd, &cs_addr, service_id, &s_fd, &s_addr, &has_service_requested) == -1) {
                    has_error = true;
                    break;
                }
            }
        } else {
            // Must be terminate_service or exit
            if (strcmp(opcode, "terminate_service") == 0 || strcmp(opcode, "ts") == 0) {
                if (terminate_service(s_fd, &s_addr, &has_service_requested) == -1) {
                    has_error = true;
                    break;
                }
            } else if (strcmp(opcode, "exit") == 0){
                if (has_service_requested) {
                    if (terminate_service(s_fd, &s_addr, &has_service_requested) == -1) {
                        has_error = true;
                        break;
                    }
                }
                break;
            } else if (strcmp(opcode, "help") == 0) {
                print_command_help_reqserver();
            } else {
                print_command_not_found(opcode);
            }
        }
    }

    if (udp_close(&cs_fd) == -1 || udp_close(&s_fd) == -1){
        return -1;
    }

    return has_error ? -1 : 0;
}

int parse_args(int argc, char **argv, char **csip, int *cspt) {
    int opt = -1;
    while ((opt = getopt(argc, argv, "i:p:")) != -1) {
        switch(opt) {
            case 'i':
                *csip = optarg;
                break;
            case 'p':
                *cspt = atoi(optarg);
                break;
            default:
                return -1;
        }
    }

    return set_default_cs_props(csip, cspt);
}