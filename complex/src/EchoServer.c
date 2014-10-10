#include "EchoServer.h"
#include "Log.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <signal.h>

#include <sys/types.h>
#include <sys/wait.h>

#define RETURN_ON_ERROR(stat, str)      if (stat == -1) { \
                                            Log_errno(LOG_ERROR, errno, str); \
                                            if (listenfd >= 0) close(listenfd); \
                                            return false; \
                                        }

static bool EchoServer_installSignal(int signum, void (*sighandler) (int, siginfo_t *, void *));
static void EchoServer_sigint(int signal, siginfo_t *siginfo, void *context);
static bool EchoServer_handleChild(struct addrinfo *addrinfo);

bool running;

bool
EchoServer_create(struct addrinfo *addrinfo)
{
    pid_t               pid;
    int                 status;

    EchoServer_installSignal(SIGINT, EchoServer_sigint);

    running = true;

    for (; running && addrinfo != NULL; addrinfo = addrinfo->ai_next) {

        /* allow only IPv4 and IPv6 */
        if (addrinfo->ai_family != AF_INET && addrinfo->ai_family != AF_INET6) {
            Log_println(LOG_WARN, "Ignore family %d", addrinfo->ai_family);
            continue;
        }

        /* fork child process (= duplicate parent process!!) */
        pid = fork();
        switch(pid) {
            /* error */
            case -1:
                /* TODO: if fork failes, how can the other childs be informed? */
                Log_errno(LOG_ERROR, errno, "Can't create child process");
                running = false;
                break;

            /* child */
            case 0:
                /* child waits a bit */
                usleep(2000 * (rand() % 10));

                EchoServer_handleChild(addrinfo);

                /* child exits (= breaks loop) */
                exit(EXIT_SUCCESS);
                break;

            /* parent */
            default:
                break;
        }
    }

    /* wait until all child processes terminate */
    while ((pid = waitpid(-1, &status, 0)) > 0) {
        /* ... */
    }
    return true;
}

static bool
EchoServer_installSignal(int signum, void (*sighandler) (int, siginfo_t *, void *))
{
    struct sigaction signal;

    bzero(&signal, sizeof(signal));

    /* set signal handler */
    signal.sa_sigaction = sighandler;

    /* signal handler takes 3 arguments, not one */
    signal.sa_flags = SA_SIGINFO;

    Log_println(LOG_DEBUG, "Install SIGINT signal-handler");
    if (sigaction(signum, &signal, NULL) == -1) {
        Log_errno(LOG_FATAL, errno, "Couldn't set signal action");
        return false;
    }

    return true;
}

static void
EchoServer_sigint(int signal, siginfo_t *siginfo, void *context)
{
    running = false;
}

static bool
EchoServer_handleChild(struct addrinfo *addrinfo)
{
    int                 listenfd;
    int                 status;
    //int                 num_bytes;
    //char                buffer[64];
    //int                 len;

    /* create socket */
    Log_println(LOG_INFO, "Create %s socket", Log_getFamily(addrinfo->ai_family));
    listenfd = socket(addrinfo->ai_family, addrinfo->ai_socktype, 0);
    RETURN_ON_ERROR(listenfd, "Can't create socket");

    /* bind address to socket */
    Log_println(LOG_INFO, "Bind address to socket");
    status = bind(listenfd, addrinfo->ai_addr, addrinfo->ai_addrlen);
    RETURN_ON_ERROR(status, "Can't bind address to socket");

    /* set to passive socket */
    Log_println(LOG_INFO, "Set to passive socket");
    status = listen(listenfd, CONFIG_LISTEN_QUEUE);
    RETURN_ON_ERROR(status, "Can't set to passive socket");

    return true;
}
