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

/* Incoming chat message from server (Message Read ACK) */
typedef struct {
    char     sender[16];    /* username of sender */
    uint8_t  sender_id;     /* user id of sender  */
    uint8_t  channel_id;
    uint64_t timestamp;
    char     text[512];     /* message body */
    uint16_t text_len;
} ChatMessage;

/* Server discovery */
int          discover_server(const char *manager_ip);
const char  *get_server_ip(void);
int          get_server_port(void);
uint8_t      get_server_id(void);

/* Account management */
ServerResponse protocol_create_account(const char *username, const char *password);
ServerResponse protocol_login(const char *username, const char *password);
void           protocol_logout(void);

/* Channel list — Channels Read (RES_CHANNELS / CRUD_READ)
 * Returns channel count, fills channel_ids[] array.
 * Returns -1 on error. */
int protocol_get_channels(const char *username, const char *password,
                          uint8_t *channel_ids, int max_channels);

/* Single channel info — Channel Read (RES_CHANNEL / CRUD_READ)
 * Fills channel_name (16 bytes), user_ids array, returns user count.
 * Returns -1 on error. */
int protocol_get_channel_info(const char *username, const char *password,
                              uint8_t channel_id,
                              char *channel_name,
                              uint8_t *user_ids, int max_users);

/* Send a message — Message Create (RES_MESSAGE / CRUD_CREATE) */
ServerResponse protocol_send_message(const char *username, const char *password,
                                     uint8_t channel_id, const char *text);

/* Read messages — Message Read (RES_MESSAGE / CRUD_READ)
 * Sends a read request; server responds with Message Read ACK.
 * Returns 1 on success (fills msg), 0 on error. */
int protocol_read_messages(const char *username, const char *password,
                           uint8_t channel_id, ChatMessage *msg);

/* User Read — look up username by user id */
int protocol_get_username(const char *username, const char *password,
                          uint8_t user_id, char *out_username);

#endif