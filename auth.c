//
// Created by kiana on 2/5/26.
//

#include "auth.h"
#include "protocol.h"
#include <ncurses.h>
#include <string.h>

void create_account(ClientState *state) {
    char username[50];
    char password[50];

    clear();
    printw("=== Create Account ===\n");
    printw("Username: ");
    echo();
    getnstr(username, 49);
    printw("Password: ");
    getnstr(password, 49);
    noecho();

    ServerResponse res = protocol_create_account(username, password);

    if (res == SERVER_ACK)
        printw("\nServer: Account created successfully.\n");
    else
        printw("\nServer: Account creation failed.\n");

    printw("\nPress any key to continue...");
    getch();
}

void login(ClientState *state) {
    char username[50];
    char password[50];

    clear();
    printw("=== Login ===\n");
    printw("Username: ");
    echo();
    getnstr(username, 49);
    printw("Password: ");
    getnstr(password, 49);
    noecho();

    ServerResponse res = protocol_login(username, password);

    if (res == SERVER_ACK) {
        state->logged_in = 1;
        strcpy(state->username, username);
        printw("\nServer: Login successful.\n");
    } else {
        printw("\nServer: Login failed.\n");
    }

    printw("\nPress any key to continue...");
    getch();
}

void logout(ClientState *state) {
    protocol_logout();
    state->logged_in = 0;
    state->username[0] = '\0';
}
