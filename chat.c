//
// chat.c — Channel list + chat room (single-threaded, no pthreads)
//

#include <ncurses.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#include "chat.h"
#include "protocol.h"

/* ── Message buffer ───────────────────────────────────────────────── */
#define MAX_DISPLAY_MSGS 100

typedef struct {
    ChatMessage msgs[MAX_DISPLAY_MSGS];
    int         count;
} MsgBuffer;

/* ── Username cache ───────────────────────────────────────────────── */
#define MAX_USERS 64
static char user_cache[MAX_USERS][16];
static int  user_cache_valid[MAX_USERS];

static const char *resolve_username(const char *my_user, const char *my_pass,
                                    uint8_t user_id) {
    if (user_id < MAX_USERS && user_cache_valid[user_id])
        return user_cache[user_id];

    char name[16] = {0};
    if (protocol_get_username(my_user, my_pass, user_id, name)) {
        if (user_id < MAX_USERS) {
            strncpy(user_cache[user_id], name, 16);
            user_cache_valid[user_id] = 1;
        }
        return user_cache[user_id];
    }
    return "???";
}

/* ── Add message to buffer ────────────────────────────────────────── */
static void buf_add(MsgBuffer *buf, ChatMessage *msg) {
    if (buf->count < MAX_DISPLAY_MSGS) {
        buf->msgs[buf->count++] = *msg;
    } else {
        memmove(&buf->msgs[0], &buf->msgs[1],
                (MAX_DISPLAY_MSGS - 1) * sizeof(ChatMessage));
        buf->msgs[MAX_DISPLAY_MSGS - 1] = *msg;
    }
}

/* ── Draw messages ────────────────────────────────────────────────── */
static void draw_messages(MsgBuffer *buf, int max_lines, int cols) {
    int start = 0;
    if (buf->count > max_lines)
        start = buf->count - max_lines;

    for (int i = start; i < buf->count; i++) {
        int row = 2 + (i - start);
        mvprintw(row, 0, "[%s]: %.*s",
                 buf->msgs[i].sender,
                 cols - 20,
                 buf->msgs[i].text);
        clrtoeol();
    }
}

/* ── Chat room UI (single-threaded) ──────────────────────────────── */
static void enter_chat(ClientState *state, uint8_t channel_id,
                       const char *channel_name) {
    memset(user_cache_valid, 0, sizeof(user_cache_valid));

    MsgBuffer buf;
    memset(&buf, 0, sizeof(buf));

    char input[512];
    int input_len = 0;
    memset(input, 0, sizeof(input));

    /* Use halfdelay mode: getch() waits up to N tenths of a second */
    halfdelay(1);    /* wait up to 0.1 seconds for input */
    keypad(stdscr, TRUE);

    long last_poll = 0;

    int running = 1;
    while (running) {
        int rows, cols;
        getmaxyx(stdscr, rows, cols);

        /* Poll server for new messages every ~1 second */
        long now = (long)time(NULL);
        if (now != last_poll) {
            last_poll = now;
            ChatMessage msg;
            if (protocol_read_messages(state->username, state->password,
                                       channel_id, &msg)) {
                const char *sender = resolve_username(
                    state->username, state->password, msg.sender_id);
                strncpy(msg.sender, sender, 15);
                msg.sender[15] = '\0';
                buf_add(&buf, &msg);
            }
        }

        /* Draw screen */
        erase();
        mvprintw(0, 0, "=== #%s === (ESC to leave)", channel_name);
        mvhline(1, 0, '-', cols);

        int msg_area = rows - 4;
        if (msg_area < 1) msg_area = 1;
        draw_messages(&buf, msg_area, cols);

        mvhline(rows - 2, 0, '-', cols);
        mvprintw(rows - 1, 0, "> %s", input);
        clrtoeol();
        move(rows - 1, 2 + input_len);
        refresh();

        /* Read input (halfdelay: returns ERR after 0.1s if no key) */
        int ch = getch();
        if (ch == ERR)
            continue;

        if (ch == 27) {   /* ESC */
            running = 0;
        } else if (ch == '\n' || ch == '\r') {
            if (input_len > 0) {
                /* Add our own message to display immediately */
                ChatMessage my_msg;
                memset(&my_msg, 0, sizeof(my_msg));
                strncpy(my_msg.sender, state->username, 15);
                strncpy(my_msg.text, input, 511);
                buf_add(&buf, &my_msg);

                protocol_send_message(state->username, state->password,
                                      channel_id, input);
                memset(input, 0, sizeof(input));
                input_len = 0;
            }
        } else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
            if (input_len > 0)
                input[--input_len] = '\0';
        } else if (input_len < 510 && ch >= 32 && ch <= 126) {
            input[input_len++] = (char)ch;
            input[input_len] = '\0';
        }
    }

    /* Restore normal input mode */
    nocbreak();
    cbreak();
    noecho();
}

/* ── Channel list UI ──────────────────────────────────────────────── */
void show_channel_list(ClientState *state) {
    uint8_t channel_ids[50];
    int count;

    while (1) {
        clear();
        printw("=== Channels ===  ('r' refresh, 'q' back)\n\n");

        count = protocol_get_channels(state->username, state->password,
                                      channel_ids, 50);

        if (count < 0) {
            printw("Failed to fetch channel list.\n");
        } else if (count == 0) {
            printw("No channels available.\n");
        } else {
            for (int i = 0; i < count; i++) {
                char cname[16] = {0};
                uint8_t users[64];
                int nusers = protocol_get_channel_info(
                    state->username, state->password,
                    channel_ids[i], cname, users, 64);

                printw("  %d. #%s  (%d users)\n",
                       i + 1, cname, nusers > 0 ? nusers : 0);
            }
        }

        printw("\nEnter channel number to join: ");
        refresh();

        int ch = getch();

        if (ch == 'q' || ch == 'Q') {
            break;
        } else if (ch == 'r' || ch == 'R') {
            continue;
        } else if (ch >= '1' && ch <= '9') {
            int idx = ch - '1';
            if (idx < count) {
                char cname[16] = {0};
                uint8_t users[64];
                protocol_get_channel_info(state->username, state->password,
                                          channel_ids[idx], cname, users, 64);
                enter_chat(state, channel_ids[idx], cname);
            }
        }
    }
}