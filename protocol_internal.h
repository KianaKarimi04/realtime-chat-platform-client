//
// Created by kiana on 2/10/26.
//

#ifndef REALTIME_CHAT_PLATFORM_CLIENT_PROTOCOL_INTERNAL_H
#define REALTIME_CHAT_PLATFORM_CLIENT_PROTOCOL_INTERNAL_H
#include <stdint.h>

/* Global Header: 8 bytes */
typedef struct __attribute__((packed)) {
    uint8_t version;
    uint8_t type;
    uint8_t status;
    uint8_t padding;
    uint32_t length;
} BigHeader;

/* Version */
#define BIG_VERSION_1 0x01

/* Resource Types (5 bits) */
#define RES_SERVER        0x00
#define RES_ACTIVATION    0x01
#define RES_ACCOUNT       0x02
#define RES_LOG           0x03

/* CRUD (2 bits) */
#define CRUD_CREATE 0x00
#define CRUD_READ   0x01
#define CRUD_UPDATE 0x02
#define CRUD_DELETE 0x03

/* ACK (1 bit) */
#define ACK_REQ 0
#define ACK_RES 1

/* Helper macro to build message type byte */
#define MSG_TYPE(res, crud, ack) \
((res << 3) | (crud << 1) | (ack))

#endif //REALTIME_CHAT_PLATFORM_CLIENT_PROTOCOL_INTERNAL_H