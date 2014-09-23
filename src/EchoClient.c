#include "EchoClient.h"
#include "Log.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define CLIENT_FAILURE_EXIT         close(sockfd); \
                                    return false;

bool
EchoClient_connect(struct addrinfo *addrinfo)
{
    int                 sockfd;
    int                 num_bytes;
    char                buffer[64];
    int                 len;

    /* create socket */
    if((sockfd = socket(addrinfo->ai_family, addrinfo->ai_socktype, 0)) == -1) {
        Log_errno(LOG_ERROR, errno, "Can't create socket");
    }

    /* connect to server */
    if (connect(sockfd, addrinfo->ai_addr, addrinfo->ai_addrlen) == -1) {
        Log_errno(LOG_ERROR, errno, ("Can't connect to server"));
        CLIENT_FAILURE_EXIT
    }

    /* send */
    strncpy(buffer, "Ping!", sizeof(buffer));
    len = strnlen(buffer, sizeof(buffer));
    if ((num_bytes = send(sockfd, buffer, len, 0)) == -1) {
        Log_errno(LOG_ERROR, errno, ("Can't send"));
        CLIENT_FAILURE_EXIT
    }

    /* receive */





    return true;
}
