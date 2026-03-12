#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>

#include "protocol.h"
#include "network.h"
#include "protocol_internal.h"

/* ── Discovered server info ───────────────────────────────────────── */
static char     server_ip[64];
static uint8_t  server_id;
static uint16_t server_port = 42069;

/* ── Helpers ──────────────────────────────────────────────────────── */

static int send_all(int sock, const void *buf, size_t len) {
    const uint8_t *p = (const uint8_t *)buf;
    size_t sent = 0;
    while (sent < len) {
        ssize_t n = send(sock, p + sent, len - sent, 0);
        if (n < 0) { if (errno == EINTR) continue; return 0; }
        if (n == 0) return 0;
        sent += (size_t)n;
    }
    return 1;
}

static int recv_all(int sock, void *buf, size_t len) {
    ssize_t n = recv(sock, buf, len, MSG_WAITALL);
    return (n == (ssize_t)len);
}

/* Build and zero-fill a header */
static BigHeader make_header(uint8_t resource, uint8_t crud, uint32_t body_len) {
    BigHeader hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.version = BIG_VERSION;
    hdr.type    = MSG_TYPE(resource, crud, ACK_REQ);
    hdr.status  = 0x00;
    hdr.padding = 0x00;
    hdr.length  = htonl(body_len);
    return hdr;
}

/* Get our local IP as 4 bytes (for login/logout bodies) */
static void get_local_ip_bytes(int sock, uint8_t out[4]) {
    struct sockaddr_in local;
    socklen_t len = sizeof(local);
    memset(out, 0, 4);
    if (getsockname(sock, (struct sockaddr *)&local, &len) == 0) {
        memcpy(out, &local.sin_addr, 4);
    }
}

/* ═══════════════════════════════════════════════════════════════════
 *  SERVER DISCOVERY  (Get Active Server)
 *  RES_ACTIVATION / CRUD_READ
 *  Body out: 4 IP (zeros) + 1 server ID (zero) = 5 bytes
 *  Body in:  4 IP + 1 server ID = 5 bytes
 * ═══════════════════════════════════════════════════════════════════ */

int discover_server(const char *manager_ip) {
    int sock = connect_to(manager_ip, 42069);
    if (sock < 0) return 0;

    BigHeader hdr = make_header(RES_ACTIVATION, CRUD_READ, 5);
    uint8_t body[5] = {0};

    if (!send_all(sock, &hdr, sizeof(hdr)) ||
        !send_all(sock, body, sizeof(body))) {
        close_conn(sock); return 0;
    }

    BigHeader resp;
    if (!recv_all(sock, &resp, sizeof(resp))) { close_conn(sock); return 0; }

    uint32_t len = ntohl(resp.length);
    if (len != 5) { close_conn(sock); return 0; }

    uint8_t resp_body[5];
    if (!recv_all(sock, resp_body, 5)) { close_conn(sock); return 0; }

    close_conn(sock);

    struct in_addr ip;
    memcpy(&ip, resp_body, 4);
    server_id = resp_body[4];
    strncpy(server_ip, inet_ntoa(ip), sizeof(server_ip) - 1);
    server_ip[sizeof(server_ip) - 1] = '\0';
    return 1;
}

const char *get_server_ip(void)   { return server_ip;   }
int         get_server_port(void) { return server_port;  }
uint8_t     get_server_id(void)   { return server_id;    }

/* ═══════════════════════════════════════════════════════════════════
 *  CREATE ACCOUNT
 *  RES_ACCOUNT / CRUD_CREATE
 *  Body: 16 username + 16 password + 1 client_id = 33 bytes
 * ═══════════════════════════════════════════════════════════════════ */

ServerResponse protocol_create_account(const char *username,
                                       const char *password) {
    int sock = connect_to(server_ip, server_port);
    if (sock < 0) return SERVER_NACK;

    BigHeader hdr = make_header(RES_ACCOUNT, CRUD_CREATE, 33);

    uint8_t body[33];
    memset(body, 0, sizeof(body));
    strncpy((char *)body,      username ? username : "", 16);
    strncpy((char *)body + 16, password ? password : "", 16);
    body[32] = 0;   /* client id — server assigns it */

    if (!send_all(sock, &hdr, sizeof(hdr)) ||
        !send_all(sock, body, sizeof(body))) {
        close_conn(sock); return SERVER_NACK;
    }

    BigHeader resp;
    if (!recv_all(sock, &resp, sizeof(resp))) {
        close_conn(sock); return SERVER_NACK;
    }

    /* Drain response body if any */
    uint32_t rlen = ntohl(resp.length);
    if (rlen > 0 && rlen < 4096) {
        uint8_t discard[4096];
        recv_all(sock, discard, rlen);
    }

    close_conn(sock);
    return (resp.status == STATUS_OK) ? SERVER_ACK : SERVER_NACK;
}

/* ═══════════════════════════════════════════════════════════════════
 *  LOGIN
 *  RES_ACCOUNT / CRUD_UPDATE
 *  Body: 16 username + 16 password + 4 client IP + 1 status(0x01) = 37
 * ═══════════════════════════════════════════════════════════════════ */

ServerResponse protocol_login(const char *username, const char *password) {
    int sock = connect_to(server_ip, server_port);
    if (sock < 0) return SERVER_NACK;

    BigHeader hdr = make_header(RES_ACCOUNT, CRUD_UPDATE, 37);

    uint8_t body[37];
    memset(body, 0, sizeof(body));
    strncpy((char *)body,      username ? username : "", 16);
    strncpy((char *)body + 16, password ? password : "", 16);
    get_local_ip_bytes(sock, body + 32);   /* 4-byte client IP */
    body[36] = 0x01;                       /* status = online  */

    if (!send_all(sock, &hdr, sizeof(hdr)) ||
        !send_all(sock, body, sizeof(body))) {
        close_conn(sock); return SERVER_NACK;
    }

    BigHeader resp;
    if (!recv_all(sock, &resp, sizeof(resp))) {
        close_conn(sock); return SERVER_NACK;
    }

    uint32_t rlen = ntohl(resp.length);
    if (rlen > 0 && rlen < 4096) {
        uint8_t discard[4096];
        recv_all(sock, discard, rlen);
    }

    close_conn(sock);
    return (resp.status == STATUS_OK) ? SERVER_ACK : SERVER_NACK;
}

/* ═══════════════════════════════════════════════════════════════════
 *  LOGOUT
 *  RES_ACCOUNT / CRUD_UPDATE
 *  Body: 16 username + 16 password + 4 client IP + 1 status(0x00) = 37
 * ═══════════════════════════════════════════════════════════════════ */

void protocol_logout(void) {
    int sock = connect_to(server_ip, server_port);
    if (sock < 0) return;

    BigHeader hdr = make_header(RES_ACCOUNT, CRUD_UPDATE, 37);

    uint8_t body[37];
    memset(body, 0, sizeof(body));
    /* username/password empty, IP zeros, status=offline */
    body[36] = 0x00;

    send_all(sock, &hdr, sizeof(hdr));
    send_all(sock, body, sizeof(body));

    BigHeader resp;
    recv_all(sock, &resp, sizeof(resp));

    uint32_t rlen = ntohl(resp.length);
    if (rlen > 0 && rlen < 4096) {
        uint8_t discard[4096];
        recv_all(sock, discard, rlen);
    }

    close_conn(sock);
}

/* ═══════════════════════════════════════════════════════════════════
 *  CHANNELS READ  (get list of all channels)
 *  RES_CHANNELS (0x05) / CRUD_READ
 *
 *  Request body:
 *    16 username + 16 password + 1 channel_list_length(0) + 1 channel_list(0)
 *    = 34 bytes
 *
 *  Response body (ACK):
 *    16 username + 16 password + 1 channel_list_length + N channel IDs (1 byte each)
 * ═══════════════════════════════════════════════════════════════════ */

int protocol_get_channels(const char *username, const char *password,
                          uint8_t *channel_ids, int max_channels) {
    int sock = connect_to(server_ip, server_port);
    if (sock < 0) return -1;

    BigHeader hdr = make_header(RES_CHANNELS, CRUD_READ, 34);

    uint8_t body[34];
    memset(body, 0, sizeof(body));
    strncpy((char *)body,      username ? username : "", 16);
    strncpy((char *)body + 16, password ? password : "", 16);
    body[32] = 0;   /* channel list length = 0 (requesting) */
    body[33] = 0;   /* channel list placeholder */

    if (!send_all(sock, &hdr, sizeof(hdr)) ||
        !send_all(sock, body, sizeof(body))) {
        close_conn(sock); return -1;
    }

    /* Receive ACK header */
    BigHeader resp;
    if (!recv_all(sock, &resp, sizeof(resp))) {
        close_conn(sock); return -1;
    }

    if (resp.status != STATUS_OK) {
        close_conn(sock); return -1;
    }

    uint32_t rlen = ntohl(resp.length);
    if (rlen < 33) { close_conn(sock); return -1; }

    /* Read full response body */
    uint8_t *rbuf = (uint8_t *)malloc(rlen);
    if (!rbuf) { close_conn(sock); return -1; }

    if (!recv_all(sock, rbuf, rlen)) {
        free(rbuf); close_conn(sock); return -1;
    }
    close_conn(sock);

    /* Parse: skip 32 bytes (username+password), then 1 byte count, then N IDs */
    uint8_t count = rbuf[32];
    int n = (count < max_channels) ? count : max_channels;

    for (int i = 0; i < n; i++) {
        channel_ids[i] = rbuf[33 + i];
    }

    free(rbuf);
    return n;
}

/* ═══════════════════════════════════════════════════════════════════
 *  CHANNEL READ  (get single channel info)
 *  RES_CHANNEL (0x04) / CRUD_READ
 *
 *  Request body:
 *    16 username + 16 password + 16 channel_name + 1 channel_id
 *    + 1 user_id_array_length + 1 user_id_array
 *    = 51 bytes
 *
 *  Response body (ACK):
 *    16 username + 16 password + 16 channel_name + 1 channel_id
 *    + 1 user_id_array_length + N user_ids
 * ═══════════════════════════════════════════════════════════════════ */

int protocol_get_channel_info(const char *username, const char *password,
                              uint8_t channel_id,
                              char *channel_name,
                              uint8_t *user_ids, int max_users) {
    int sock = connect_to(server_ip, server_port);
    if (sock < 0) return -1;

    BigHeader hdr = make_header(RES_CHANNEL, CRUD_READ, 51);

    uint8_t body[51];
    memset(body, 0, sizeof(body));
    strncpy((char *)body,      username ? username : "", 16);
    strncpy((char *)body + 16, password ? password : "", 16);
    /* channel_name[16] at offset 32 — zeros (server fills it) */
    body[48] = channel_id;
    body[49] = 0;   /* user_id_array_length */
    body[50] = 0;   /* user_id_array placeholder */

    if (!send_all(sock, &hdr, sizeof(hdr)) ||
        !send_all(sock, body, sizeof(body))) {
        close_conn(sock); return -1;
    }

    BigHeader resp;
    if (!recv_all(sock, &resp, sizeof(resp))) {
        close_conn(sock); return -1;
    }

    if (resp.status != STATUS_OK) {
        close_conn(sock); return -1;
    }

    uint32_t rlen = ntohl(resp.length);
    if (rlen < 50) { close_conn(sock); return -1; }

    uint8_t *rbuf = (uint8_t *)malloc(rlen);
    if (!rbuf) { close_conn(sock); return -1; }

    if (!recv_all(sock, rbuf, rlen)) {
        free(rbuf); close_conn(sock); return -1;
    }
    close_conn(sock);

    /* Parse response */
    if (channel_name) {
        memcpy(channel_name, rbuf + 32, 16);
        channel_name[15] = '\0';
    }

    uint8_t user_count = rbuf[49];
    int n = (user_count < max_users) ? user_count : max_users;

    for (int i = 0; i < n; i++) {
        user_ids[i] = rbuf[50 + i];
    }

    free(rbuf);
    return n;
}

/* ═══════════════════════════════════════════════════════════════════
 *  MESSAGE CREATE  (send a message)
 *  RES_MESSAGE (0x06) / CRUD_CREATE
 *
 *  Body:
 *    16 username + 16 password + 8 timestamp + 2 message_length
 *    + 1 channel_id + N message bytes
 * ═══════════════════════════════════════════════════════════════════ */

ServerResponse protocol_send_message(const char *username, const char *password,
                                     uint8_t channel_id, const char *text) {
    int sock = connect_to(server_ip, server_port);
    if (sock < 0) return SERVER_NACK;

    uint16_t msg_len = (uint16_t)strlen(text);
    if (msg_len > 512) msg_len = 512;

    /* Fixed part: 16+16+8+2+1 = 43 bytes, then msg_len variable bytes */
    uint32_t body_len = 43 + msg_len;

    BigHeader hdr = make_header(RES_MESSAGE, CRUD_CREATE, body_len);

    uint8_t *body = (uint8_t *)calloc(1, body_len);
    if (!body) { close_conn(sock); return SERVER_NACK; }

    strncpy((char *)body,      username ? username : "", 16);
    strncpy((char *)body + 16, password ? password : "", 16);

    /* 8-byte timestamp (big-endian) */
    uint64_t ts = (uint64_t)time(NULL);
    for (int i = 0; i < 8; i++) {
        body[32 + i] = (uint8_t)((ts >> (56 - i * 8)) & 0xFF);
    }

    /* 2-byte message length (big-endian) */
    body[40] = (uint8_t)(msg_len >> 8);
    body[41] = (uint8_t)(msg_len & 0xFF);

    body[42] = channel_id;

    memcpy(body + 43, text, msg_len);

    if (!send_all(sock, &hdr, sizeof(hdr)) ||
        !send_all(sock, body, body_len)) {
        free(body); close_conn(sock); return SERVER_NACK;
    }
    free(body);

    close_conn(sock);
    return SERVER_ACK;
}

/* ═══════════════════════════════════════════════════════════════════
 *  MESSAGE READ  (poll for messages)
 *  RES_MESSAGE (0x06) / CRUD_READ
 *
 *  Request body:
 *    16 username + 16 password + 8 timestamp + 2 msg_length(0)
 *    + 1 channel_id + 1 user_id_of_sender(0) + 1 message(0)
 *    = 45 bytes
 *
 *  Response body (ACK):
 *    16 username + 16 password + 8 timestamp + 2 msg_length
 *    + 1 channel_id + 1 user_id_of_sender + N message bytes
 * ═══════════════════════════════════════════════════════════════════ */

int protocol_read_messages(const char *username, const char *password,
                           uint8_t channel_id, ChatMessage *msg) {
    int sock = connect_to(server_ip, server_port);
    if (sock < 0) return 0;

    BigHeader hdr = make_header(RES_MESSAGE, CRUD_READ, 45);

    uint8_t body[45];
    memset(body, 0, sizeof(body));
    strncpy((char *)body,      username ? username : "", 16);
    strncpy((char *)body + 16, password ? password : "", 16);
    /* timestamp zeros = give me latest? or server determines */
    /* msg_length = 0 */
    body[42] = channel_id;
    body[43] = 0;   /* user_id_of_sender = 0 (any) */
    body[44] = 0;   /* message placeholder */

    if (!send_all(sock, &hdr, sizeof(hdr)) ||
        !send_all(sock, body, sizeof(body))) {
        close_conn(sock); return 0;
    }

    BigHeader resp;
    if (!recv_all(sock, &resp, sizeof(resp))) {
        close_conn(sock); return 0;
    }

    if (resp.status != STATUS_OK) {
        close_conn(sock); return 0;
    }

    uint32_t rlen = ntohl(resp.length);
    if (rlen < 44) { close_conn(sock); return 0; }

    uint8_t *rbuf = (uint8_t *)malloc(rlen);
    if (!rbuf) { close_conn(sock); return 0; }

    if (!recv_all(sock, rbuf, rlen)) {
        free(rbuf); close_conn(sock); return 0;
    }
    close_conn(sock);

    /* Parse */
    memset(msg, 0, sizeof(*msg));

    /* Timestamp at offset 32, 8 bytes big-endian */
    msg->timestamp = 0;
    for (int i = 0; i < 8; i++) {
        msg->timestamp = (msg->timestamp << 8) | rbuf[32 + i];
    }

    /* Message length at offset 40, 2 bytes big-endian */
    msg->text_len = (uint16_t)((rbuf[40] << 8) | rbuf[41]);

    /* If message length is 0, server has no new messages */
    if (msg->text_len == 0) {
        free(rbuf);
        close_conn(sock);
        return 0;
    }

    msg->channel_id = rbuf[42];
    msg->sender_id  = rbuf[43];

    /* Message text starts at offset 44 */
    uint16_t copy_len = msg->text_len;
    if (copy_len > 511) copy_len = 511;
    if (44 + copy_len <= rlen) {
        memcpy(msg->text, rbuf + 44, copy_len);
    }
    msg->text[copy_len] = '\0';

    /* sender username is not in Message Read ACK — need User Read to resolve */
    msg->sender[0] = '\0';

    free(rbuf);
    return 1;
}

/* ═══════════════════════════════════════════════════════════════════
 *  USER READ  (look up username by user id)
 *  RES_ACCOUNT (0x02) / CRUD_READ
 *
 *  Request body:
 *    16 username + 16 password + 16 username_for_user_id(zeros) + 1 user_id
 *    = 49 bytes
 *
 *  Response body (ACK):
 *    16 username + 16 password + 16 username_for_user_id + 1 user_id
 * ═══════════════════════════════════════════════════════════════════ */

int protocol_get_username(const char *username, const char *password,
                          uint8_t user_id, char *out_username) {
    int sock = connect_to(server_ip, server_port);
    if (sock < 0) return 0;

    BigHeader hdr = make_header(RES_ACCOUNT, CRUD_READ, 49);

    uint8_t body[49];
    memset(body, 0, sizeof(body));
    strncpy((char *)body,      username ? username : "", 16);
    strncpy((char *)body + 16, password ? password : "", 16);
    /* username_for_user_id at offset 32 = zeros (server fills it) */
    body[48] = user_id;

    if (!send_all(sock, &hdr, sizeof(hdr)) ||
        !send_all(sock, body, sizeof(body))) {
        close_conn(sock); return 0;
    }

    BigHeader resp;
    if (!recv_all(sock, &resp, sizeof(resp))) {
        close_conn(sock); return 0;
    }

    if (resp.status != STATUS_OK) {
        close_conn(sock); return 0;
    }

    uint32_t rlen = ntohl(resp.length);
    if (rlen < 49) { close_conn(sock); return 0; }

    uint8_t rbuf[49];
    if (!recv_all(sock, rbuf, 49)) {
        close_conn(sock); return 0;
    }
    close_conn(sock);

    memcpy(out_username, rbuf + 32, 16);
    out_username[15] = '\0';
    return 1;
}