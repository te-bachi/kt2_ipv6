#ifndef __WEB_CLIENT_H__
#define __WEB_CLIENT_H__

#define CONFIG_PROGRAM_NAME                 "web_client"
#define CONFIG_PROGRAM_DESC                 "KT2 Web Client"
#define CONFIG_PROGRAM_VERSION              "1.0"
#define CONFIG_PROGRAM_USAGE                "(-h | [-m <mode>] [-l <log level>] (<hostname> | <IP address>))"
#define CONFIG_PROGRAM_HELP1                "www.google.com"
#define CONFIG_PROGRAM_HELP2                "-m ipv4 www.google.com"
#define CONFIG_PROGRAM_HELP3                "-m ipv6 -l DEBUG ipv6.google.com"

#define CONFIG_SERVICE                      "80"

#include <stdbool.h>
#include <netdb.h>

bool WebClient_get(struct addrinfo *addrinfo);

#endif
