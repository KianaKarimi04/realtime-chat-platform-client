//
// Created by kiana on 2/5/26.
//

#include <ncurses.h>
#include "ui.h"
#include "auth.h"

int main() {
    ClientState state = {0, ""};

    initscr();
    cbreak();
    noecho();

    show_menu(&state);

    endwin();
    return 0;
}
