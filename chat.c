//
// chat.c — Channel list + chat room (poll-based message receive)
//

#include <ncurses.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include "chat.h"
#include "protocol.h"

/* ── Message buffer ───────────────────────────────────────────────── */
#define MAX_DISPLAY_MSGS 100

typedef struct {
    ChatMessage      msgs[MAX_DISPLAY_MSGS];
    int              count;
    pthread_mutex_t  lock;
    volatile int     running;

    /* Credentials for polling */
    char username[16];
    char password[16];
    uint8_t channel_id;
} ChatRoom;

/* ── Username cache (so we don't User-Read every message) ─────────── */
#define MAX_USERS 64
static char user_cache[MAX_USERS][16];
static int  user_cache_valid[MAX_USERS];

static const char *resolve_username(const char *my_user, const char *my_pass,
                                    uint8_t user_id) {
    if (user_id < MAX_USERS && user_cache_valid[user_id]) {
        return user_cache[user_id];
    }

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

/* ── Background polling thread ────────────────────────────────────── */
static void *poll_thread(void *arg) {
    ChatRoom *room = (ChatRoom *)arg;

    while (room->running) {
        ChatMessage msg;

        if (protocol_read_messages(room->username, room->password,
                                   room->channel_id, &msg)) {
            /* Resolve sender username from user id */
            const char *sender = resolve_username(
                room->username, room->password, msg.sender_id);
            strncpy(msg.sender, sender, 15);
            msg.sender[15] = '\0';

            pthread_mutex_lock(&room->lock);
            if (room->count < MAX_DISPLAY_MSGS) {
                room->msgs[room->count++] = msg;
            } else {
                memmove(&room->msgs[0], &room->msgs[1],
                        (MAX_DISPLAY_MSGS - 1) * sizeof(ChatMessage));
                room->msgs[MAX_DISPLAY_MSGS - 1] = msg;
            }
            pthread_mutex_unlock(&room->lock);
        }

        /* Poll interval — adjust as needed */
        usleep(500000);   /* 500 ms */
    }
    return NULL;
}

/* ── Draw messages ────────────────────────────────────────────────── */
static void draw_messages(ChatRoom *room, int max_lines, int cols) {
    pthread_mutex_lock(&room->lock);

    int start = 0;
    if (room->count > max_lines)
        start = room->count - max_lines;

    for (int i = start; i < room->count; i++) {
        int row = 2 + (i - start);
        mvprintw(row, 0, "[%s]: %.*s",
                 room->msgs[i].sender,
                 cols - 20,   /* truncate to screen width */
                 room->msgs[i].text);
        clrtoeol();
    }

    pthread_mutex_unlock(&room->lock);
}

/* ── Chat room UI ─────────────────────────────────────────────────── */
static void enter_chat(ClientState *state, uint8_t channel_id,
                       const char *channel_name) {
    /* Clear username cache for this session */
    memset(user_cache_valid, 0, sizeof(user_cache_valid));

    ChatRoom room;
    memset(&room, 0, sizeof(room));
    pthread_mutex_init(&room.lock, NULL);
    room.running = 1;
    room.channel_id = channel_id;
    strncpy(room.username, state->username, 15);
    strncpy(room.password, state->password, 15);

    /* Start polling thread */
    pthread_t tid;
    pthread_create(&tid, NULL, poll_thread, &room);

    /* ncurses: non-blocking input */
    nodelay(stdscr, TRUE);
    keypad(stdscr, TRUE);

    char input[512];
    int input_len = 0;
    memset(input, 0, sizeof(input));

    int running = 1;
    while (running) {
        int rows, cols;
        getmaxyx(stdscr, rows, cols);

        clear();
        mvprintw(0, 0, "=== #%s === (ESC to leave)", channel_name);
        mvhline(1, 0, '-', cols);

        int msg_area = rows - 4;
        if (msg_area < 1) msg_area = 1;
        draw_messages(&room, msg_area, cols);

        mvhline(rows - 2, 0, '-', cols);
        mvprintw(rows - 1, 0, "> %s", input);
        clrtoeol();
        move(rows - 1, 2 + input_len);
        refresh();

        int ch = getch();
        if (ch == ERR) {
            usleep(50000);   /* 50 ms — avoid busy-wait */
            continue;
        }

        if (ch == 27) {   /* ESC */
            running = 0;
        } else if (ch == '\n' || ch == '\r') {
            if (input_len > 0) {
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

    /* Cleanup */
    room.running = 0;
    pthread_join(tid, NULL);
    pthread_mutex_destroy(&room.lock);

    nodelay(stdscr, FALSE);
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
                /* Get channel name via Channel Read */
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
                /* Get the channel name for display */
                char cname[16] = {0};
                uint8_t users[64];
                protocol_get_channel_info(state->username, state->password,
                                          channel_ids[idx], cname, users, 64);
                enter_chat(state, channel_ids[idx], cname);
            }
        }
    }
}