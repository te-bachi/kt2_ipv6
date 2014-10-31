#include "EchoClient.h"
#include "Message.h"
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
    const char         *text = "Das ist der Daumen, " \
                               "der schüttelt die Pflaumen, " \
                               "der liest sie auf, " \
                               "der trägt sie heim, " \
                               "und der kleine isst sie ganz allein.";

    //char                buffer[64];
    //int                 len;

    /* create socket */
    if((sockfd = socket(addrinfo->ai_family, addrinfo->ai_socktype, 0)) == -1) {
        Log_errno(LOG_ERROR, errno, "Can't create socket");
    }

    /* connect to server */
    if (connect(sockfd, addrinfo->ai_addr, addrinfo->ai_addrlen) == -1) {
        Log_errno(LOG_ERROR, errno, ("Can't connect to server"));
        CLIENT_FAILURE_EXIT
    }

    /* send request to upper */
    if (!Message_send(sockfd, REQUEST_TO_UPPER, 1, text, strlen(text))) {
        CLIENT_FAILURE_EXIT
    }

    /* send request to lower */
    if (!Message_send(sockfd, REQUEST_TO_LOWER, 2, text, strlen(text))) {
        CLIENT_FAILURE_EXIT
    }

    /* send request finish */
    if (!Message_send(sockfd, REQUEST_FINISH, 3, NULL, 0)) {
        CLIENT_FAILURE_EXIT
    }

    /* receive */





    return true;
}
