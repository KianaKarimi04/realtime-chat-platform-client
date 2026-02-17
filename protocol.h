//
// Created by kiana on 2/5/26.
//

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
    SERVER_ACK,
    SERVER_NACK
} ServerResponse;

int discover_server(const char *manager_ip);
const char *get_server_ip(void);
int get_server_port(void);

uint8_t get_server_id(void);

ServerResponse protocol_create_account(const char *username, const char *password);
ServerResponse protocol_login(const char *username, const char *password);
void protocol_logout(void);

#endif
