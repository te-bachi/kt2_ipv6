#ifndef __MESSAGE_H__
#define __MESSAGE_H__

#include <stdint.h>

typedef struct {
    uint8_t         type;       /**< Request or response */
    uint8_t         flags;      /**< For future use */
    uint16_t        length;     /**< Payload length, exclude header (max. 65536 bytes) */
} MessageHeader;

typedef struct {
    MessageHeader   header;
    uint32_t        requestNr;
    char           *message;
} MessageRequest;


typedef struct {
    MessageHeader   header;
    uint32_t        responseNr;
    char           *message;
} MessageResponse;

#endif
