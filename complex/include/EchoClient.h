#ifndef __ECHO_CLIENT_H__
#define __ECHO_CLIENT_H__

#define CONFIG_PROGRAM_NAME                 "echo_client"
#define CONFIG_PROGRAM_DESC                 "KT2 Echo Client"
#define CONFIG_PROGRAM_VERSION              "1.0"
#define CONFIG_PROGRAM_USAGE                "(-h | [-m <mode>] [-l <log level>] (<hostname> | <IP address>) [<service> | <port number>])"
#define CONFIG_PROGRAM_HELP1                "192.168.0.1"
#define CONFIG_PROGRAM_HELP2                "-m ipv4 192.168.0.1 echo"
#define CONFIG_PROGRAM_HELP3                "-m ipv6 -l DEBUG fe80::21b:21ff:fe5c:2201 6500"

#define CONFIG_SERVICE                      "2345"

#include <stdbool.h>
#include <netdb.h>

bool EchoClient_connect(struct addrinfo *addrinfo);

#endif
