#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "misc.h"

int set_default_cs_props(char **csip, int *cspt) {
    if (*csip == NULL) {
        // Set default IP
        *csip = calloc(16, sizeof(char));
        if (get_ip_from_hostname("tejo.tecnico.ulisboa.pt" , *csip) == -1) {
            return -1;
        }
    }

    if (*cspt == -1) {
        // Set default Port
        *cspt = 59000;
    }

    return 0;
}

int get_ip_from_hostname(char *hostname , char *ip) {
    struct addrinfo *info, *p;

    if (getaddrinfo(hostname, NULL, NULL, &info) != 0) {
        return -1;
    }

    // Get the first available address
    for (p = info; p != NULL; p = p->ai_next) {
        strcpy(ip, inet_ntoa(((struct sockaddr_in *) info->ai_addr)->sin_addr));
    }

    freeaddrinfo(info);
    return 0;
}

void print_usage_service(void) {
    printf("USAGE: service -n id -j ip -u upt -t tpt [-i csip] [-p cspt].\n");
}

void print_usage_reqserver(void) {
    printf("USAGE: reqserver [-i csip] [-p cspt].\n");
}

void print_command_not_found(char *command) {
    printf("%s : command not found. Type 'help' for help.\n", command);
}

void print_command_help_service(void) {
    printf("HELP - Valid commands:\n");
    printf("join x - join service x ring.\n");
    printf("show_state - show server state (own availability, ring availability, successor id).\n");
    printf("leave - leave ring.\n");
    printf("exit - exit application.\n");
}

void print_command_help_reqserver(void) {
    printf("HELP - Valid commands:\n");
    printf("request_service x || rs x - request service x.\n");
    printf("terminate_service || ts - terminate current service.\n");
    printf("exit - exit application.\n");
}