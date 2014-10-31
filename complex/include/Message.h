#ifndef __MESSAGE_H__
#define __MESSAGE_H__

#include <stdint.h>
#include <stdbool.h>

#include "RingBuffer.h"

#define MESSAGE_HEADER_LEN      4

typedef enum {
    REQUEST_TO_UPPER = 1,
    RESPONSE_TO_UPPER,
    REQUEST_TO_LOWER,
    RESPONSE_TO_LOWER,
    REQUEST_FINISH,
    RESPONSE_FINISH
} MessageType;

typedef struct {
    uint8_t         data[UINT16_MAX];       /**< Data */
    uint16_t        len;                    /**< Length of the whole raw message */
} MessageRaw;


typedef struct {
    uint8_t         type;                   /**< Type */
    uint8_t         flags;                  /**< For future use */
    uint16_t        len;                    /**< Payload length, exclude header (max. 65536 bytes) */
} MessageHeader;

typedef struct {
    MessageHeader   header;
    uint32_t        nr;                     /**< Request/Response number */
    char            data[UINT16_MAX];       /**< Data */
} Message;

bool            Message_send(int sockfd, MessageType type, uint32_t nr, const char *data, uint16_t len);
bool            Message_receive(int sockfd, RingBuffer *buffer, Message *msg);
MessageRaw     *Message_encode(MessageRaw *raw, Message *msg);
Message        *Message_decode(Message *msg, RingBuffer *buffer);

//MessageRaw     *Message_encode(MessageRaw *raw, Message *msg);
//Message        *Message_decode(Message *msg, MessageRaw *raw);

#endif
