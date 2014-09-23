#include "Log.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <inttypes.h>
#include <unistd.h>
#include <pthread.h>

#include <arpa/inet.h>

static bool Log_header(LOG_PARAMETER_DECLARATION);

/* NOT thread-safe! */

typedef struct {
    FILE               *stream;
    LogLevel            level;
    uint8_t             flags;
    pthread_mutex_t     mutex;
} Log;

Log logger = {
    .stream = 0,
    .level  = 0,
    .flags  = 0,
    .mutex  = PTHREAD_MUTEX_INITIALIZER
};

/*** MESSAGES ****************************************************************/

void
Log_init(FILE *stream, LogLevel level, uint8_t flags)
{
    logger.stream = stream;
    logger.level  = level;
    logger.flags  = flags;
}

static bool
Log_header(LOG_PARAMETER_DECLARATION)
{

    if (logger.stream <= 0 || level > logger.level) {
        return false;
    }

    if ((logger.flags & LOG_FLAG_TIME) || (logger.flags & LOG_FLAG_DATE)) {
        time_t      unixtime;
        struct tm   now;
        unixtime = time(NULL);
        localtime_r(&unixtime, &now);

        /* time */
        if (!(logger.flags & LOG_FLAG_DATE)) {
            fprintf(logger.stream, "[%02d:%02d:%02d]",                now.tm_hour,
                                                                      now.tm_min,
                                                                      now.tm_sec);

        /* date */
        } else if (!(logger.flags & LOG_FLAG_TIME)) {
            fprintf(logger.stream, "[%02d.%02d.%02d]",                now.tm_mday,
                                                                      now.tm_mon + 1,
                                                                      now.tm_year + 1900);

        /* both */
        } else {
            fprintf(logger.stream, "[%02d.%02d.%02d %02d:%02d:%02d]", now.tm_mday,
                                                                      now.tm_mon + 1,
                                                                      now.tm_year + 1900,
                                                                      now.tm_hour,
                                                                      now.tm_min,
                                                                      now.tm_sec);
        }
    }

    /* PID */
    if (logger.flags & LOG_FLAG_PID) {
        fprintf(logger.stream, "[%5d]", getpid());
    }

#ifdef ENABLE_LOG_DEBUG
    if ((logger.flags & LOG_FLAG_FILENAME) && (logger.flags & LOG_FLAG_LINE)) {
        char filetrunk[20 + 1];
        char buffer[25 + 1];

        strncpy(filetrunk, filename, 21);
        filetrunk[20]= '\0';
        snprintf(buffer, 25, "%s:%d", filetrunk, line);

        fprintf(logger.stream, "[%-25s]", buffer);

    } else {
        /* filename */
        if (logger.flags & LOG_FLAG_FILENAME) {
            fprintf(logger.stream, "[%-20s]", filename);
        }

        /* line */
        if (logger.flags & LOG_FLAG_LINE) {
            fprintf(logger.stream, "[%4d]", line);
        }
    }

    /* function */
    if (logger.flags & LOG_FLAG_FUNCTION) {
        fprintf(logger.stream, "[%-40s]", function);
    }
#endif

    if      (level == LOG_FATAL_PRIVATE) fprintf(logger.stream, "[FATAL] ");
    else if (level == LOG_ERROR_PRIVATE) fprintf(logger.stream, "[ERROR] ");
    else if (level == LOG_WARN_PRIVATE)  fprintf(logger.stream, "[WARN ] ");
    else if (level == LOG_INFO_PRIVATE)  fprintf(logger.stream, "[INFO ] ");
    else if (level == LOG_DEBUG_PRIVATE) fprintf(logger.stream, "[DEBUG] ");

    return true;
}


void
Log_print(LOG_PARAMETER_DECLARATION, const char *format, ...)
{
    va_list             args;

    pthread_mutex_lock(&logger.mutex);

    Log_header(LOG_PARAMETER_IMPLEMENTATION);
    va_start(args, format);
    vfprintf(logger.stream, format, args);
    fflush(logger.stream);
    va_end(args);

    pthread_mutex_unlock(&logger.mutex);
}

void
Log_println(LOG_PARAMETER_DECLARATION, const char *format, ...)
{
    va_list             args;

    pthread_mutex_lock(&logger.mutex);

    Log_header(LOG_PARAMETER_IMPLEMENTATION);
    va_start(args, format);
    vfprintf(logger.stream, format, args);
    fprintf(logger.stream, "\n");
    fflush(logger.stream);
    va_end(args);

    pthread_mutex_unlock(&logger.mutex);
}

void
Log_append(LOG_PARAMETER_DECLARATION, const char *format, ...)
{
    va_list             args;

    pthread_mutex_lock(&logger.mutex);

    va_start(args, format);
    vfprintf(logger.stream, format, args);
    fflush(logger.stream);
    va_end(args);

    pthread_mutex_unlock(&logger.mutex);
}

void
Log_appendln(LOG_PARAMETER_DECLARATION, const char *format, ...)
{
    va_list             args;

    pthread_mutex_lock(&logger.mutex);

    va_start(args, format);
    vfprintf(logger.stream, format, args);
    fprintf(logger.stream, "\n");
    fflush(logger.stream);
    va_end(args);

    pthread_mutex_unlock(&logger.mutex);
}

void
Log_errno(LOG_PARAMETER_DECLARATION, int errnum, const char *format, ...)
{
    va_list             args;
    char                error_str[STRERROR_R_BUFFER_MAX];

    pthread_mutex_lock(&logger.mutex);

    Log_header(LOG_PARAMETER_IMPLEMENTATION);
    va_start(args, format);
    vfprintf(logger.stream, format, args);
    fflush(logger.stream);
    va_end(args);

    if (!strerror_r(errnum, error_str, sizeof(error_str))) {
        fprintf(logger.stream, ": %s\n", error_str);
    } else {
        fprintf(logger.stream, ": <lookup error number failed>\n");
    }

    pthread_mutex_unlock(&logger.mutex);
}

void
Log_charstream(LOG_PARAMETER_DECLARATION, const char *stream, const uint32_t len)
{
    /*      ________________________
     *     |   |   |   |   |   |    |
     *     | 0 | x | F | F | _ | \0 |
     *     |___|___|___|___|___|____|
     *       0   1   2   3   4   5
     */
    uint32_t                idx;

    pthread_mutex_lock(&logger.mutex);

    fprintf(logger.stream, " (len=%02" PRIu32 ") ", len);

    for (idx = 0; idx < len; idx++) {
        fprintf(logger.stream,  "%02" PRIx8 " ", (uint8_t) stream[idx]);
    }

    fprintf(logger.stream, "\n");

    pthread_mutex_unlock(&logger.mutex);
}

const char *
Log_getFamily(int family)
{
    switch (family) {
        case AF_INET:   return "IPv4";
        case AF_INET6:  return "IPv6";
        default:        return "Unknow";
    }
}
