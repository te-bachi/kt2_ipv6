#ifndef __ECHO_SERVER_H__
#define __ECHO_SERVER_H__

#define CONFIG_PROGRAM_NAME                 "echo_server"
#define CONFIG_PROGRAM_DESC                 "KT2 Echo Server"
#define CONFIG_PROGRAM_VERSION              "1.0"
#define CONFIG_PROGRAM_USAGE                "(-h | [-m <mode>] [-l <log level>] [<service>])"
#define CONFIG_PROGRAM_HELP1                "2345"
#define CONFIG_PROGRAM_HELP2                "-m ipv4"
#define CONFIG_PROGRAM_HELP3                "-m ipv6 -l DEBUG"

#define CONFIG_SERVICE                      "2345"
#define CONFIG_LISTEN_QUEUE                 6

#include <stdbool.h>
#include <netdb.h>

bool EchoServer_create(struct addrinfo *addrinfo);

#endif
