//
// Created by kiana on 2/5/26.
//

//
// Created by kiana on 2/5/26.
//

#include <ncurses.h>
#include "ui.h"
#include "auth.h"
#include "protocol.h"

int main() {
    ClientState state = {0, ""};

    initscr();
    cbreak();
    noecho();

    clear();
    printw("Connecting to server manager...\n");
    refresh();

    if (!discover_server()) {
        printw("Failed to contact server manager\n");
        printw("Press any key to exit...");
        getch();
        endwin();
        return 1;
    }

    printw("Connected to server manager\n");
    printw("Active server IP: %s\n", get_server_ip());
    printw("Active server port: %d\n", get_server_port());
    printw("\nPress any key to continue...");
    getch();

    show_menu(&state);

    endwin();
    return 0;
}