#ifndef MISC_H
#define MISC_H

#include "com_service.h"

int set_default_cs_props(char **csip, int *cspt);
int get_ip_from_hostname(char *hostname , char *ip);
void print_usage_service(void);
void print_usage_reqserver(void);
void print_command_not_found(char *command);
void print_command_help_service(void);
void print_command_help_reqserver(void);

#endif