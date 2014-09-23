#include "Process.h"

#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include <sys/prctl.h>

// extern char **environ;
// extern int    g_argc;
extern char **g_argv;

/**
 * Set the process title
 *
 * Overwrites environment variables.
 * @see http://articles.manugarg.com/aboutelfauxiliaryvectors.html
 *
 * EXPERIMENTAL!
 */
void
Process_setTitle(const char *fmt, ...)
{
    char           *title;
    int             len;
    va_list         args;

    va_start(args, fmt);
    vasprintf(&title, fmt, args);
    va_end(args);

    len = strlen(title);
    strncpy(g_argv[0], title, len);
    memset(&g_argv[0][len], '\0', strlen(&g_argv[0][len]));
    prctl(PR_SET_NAME, title, 0, 0, 0);

    free(title);
}
