//
// Created by kiana on 2/5/26.
//

#ifndef AUTH_H
#define AUTH_H

typedef struct {
    int  logged_in;
    char username[50];
    char password[50];   /* needed for authenticated channel/message requests */
} ClientState;

void create_account(ClientState *state);
void login(ClientState *state);
void logout(ClientState *state);

#endif