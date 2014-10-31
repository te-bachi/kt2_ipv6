
#ifndef __LOG_H__
#define __LOG_H__

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

/*** DEFINES ****************************************************************/

#define LOG_PRINTF                      fprintf
#define LOG_VPRINTF                     vfprintf
#define LOG_FLUSH                       fflush

#define STRERROR_R_BUFFER_MAX           64

#ifdef ENABLE_LOG_DEBUG
#define LOG_LEVEL_ADDITION              ,__FILE__, __LINE__, __FUNCTION__
#define LOG_PARAMETER_DECLARATION       LogLevel level, \
                                        const char *filename, \
                                        int line, \
                                        const char *function
#define LOG_PARAMETER_IMPLEMENTATION    level, filename, line, function
#define LOG_FORMAT_STRING               5
#define LOG_FORMAT_PARAMETER            6
#else
#define LOG_LEVEL_ADDITION
#define LOG_PARAMETER_DECLARATION       LogLevel level
#define LOG_PARAMETER_IMPLEMENTATION    level
#define LOG_FORMAT_STRING               2
#define LOG_FORMAT_PARAMETER            3
#endif

#define LOG_NONE                        LOG_NONE_PRIVATE  LOG_LEVEL_ADDITION
#define LOG_FATAL                       LOG_FATAL_PRIVATE LOG_LEVEL_ADDITION
#define LOG_ERROR                       LOG_ERROR_PRIVATE LOG_LEVEL_ADDITION
#define LOG_WARN                        LOG_WARN_PRIVATE  LOG_LEVEL_ADDITION
#define LOG_INFO                        LOG_INFO_PRIVATE  LOG_LEVEL_ADDITION
#define LOG_DEBUG                       LOG_DEBUG_PRIVATE LOG_LEVEL_ADDITION

#define LOG_FLAG_FILENAME               0x01
#define LOG_FLAG_LINE                   0x02
#define LOG_FLAG_FUNCTION               0x04
#define LOG_FLAG_PID                    0x08
#define LOG_FLAG_TIME                   0x10
#define LOG_FLAG_DATE                   0x20

/*** DECLARATION ************************************************************/

typedef enum   LogLevel LogLevel;

enum LogLevel {
    LOG_NONE_PRIVATE = 0,
    LOG_FATAL_PRIVATE,
    LOG_ERROR_PRIVATE,
    LOG_WARN_PRIVATE,
    LOG_INFO_PRIVATE,
    LOG_DEBUG_PRIVATE
};

/*** DEFINITION *************************************************************/


void        Log_init                    (FILE *stream, LogLevel level, uint8_t flags);
void        Log_print                   (LOG_PARAMETER_DECLARATION, const char *format, ...)                __attribute__ ((format (printf, LOG_FORMAT_STRING, LOG_FORMAT_PARAMETER)));
void        Log_println                 (LOG_PARAMETER_DECLARATION, const char *format, ...)                __attribute__ ((format (printf, LOG_FORMAT_STRING, LOG_FORMAT_PARAMETER)));
void        Log_errno                   (LOG_PARAMETER_DECLARATION, int errnum, const char *format, ...)    __attribute__ ((format (printf, LOG_FORMAT_STRING + 1, LOG_FORMAT_PARAMETER + 1)));
void        Log_gai                     (LOG_PARAMETER_DECLARATION, int gai,    const char *format, ...)    __attribute__ ((format (printf, LOG_FORMAT_STRING + 1, LOG_FORMAT_PARAMETER + 1)));
void        Log_append                  (LOG_PARAMETER_DECLARATION, const char *format, ...)                __attribute__ ((format (printf, LOG_FORMAT_STRING, LOG_FORMAT_PARAMETER)));
void        Log_appendln                (LOG_PARAMETER_DECLARATION, const char *format, ...)                __attribute__ ((format (printf, LOG_FORMAT_STRING, LOG_FORMAT_PARAMETER)));
void        Log_charstream              (LOG_PARAMETER_DECLARATION, const char *stream, const uint32_t len);

const char *Log_getFamily               (int family);


#endif

