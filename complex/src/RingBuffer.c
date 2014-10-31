
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "RingBuffer.h"
#include "Log.h"

static inline bool RingBuffer_canRead (RingBuffer *this);
static inline bool RingBuffer_canWrite(RingBuffer *this);

/**
 *     ring buffer:
 *      _______________________________
 *     |   |   |   |   |   |   |   |   |
 *     |   |   |   |   |   |   |   |   |
 *     |___|___|___|___|___|___|___|___|
 *       0   1   2   3   4   5   6   7
 *       ^
 *       RW
 *
 */
RingBuffer *
RingBuffer_new(int num_bytes)
{
    RingBuffer *this = (RingBuffer *) malloc(sizeof(RingBuffer));

    if (pthread_mutex_init(&(this->mutex), NULL)) {
        Log_errno(LOG_ERROR, errno, "Failed to initialize mutex");
        free(this);
        return NULL;
    }

    this->max           = (uint32_t) (1 << num_bytes);
    this->ringBuffer    = (char *) malloc(this->max);
    this->readPointer   = 0;
    this->writePointer  = 0;
    this->size          = 0;

    return this;
}

void
RingBuffer_delete(RingBuffer *this)
{
    if (this != NULL) {
        delete(this);
    }
}

/**
 * read from ring buffer the whole data
 *
 * @param   this                    a
 * @param   buffer                  a
 * @param   size                    input is the size of the buffer (to check bounds),
 *                                  output is the actually size of read data.
 * @return                          a
 */
bool
RingBuffer_read(RingBuffer *this, char *buffer, uint16_t *size)
{
    uint16_t        first_stage_size;
    uint16_t        second_stage_size;
    uint16_t        read_size;
    bool            result = true;

    pthread_mutex_lock(&(this->mutex));

//    if (this == &(uart0->tx_ring)) {
//        uart_put(uart0, '-');
//    }

    if (!RingBuffer_canRead(this)) {
        result = false;
        goto RingBuffer_readExit;
    }

    /* is the buffer not as big as the ring buffer?
     * just read until buffer is full */
    if (*size < this->size) read_size = *size;
    else                    read_size = this->size;

    //  || *size < RINGBUFFER_MAX_SIZE

    /* write pointer has got an overflow ?
     * ring buffer two-stage copy
     *
     * ring buffer:
     *      _______________________________
     *     |   |   |   |   |   |   |   |   |
     *     | Y | Y | 0 |   |   |   | X | X |
     *     |___|___|___|___|___|___|___|___|
     *       0   1   2   3   4   5   6   7
     *               ^               ^
     *               W               R
     */
    if (this->writePointer < this->readPointer) {

        /* calculate first stage size */
        first_stage_size = this->max - this->readPointer;

        /* copy first stage characters
         *
         * event data:
         *      _______________________________
         *     |   |   |   |   |   |   |   |   |
         *     | X | X |   |   |   |   |   |   |
         *     |___|___|___|___|___|___|___|___|
         *       0   1   2   3   4   5   6   7
         */
        memcpy(buffer, &(this->ringBuffer[this->readPointer]), first_stage_size);

        /* calculate second stage size */
        second_stage_size = read_size - first_stage_size;

        /* copy second stage characters
         *
         * event data:
         *      _______________________________
         *     |   |   |   |   |   |   |   |   |
         *     | X | X | Y | Y | 0 |   |   |   |
         *     |___|___|___|___|___|___|___|___|
         *       0   1   2   3   4   5   6   7
         */
        memcpy(&(buffer[first_stage_size]), &(this->ringBuffer[0]), second_stage_size);

    /* no overflow of write pointer */
    } else {
        memcpy(buffer, &(this->ringBuffer[this->readPointer]), read_size);
    }

    /* size of read data */
    *size = read_size;

    /* set read pointer to write pointer
     *      _______________________________
     *     |   |   |   |   |   |   |   |   |
     *     |   |   |   |   |   |   |   |   |
     *     |___|___|___|___|___|___|___|___|
     *       0   1   2   3   4   5   6   7
     *               ^
     *               WR
     */
    this->readPointer = (this->readPointer + read_size) & (this->max - 1);

    /* decrement size from read size */
    this->size -= read_size;

RingBuffer_readExit:
    pthread_mutex_unlock(&(this->mutex));

    return result;
}

/**
 * get single character from ring buffer
 *
 * @param   this                    a
 * @param   character               a
 * @return                          a
 */
bool
RingBuffer_get(RingBuffer *this, char *character)
{
    bool result = true;
    pthread_mutex_lock(&(this->mutex));

    if (!RingBuffer_canRead(this)) {
        result = false;
        goto RingBuffer_getExit;
    }

    /* get charater */
    *character = this->ringBuffer[this->readPointer];

    /* decrement size */
    this->size -= 1;

    /* increment read pointer */
    this->readPointer = (this->readPointer + 1) & (this->max - 1);

RingBuffer_getExit:
    pthread_mutex_unlock(&(this->mutex));

    return result;
}

/**
 *
 */
bool
RingBuffer_write(RingBuffer *this, char *buffer, uint16_t size)
{
    uint16_t size_written;
    uint16_t write_size;
    bool     result = true;

    pthread_mutex_lock(&(this->mutex));

    if (!RingBuffer_canWrite(this)) {
        result = false;
        goto RingBuffer_writeExit;
    }

    /* buffer is not empty */
    if (size > 0) {
        /* cancel if the size doesn't fit in ring buffer */
        if (size >= (this->max - this->size)) {
            result = false;
            goto RingBuffer_writeExit;
        }
        write_size  = size;
        this->size += write_size;

        /* write pointer doesn't overflow */
        if (this->writePointer + write_size < this->max) {

            /* write to ring buffer */
            memcpy(&(this->ringBuffer[this->writePointer]), buffer, write_size);

            /* increment write pointer */
            this->writePointer += write_size;

        /* write pointer would overflow, handle it special */
        } else {

            /* write ring buffer until write pointer reaches maximum size
             *
             * ex. size = 5, MAX_SIZE = 8, write_pointer = 5, read_pointer = 3
             *
             *     buffer:
             *      ____________________
             *     |   |   |   |   |   |
             *     | X | X | X | Y | Y |
             *     |___|___|___|___|___|
             *       0   1   2   3   4
             *
             *     ring buffer:
             *      _______________________________
             *     |   |   |   |   |   |   |   |   |
             *     |   |   |   | a | b |   |   |   |
             *     |___|___|___|___|___|___|___|___|
             *       0   1   2   3   4   5   6   7
             *
             *     size_written = MAX_SIZE - write_pointer = 8 - 5 = 3
             *      _______________________________
             *     |   |   |   |   |   |   |   |   |
             *     |   |   |   | a | b | X | X | X |
             *     |___|___|___|___|___|___|___|___|
             *       0   1   2   3   4   5   6   7
             */
            size_written = this->max - this->writePointer;
            memcpy(&(this->ringBuffer[this->writePointer]), buffer, size_written);

            /* subtract size from already written data
             *
             * ex. size = 5, MAX_SIZE = 8, write_pointer = 5, read_pointer = 3
             *
             *     size = size - size_written = 5 - 3 = 2
             */
            write_size -= size_written;

            /* shift buffer to not already written data
             *
             * ex. buffer:
             *      ____________________
             *     |   |   |   |   |   |
             *     | X | X | X | Y | Y |
             *     |___|___|___|___|___|
             *       0   1   2   3   4
             *
             *     buffer = buffer + size_written
             *      _______
             *     |   |   |
             *     | Y | Y |
             *     |___|___|
             *       0   1
             */
            buffer += size_written;

            /* set write pointer to zero to write into the first ringbuffer entry */
            this->writePointer = 0;

            /* write to ring buffer */
            memcpy(&(this->ringBuffer[this->writePointer]), buffer, write_size);

            /* increment write pointer */
            this->writePointer += write_size;
        }
    }

RingBuffer_writeExit:
    pthread_mutex_unlock(&(this->mutex));

    return result;
}

/**
 *
 */
bool
RingBuffer_put(RingBuffer *this, char character)
{
    bool result = true;

    pthread_mutex_lock(&(this->mutex));

    if (!RingBuffer_canWrite(this)) {
        result = false;
        goto RingBuffer_putExit;
    }

    /* write to ring buffer */
    this->ringBuffer[this->writePointer] = character;

    /* increment write pointer */
    this->writePointer = (this->writePointer + 1) & (this->max - 1);

    /* increase size */
    this->size++;

RingBuffer_putExit:
    pthread_mutex_unlock(&(this->mutex));

    return result;
}
