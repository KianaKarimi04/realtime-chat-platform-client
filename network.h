//
// Created by kiana on 2/10/26.
//

#ifndef REALTIME_CHAT_PLATFORM_CLIENT_NETWORK_H
#define REALTIME_CHAT_PLATFORM_CLIENT_NETWORK_H

int connect_to(const char *ip, int port);
int send_line(int sockfd, const char *msg);
int recv_line(int sockfd, char *buf, int max_len);
void close_conn(int sockfd);

#endif //REALTIME_CHAT_PLATFORM_CLIENT_NETWORK_H