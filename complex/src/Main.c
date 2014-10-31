#include "Log.h"

#ifdef WITH_ECHO_CLIENT
#include "EchoClient.h"
#elif WITH_ECHO_SERVER
#include "EchoServer.h"
#elif WITH_WEB_CLIENT
#include "WebClient.h"
#endif

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <errno.h>

#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define OPT_REQUIRED            (void *) 1
#define OPT_UNRECOGNISED        (void *) 2
#define OPT_ARGUMENT_INVALID    (void *) 3

typedef struct {
    const char     *str;
    LogLevel        level;
} level_str_t;

typedef struct {
    const char     *str;
    int             family;
} mode_str_t;

static void usage(int argc, char *argv[]);
static void usage_help(int argc, char *argv[]);
static void usage_opt(int argc, char *argv[], const char *msg);

const level_str_t level_str[] = {
        { "NONE" ,      LOG_NONE_PRIVATE    },
        { "ERROR" ,     LOG_ERROR_PRIVATE   },
        { "WARNING",    LOG_WARN_PRIVATE    },
        { "INFO" ,      LOG_INFO_PRIVATE    },
        { "DEBUG" ,     LOG_DEBUG_PRIVATE   }
};

const mode_str_t mode_str[] = {
        { "unspec",     AF_UNSPEC   },
        { "IPv4",       AF_INET     },
        { "IPv6" ,      AF_INET6    }
};

int     g_argc;
char  **g_argv;
int     opt;        /**< argument for getopt() as a single integer */

static void
usage(int argc, char *argv[])
{
   fprintf(stderr, CONFIG_PROGRAM_DESC " " CONFIG_PROGRAM_VERSION "\n");
   fprintf(stderr, "Usage:\n%s %s\n", argv[0], CONFIG_PROGRAM_USAGE);
}

static void
usage_help(int argc, char *argv[])
{
    usage(argc, argv);

    fprintf(stderr, "\n");
    fprintf(stderr, "Examples:\n");
    fprintf(stderr, "%s %s\n", argv[0], CONFIG_PROGRAM_HELP1);
    fprintf(stderr, "%s %s\n", argv[0], CONFIG_PROGRAM_HELP2);
    fprintf(stderr, "%s %s\n", argv[0], CONFIG_PROGRAM_HELP3);
    fprintf(stderr, "\n");

    exit(EXIT_FAILURE);
}

static void
usage_opt(int argc, char *argv[], const char *msg)
{
    usage(argc, argv);

   if (msg == OPT_REQUIRED) {
       fprintf(stderr, "\nOption -%c requires an operand\n", optopt);
   } else if (msg == OPT_UNRECOGNISED) {
       fprintf(stderr, "\nUnrecognised option: -%c\n", optopt);
   } else if (msg == OPT_ARGUMENT_INVALID) {
       fprintf(stderr, "\nArgument invalid: -%c %s\n", opt, optarg);
   } else if (msg != NULL) {
       fprintf(stderr, "\n%s\n", msg);
   }

   exit(EXIT_FAILURE);
}

int
main(int argc, char *argv[])
{
    int                 idx;
    bool                hflag = false;
    bool                mflag = false;
    bool                lflag = false;
    int                 family;
    const char         *hostname = NULL;
    const char         *service  = CONFIG_SERVICE;

    struct addrinfo     hints;
    struct addrinfo    *addrinfo = NULL;
    char                host_str[NI_MAXHOST];
    char                ip_address_str[NI_MAXHOST];
    char                service_str[NI_MAXSERV];
    char                port_str[NI_MAXSERV];
    int                 status;

    g_argc = argc;
    g_argv = argv;

    //log_level = LOG_DEBUG;

    /* The getopt() function parses the command-line arguments */
    while ((opt = getopt(argc, argv, ":hm:l:")) != -1) {
        switch (opt) {
            /* option: help */
            case 'h':
                hflag = true;
                break;

            /* option: mode */
            case 'm':
                for (idx = 0; idx < (sizeof(mode_str) / sizeof(mode_str_t)); idx++) {
                    if (strcasecmp(optarg, mode_str[idx].str) == 0) {
                        mflag  = true;
                        family = mode_str[idx].family;
                        break;
                    }
                }

                if (!mflag) {
                    usage_opt(argc, argv, OPT_ARGUMENT_INVALID);
                }
                break;

            /* option: log level */
            case 'l':
                for (idx = 0; idx < (sizeof(level_str) / sizeof(level_str_t)); idx++) {
                    if (strcasecmp(optarg, level_str[idx].str) == 0) {
                        lflag     = true;
                        //log_level = level_str[idx].level;
                        break;
                    }
                }

                if (!lflag) {
                    usage_opt(argc, argv, OPT_ARGUMENT_INVALID);
                }

                break;

            /**
             * missing option argument:
             * If the first character of optstring is a colon (':')
             * then getopt() returns ':' instead of '?' to indicate
             * a missing option argument.
             */
            case ':':
                usage_opt(argc, argv, OPT_REQUIRED);
                break;

            /* unrecognised option */
            case '?':
                usage_opt(argc, argv, OPT_UNRECOGNISED);
                break;

            default:
                usage_opt(argc, argv, NULL);
        }
    }

    if (hflag) {
        usage_help(argc, argv);

    }

    if (!mflag) {
        family = AF_UNSPEC;
    }

    /* additional arguments */
    if ((argc - optind) >= 1) {
        hostname = argv[optind];
    }

    if ((argc - optind) >= 2) {
        service  = argv[optind + 1];
    }

    Log_init(stderr, LOG_DEBUG_PRIVATE, LOG_FLAG_TIME | LOG_FLAG_PID | LOG_FLAG_FILENAME | LOG_FLAG_LINE);

#if defined(WITH_ECHO_CLIENT) ||defined(WITH_WEB_CLIENT)
    if (hostname == NULL) {
        Log_println(LOG_ERROR,("Hostname resp. IP-address must be specified"));
        return false;
    }
#else
    if (hostname == NULL) {
        //hostname = "0.0.0.0";
    }
#endif

    /* name resolution */
    memset(&hints, 0, sizeof(hints));

    hints.ai_family   = family;
    hints.ai_socktype = SOCK_STREAM;
#ifdef WITH_ECHO_SERVER
    hints.ai_flags = AI_PASSIVE;
#endif

    Log_println(LOG_ERROR, "hostname = %s, service = %s", hostname, service);

    status = getaddrinfo(hostname, service, &hints, &addrinfo);
    if (status) {
        Log_gai(LOG_ERROR, status, "Invalid address");
        return false;
    }

    status = getnameinfo(addrinfo->ai_addr, addrinfo->ai_addrlen, host_str, sizeof(host_str), service_str, sizeof(service_str), 0);
    if (status) {
        Log_gai(LOG_ERROR, status, "Invalid address");
        return false;
    }

    status = getnameinfo(addrinfo->ai_addr, addrinfo->ai_addrlen, ip_address_str, sizeof(ip_address_str), port_str, sizeof(port_str), NI_NUMERICHOST | NI_NUMERICSERV);
    if (status) {
        Log_gai(LOG_ERROR, status, "Invalid address");
        return false;
    }

#ifdef WITH_ECHO_CLIENT
    Log_println(LOG_DEBUG, "Connect to server %s (%s), service %s (%s)", host_str, ip_address_str, service_str, port_str);
    EchoClient_connect(addrinfo);
#elif WITH_ECHO_SERVER
    Log_println(LOG_DEBUG, "Listen on %s (%s), service %s (%s)", host_str, ip_address_str, service_str, port_str);
    EchoServer_create(addrinfo);
#elif WITH_WEB_CLIENT
    Log_println(LOG_DEBUG, "Connect from web server %s (%s), service %s (%s)", host_str, ip_address_str, service_str, port_str);
    WebClient_get(addrinfo);
#endif

    freeaddrinfo(addrinfo);

    return 0;
}
