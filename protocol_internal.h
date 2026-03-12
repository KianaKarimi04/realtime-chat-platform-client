//
// Created by kiana on 2/10/26.
//

#ifndef REALTIME_CHAT_PLATFORM_CLIENT_PROTOCOL_INTERNAL_H
#define REALTIME_CHAT_PLATFORM_CLIENT_PROTOCOL_INTERNAL_H
#include <stdint.h>

/*
 * Global Header: 8 bytes (packed)
 *
 *   Byte 0:  version       [4-bit major | 4-bit minor]  → 0x02
 *   Byte 1:  type          [5-bit resource | 2-bit CRUD | 1-bit ACK]
 *   Byte 2:  status        [4-bit major | 4-bit minor]
 *   Byte 3:  padding       0x00
 *   Byte 4-7: length       (network byte order, uint32)
 */
typedef struct __attribute__((packed)) {
    uint8_t  version;
    uint8_t  type;
    uint8_t  status;
    uint8_t  padding;
    uint32_t length;
} BigHeader;

/* Version: major=0, minor=2 → 0x02 */
#define BIG_VERSION  0x02

/* Resource Types (5 bits, upper bits of type byte) */
#define RES_SERVER        0x00   /* 00000 */
#define RES_ACTIVATION    0x01   /* 00001 */
#define RES_ACCOUNT       0x02   /* 00010 */
#define RES_LOG           0x03   /* 00011 */
#define RES_CHANNEL       0x04   /* 00100 — single channel read */
#define RES_CHANNELS      0x05   /* 00101 — channel list read  */
#define RES_MESSAGE       0x06   /* 00110 */

/* CRUD (2 bits) */
#define CRUD_CREATE 0x00
#define CRUD_READ   0x01
#define CRUD_UPDATE 0x02
#define CRUD_DELETE 0x03

/* ACK (1 bit) */
#define ACK_REQ 0
#define ACK_RES 1

/* Build the type byte: [resource(5) | crud(2) | ack(1)] */
#define MSG_TYPE(res, crud, ack) \
(((res) << 3) | ((crud) << 1) | (ack))

/* Status byte helpers (4-bit major | 4-bit minor) */
#define STATUS_OK   0x00

#endif //REALTIME_CHAT_PLATFORM_CLIENT_PROTOCOL_INTERNAL_H