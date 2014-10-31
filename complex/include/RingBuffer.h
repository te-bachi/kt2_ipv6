#ifndef __RINGBUFFER_H__
#define __RINGBUFFER_H__

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

typedef struct {
    pthread_mutex_t         mutex;                              /**< mutual exclusive      */
    uint32_t                max;                                /**< maximum buffer length */
    char                   *ringBuffer;                         /**< ring buffer           */
    uint32_t                readPointer;                        /**< read pointer          */
    uint32_t                writePointer;                       /**< write pointer         */
    uint32_t                size;                               /**< current buffer size   */
} RingBuffer;

RingBuffer         *RingBuffer_new          (int num_bytes);
void                RingBuffer_delete       (RingBuffer *this);

inline uint32_t     RingBuffer_getSize      (RingBuffer *this);
inline bool         RingBuffer_canRead      (RingBuffer *this);
inline bool         RingBuffer_canWrite     (RingBuffer *this);

bool                RingBuffer_read         (RingBuffer *this, char *buffer, uint16_t *size);
bool                RingBuffer_get          (RingBuffer *this, char *character);

bool                RingBuffer_write        (RingBuffer *this, char *buffer, uint16_t size);
bool                RingBuffer_put          (RingBuffer *this, char character);

/**
 *
 */
inline uint32_t
RingBuffer_getSize(RingBuffer *this)
{
    return this->size;
}


/**
 *
 */
inline bool
RingBuffer_canRead(RingBuffer *this)
{
    return (this->writePointer != this->readPointer);
}


/**
 * ring buffer full (write pointer is next to read pointer)?
 *      _______________________________
 *     |   |   |   |   |   |   |   |   |
 *     |   |   |   |   |   |   |   |   |
 *     |___|___|___|___|___|___|___|___|
 *       0   1   2   3   4   5   6   7
 *           ^   ^
 *           W   R
 *      _______________________________
 *     |   |   |   |   |   |   |   |   |
 *     |   |   |   |   |   |   |   |   |
 *     |___|___|___|___|___|___|___|___|
 *       0   1   2   3   4   5   6   7
 *       ^                           ^
 *       R                           W
 */
inline bool
RingBuffer_canWrite(RingBuffer *this)
{
    return (((this->writePointer + 1) & (this->max - 1)) != this->readPointer);
}

#endif
