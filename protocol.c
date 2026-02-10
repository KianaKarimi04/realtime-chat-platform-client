#include "protocol.h"
#include "network.h"
#include <stdio.h>
#include <string.h>

/* Discovered server info */
static char server_ip[64] = "";
static int server_port = 0;


/* Server discovery (Client -> Server Manager)*/

int discover_server(void) {
    int sock = connect_to("192.168.0.19", 42069);
    if (sock < 0)
        return 0;

    /* Ask server manager for active server */
    send_line(sock, "GET_SERVER\n");

    char reply[128];
    int n = recv_line(sock, reply, sizeof(reply));
    close_conn(sock);

    if (n <= 0)
        return 0;

    /* Expected format: SERVER <ip> <port> */
    if (sscanf(reply, "SERVER %63s %d", server_ip, &server_port) == 2)
        return 1;

    return 0;
}

const char *get_server_ip(void) {
    return server_ip;
}

int get_server_port(void) {
    return server_port;
}

/* Authentication with Server */

ServerResponse protocol_create_account(const char *username, const char *password) {
    int sock = connect_to(server_ip, server_port);
    if (sock < 0)
        return SERVER_NACK;

    char msg[128];
    snprintf(msg, sizeof(msg), "CREATE %s %s\n", username, password);
    send_line(sock, msg);

    char reply[64];
    recv_line(sock, reply, sizeof(reply));
    close_conn(sock);

    return (strncmp(reply, "OK", 2) == 0)
           ? SERVER_ACK
           : SERVER_NACK;
}

ServerResponse protocol_login(const char *username, const char *password) {
    int sock = connect_to(server_ip, server_port);
    if (sock < 0)
        return SERVER_NACK;

    char msg[128];
    snprintf(msg, sizeof(msg), "LOGIN %s %s\n", username, password);
    send_line(sock, msg);

    char reply[64];
    recv_line(sock, reply, sizeof(reply));
    close_conn(sock);

    return (strncmp(reply, "OK", 2) == 0)
           ? SERVER_ACK
           : SERVER_NACK;
}

void protocol_logout(void) {
    int sock = connect_to(server_ip, server_port);
    if (sock < 0)
        return;

    send_line(sock, "LOGOUT\n");
    close_conn(sock);
}
