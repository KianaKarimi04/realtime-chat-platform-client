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

    if (!discover_server()) {
        clear();
        printw("Failed to contact server manager\n");
        printw("Press any key to exit...");
        getch();
        endwin();
        return 1;
    }

    show_menu(&state);

    endwin();
    return 0;
}

