//
// Created by kiana on 2/5/26.
//

#ifndef PROTOCOL_H
#define PROTOCOL_H

typedef enum {
    SERVER_ACK,
    SERVER_NACK
} ServerResponse;

ServerResponse protocol_create_account(const char *username, const char *password);
ServerResponse protocol_login(const char *username, const char *password);
void protocol_logout(void);

#endif
