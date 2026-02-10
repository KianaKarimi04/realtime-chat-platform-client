//
// Created by kiana on 2/10/26.
//

#include "network.h"
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

int connect_to(const char *ip, int port) {
    int sock;
    struct sockaddr_in addr;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) return -1;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);

    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(sock);
        return -1;
    }

    return sock;
}

int send_line(int sock, const char *msg) {
    return send(sock, msg, strlen(msg), 0);
}

int recv_line(int sock, char *buf, int max) {
    int n = recv(sock, buf, max - 1, 0);
    if (n > 0) buf[n] = '\0';
    return n;
}

void close_conn(int sock) {
    close(sock);
}
