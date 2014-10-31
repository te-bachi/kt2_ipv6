#include "Message.h"
#include "Log.h"

#include <string.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define SEND_BUFFER_SIZE    16
#define SEND_MAX_SIZE       16
#define RECV_MAX_SIZE       16

#define MESSAGE_FAILURE_EXIT        RingBuffer_delete(sendBuffer); \
                                    return false;



bool
Message_send(int sockfd, MessageType type, uint32_t nr, const char *data, uint16_t len)
{
    Message             msg;
    MessageRaw          raw;
    RingBuffer         *sendBuffer;
    int                 num_bytes;

    /* length exceeds maximum */
    if (len > sizeof(msg.data)) {
        Log_println(LOG_ERROR, "Length too long. Abort!");
        return false;
    }

    msg.header.type  = type;
    msg.header.flags = 0;
    msg.header.len   = len;
    msg.nr           = nr;

    if (data == NULL) {
        Log_println(LOG_INFO, "Send message without data");
    } else {
        strncpy(msg.data, data, sizeof(msg.data));
        Log_println(LOG_INFO, "Send message with data \"%s\"", data);
    }

    sendBuffer = RingBuffer_new(SEND_BUFFER_SIZE);
    Message_encode(&sendBuffer, &msg);

    while (RingBuffer_canRead(sendBuffer)) {
        raw.len = SEND_MAX_SIZE;
        RingBuffer_read(sendBuffer, raw.data, &raw.len);

        num_bytes = send(sockfd, raw.data, raw.len, 0);

        if (num_bytes == -1) {
            Log_errno(LOG_ERROR, errno, "Can't send");
            MESSAGE_FAILURE_EXIT;
        } else if (num_bytes != raw.len) {
            Log_println(LOG_ERROR, "Length mismatch. Abort!");
            MESSAGE_FAILURE_EXIT;
        }

    }

    return true;
}

/**
 *
 * @param   recvBuffer      receive buffer
 */
bool
Message_receive(int sockfd, RingBuffer *recvBuffer, Message *msg)
{

    struct timeval tv;
    MessageRaw          raw;
    int                 num_bytes;

    tv.tv_sec  = 1;
    tv.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    /* less than the size of a header */
    if (RingBuffer_getSize(recvBuffer) < MESSAGE_HEADER_LEN) {
        /* receive header */
        do {
            raw.len = recv(sockfd, raw.data, raw.len, 0);

            /* timeout */
            if (raw.len == -1 && errno == EAGAIN) {
                Log_errno(LOG_ERROR, errno, "Timeout");
                return false;
            }

            /* write into buffer */
            if (!RingBuffer_write(recvBuffer, raw.data, raw.len)) {
                Log_println(LOG_ERROR, "Can't write to ring buffer");
                return false;
            }

        } while (RingBuffer_getSize(recvBuffer) < MESSAGE_HEADER_LEN);
    }



}

/*
bool
{
}
*/

RingBuffer *
Message_encode(RingBuffer *buffer, Message *msg)
{
    uint16_t len    = htons(msg->header.len);   /* host to network order */
    uint32_t nr     = htonl(msg->nr);           /* host to network order */

    RingBuffer_write(buffer, (char *) msg->header.type,  sizeof(msg->header.type));
    RingBuffer_write(buffer, (char *) msg->header.flags, sizeof(msg->header.flags));
    RingBuffer_write(buffer, (char *) len,               sizeof(msg->header.len));
    RingBuffer_write(buffer, (char *) nr,                sizeof(msg->nr));
    RingBuffer_write(buffer, (char *) msg->data,         msg->header.len);

    return buffer;

//    uint16_t len;
//    uint32_t nr;
//
//    raw->len = 0;
//
//    /* type */
//    memcpy(&(raw->data[raw->len]), &(msg->header.type), sizeof(msg->header.type));
//    raw->len += sizeof(msg->header.type);
//
//    /* flags */
//    memcpy(&(raw->data[raw->len]), &(msg->header.flags), sizeof(msg->header.flags));
//    raw->len += sizeof(msg->header.flags);
//
//    /* len */
//    len = htons(msg->header.len); /* host to network order */
//    memcpy(&(raw->data[raw->len]), &len, sizeof(len));
//    raw->len += sizeof(msg->header.len);
//
//    /* nr */
//    nr = htonl(msg->nr); /* host to network order */
//    memcpy(&(raw->data[raw->len]), &nr, sizeof(nr));
//    raw->len += sizeof(msg->nr);
//
//    /* data */
//    memcpy(&(raw->data[raw->len]), msg->data, msg->header.len);
//    raw->len += msg->header.len;
//
//    return raw;
}

Message *
Message_decode(Message *msg, RingBuffer *buffer)
{
    return msg;
}
