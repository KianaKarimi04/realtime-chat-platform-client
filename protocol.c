#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>

#include "protocol.h"
#include "network.h"
#include "protocol_internal.h"

/* Discovered server info */
static char server_ip[64];
static uint8_t server_id;
static uint16_t server_port = 42069;   // agreed server port

/* Client -> Server Manager : Get Active Server */
int discover_server(const char *manager_ip) {
    int sock = connect_to(manager_ip, 42069);
    if (sock < 0) {
        printw("pass")
        return 0;
    }
    BigHeader hdr;
    memset(&hdr, 0, sizeof(hdr));

    hdr.version = BIG_VERSION_1;
    hdr.type    = MSG_TYPE(RES_ACTIVATION, CRUD_READ, ACK_REQ);
    hdr.status  = 0x00;
    hdr.padding = 0x00;
    hdr.length  = htonl(5);   // 4 bytes IP + 1 byte server ID

    if (send(sock, &hdr, sizeof(hdr), 0) != sizeof(hdr)) {
        close_conn(sock);
        return 0;
    }

    uint8_t body[5] = {0};
    send(sock, body, sizeof(body), 0);

    BigHeader resp;
    if (recv(sock, &resp, sizeof(resp), MSG_WAITALL) != sizeof(resp)) {
        close_conn(sock);
        return 0;
    }

    uint32_t len = ntohl(resp.length);
    if (len != 5) {
        close_conn(sock);
        return 0;
    }

    uint8_t resp_body[5];
    if (recv(sock, resp_body, len, MSG_WAITALL) != (int)len) {
        close_conn(sock);
        return 0;
    }

    close_conn(sock);

    struct in_addr ip;
    memcpy(&ip, resp_body, 4);
    server_id = resp_body[4];

    strncpy(server_ip, inet_ntoa(ip), sizeof(server_ip) - 1);
    server_ip[sizeof(server_ip) - 1] = '\0';

    return 1;
}

const char *get_server_ip(void) {
    return server_ip;
}

int get_server_port(void) {
    return server_port;
}

uint8_t get_server_id(void) {
    return server_id;
}

/* Helper: send account message (create / login / logout) */
static ServerResponse send_account_msg(
        uint8_t crud,
        const char *username,
        const char *password,
        uint8_t status_flag) {

    int sock = connect_to(server_ip, server_port);
    if (sock < 0)
        return SERVER_NACK;

    BigHeader hdr;
    memset(&hdr, 0, sizeof(hdr));

    hdr.version = BIG_VERSION_1;
    hdr.type    = MSG_TYPE(RES_ACCOUNT, crud, ACK_REQ);
    hdr.status  = 0x00;
    hdr.padding = 0x00;
    hdr.length  = htonl(34);

    send(sock, &hdr, sizeof(hdr), 0);

    uint8_t body[34];
    memset(body, 0, sizeof(body));

    strncpy((char *)body, username, 16);
    strncpy((char *)body + 16, password, 16);
    body[32] = server_id;     // client/server ID
    body[33] = status_flag;   // 1 = online, 0 = offline

    send(sock, body, sizeof(body), 0);

    BigHeader resp;
    ssize_t n = recv(sock, &resp, sizeof(resp), MSG_WAITALL);

    if (n == 0) {
        fprintf(stderr, "Server closed connection (no response)");
        close(sock);
        return -1;
    }

    if (n < 0) {
        perror("recv failed");
        close(sock);
        return -1;
    }

    if (n != sizeof(resp)) {
        fprintf(stderr, "Incomplete response: got %zd bytes, expected %zu\n",
                n, sizeof(resp));
        close(sock);
        return -1;
    }

    close_conn(sock);

    return (resp.type & 0x01) ? SERVER_ACK : SERVER_NACK;
}

/* Client -> Server : Create Account */
ServerResponse protocol_create_account(const char *username,
                                       const char *password) {
    return send_account_msg(CRUD_CREATE, username, password, 0x01);
}

/* Client -> Server : Login */
ServerResponse protocol_login(const char *username,
                              const char *password) {
    return send_account_msg(CRUD_UPDATE, username, password, 0x01);
}

/* Client -> Server : Logout */
void protocol_logout(void) {
    send_account_msg(CRUD_UPDATE, "", "", 0x00);
}
