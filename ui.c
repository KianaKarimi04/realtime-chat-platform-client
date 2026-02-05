//
// Created by kiana on 2/5/26.
//

#include <ncurses.h>
#include "ui.h"
#include "auth.h"

void show_menu(ClientState *state) {
    int ch;

    while (1) {
        clear();

        if (!state->logged_in) {
            printw("=== Client Menu ===\n");
            printw("1. Create Account\n");
            printw("2. Login\n");
            printw("3. Exit\n");
            ch = getch();

            if (ch == '1') create_account(state);
            else if (ch == '2') login(state);
            else if (ch == '3') break;
        } else {
            printw("Welcome, %s\n", state->username);
            printw("1. Logout\n");
            printw("2. Exit\n");
            ch = getch();

            if (ch == '1') logout(state);
            else if (ch == '2') break;
        }
    }
}
