#include "EchoServer.h"
#include "Log.h"

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <signal.h>

#include <pthread.h>

#define ECHO_SERVER_SOCKET              0x01

#define THREAD_MAX                      2

#define RETURN_ON_ERROR(stat, str)      if (stat < 0) { \
                                            Log_errno(LOG_ERROR, errno, str); \
                                            if (listenfd >= 0) close(listenfd); \
                                            return false; \
                                        }


typedef struct _WorkerInfo {
    struct sockaddr    *client_addr;
    socklen_t           client_addrlen;
    int                 connectfd;
} WorkerInfo;

static bool EchoServer_installSignal(int signum, void (*sighandler) (int, siginfo_t *, void *));
static void EchoServer_sigint(int signal, siginfo_t *siginfo, void *context);
static uint8_t EchoServer_select(int socket);
static void *EchoServer_protocolThread(void *arg);
static void *EchoServer_workerThread(void *arg);

bool                running;
pthread_mutex_t     mutex_running = PTHREAD_MUTEX_INITIALIZER;

bool
EchoServer_create(struct addrinfo *addrinfo)
{
    pthread_t           tid[THREAD_MAX];
    int                 idx;
    int                 numThreads;
    int                 status;

    EchoServer_installSignal(SIGINT, EchoServer_sigint);

    running     = true;
    numThreads  = 0;

    for (idx = 0; running && addrinfo != NULL; addrinfo = addrinfo->ai_next, idx++) {

        /* allow only IPv4 and IPv6 */
        if (addrinfo->ai_family != AF_INET && addrinfo->ai_family != AF_INET6) {
            Log_println(LOG_WARN, "Ignore family %d", addrinfo->ai_family);
            continue;
        }

        if (idx >= THREAD_MAX) {
            Log_println(LOG_ERROR, "Maximum number of threads (= %d) exceeds", THREAD_MAX);
            break;
        }

        /* create new thread */
        if ((status = pthread_create(&tid[idx], NULL, EchoServer_protocolThread, addrinfo))) {
            Log_println(LOG_ERROR, "Can't create thread: error = %d", status);
            break;
        }

        numThreads++;
    }

    /* wait until all threads terminate */

    for (idx = 0; idx < numThreads; idx++) {
        if ((status = pthread_join(tid[idx], NULL))) {
            Log_println(LOG_ERROR, "Can't join thread: error = %d", status);
        }
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
    Log_println(LOG_DEBUG, "SIGINT");

    pthread_mutex_lock(&mutex_running);
    running = false;
    pthread_mutex_unlock(&mutex_running);
}

static void *
EchoServer_protocolThread(void *arg)
{
    struct addrinfo    *server_addrinfo = (struct addrinfo *) arg;
    int                 listenfd;
    int                 connectfd;
    int                 status;
    uint8_t             readyMask;
    bool                local_running;
    pthread_t           tid;
    pthread_attr_t      attr;
    WorkerInfo         *workerInfo;
    //int                 num_bytes;
    //char                buffer[64];
    //int                 len;

    /* create socket */
    Log_println(LOG_INFO, "Create %s socket", Log_getFamily(server_addrinfo->ai_family));
    listenfd = socket(server_addrinfo->ai_family, server_addrinfo->ai_socktype, 0);
    RETURN_ON_ERROR(listenfd, "Can't create socket");

    /* turn off IPv4 to IPv6 mapping */
    if (server_addrinfo->ai_family == AF_INET6) {
        int v6only = 1;
        Log_println(LOG_INFO, "Turn off IPv4 to IPv6 mapping");
        if (setsockopt(listenfd, IPPROTO_IPV6, IPV6_V6ONLY, &v6only, sizeof(v6only)) < 0) {
            Log_errno(LOG_ERROR, errno, "Can't set socket option IPV6_V6ONLY = 1");
            return NULL;
        }
    }

    /* bind address to socket */
    Log_println(LOG_INFO, "Bind address to socket");
    status = bind(listenfd, server_addrinfo->ai_addr, server_addrinfo->ai_addrlen);
    RETURN_ON_ERROR(status, "Can't bind address to socket");

    /* set to passive socket */
    Log_println(LOG_INFO, "Set to passive socket");
    status = listen(listenfd, CONFIG_LISTEN_QUEUE);
    RETURN_ON_ERROR(status, "Can't set to passive socket");

    do {
        readyMask = EchoServer_select(listenfd);

        if (readyMask & ECHO_SERVER_SOCKET) {
            /* Initialize and set thread detached attribute */
            pthread_attr_init(&attr);
            pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

            workerInfo                  = (WorkerInfo *) malloc(sizeof(WorkerInfo));
            workerInfo->client_addr     = malloc(server_addrinfo->ai_addrlen);
            workerInfo->client_addrlen  = server_addrinfo->ai_addrlen;

            connectfd = accept(listenfd, workerInfo->client_addr, &(workerInfo->client_addrlen));

            workerInfo->connectfd = connectfd;

            pthread_create(&tid, &attr, EchoServer_workerThread, workerInfo);

            pthread_attr_destroy(&attr);
        }

        pthread_mutex_lock(&mutex_running);
        local_running = running;
        pthread_mutex_unlock(&mutex_running);
    } while (local_running);

    return NULL;
}

static uint8_t
EchoServer_select(int socket)
{
    uint8_t             selected    = 0;    /* bit-field of selected sockets */
    fd_set              readfds;            /* list of monitored file descriptors */
    int                 numfds;             /* number of ready file descriptors */
    int                 maxfd       = 0;    /* maximum socket used in select() */
    struct timeval      tv = {              /* time structure used in select() */
        .tv_sec  = CONFIG_SELECT_WAIT_SECS,
        .tv_usec = CONFIG_SELECT_WAIT_USECS
    };

    FD_ZERO(&readfds);

    /* add packet socket to monitored list */
    FD_SET(socket, &readfds);
    if (socket > maxfd) maxfd = socket;

    /* you could add more socket file descriptors... */

    /* wait until one or multiple "file descriptor(s)" are ready (returns immediately after ready)
       or wait until time is up. Return value is number of ready
       "file descriptors", 0 (time is up) or -1 (error) */
    numfds = select(maxfd + 1, &readfds, NULL, NULL, &tv);
    switch (numfds) {
        /* error */
        case -1:
            Log_errno(LOG_ERROR, errno, "can't monitor socket");
            break;

        /* time up */
        case 0:
            break;

        /* fd ready */
        default:
            if (FD_ISSET(socket, &readfds)) selected |= ECHO_SERVER_SOCKET;
            /* you could check if more socket file descriptors are ready ... */
            break;
    }

    return selected;
}

static void *
EchoServer_workerThread(void *arg)
{
    WorkerInfo         *workerInfo = (WorkerInfo *) arg;

    char                host_str[NI_MAXHOST];
    char                ip_address_str[NI_MAXHOST];
    char                service_str[NI_MAXSERV];
    char                port_str[NI_MAXSERV];
    int                 status;

    status = getnameinfo(workerInfo->client_addr, workerInfo->client_addrlen, host_str, sizeof(host_str), service_str, sizeof(service_str), 0);
    if (status) {
        Log_gai(LOG_ERROR, status, "can't resolve client address");
        return NULL;
    }

    status = getnameinfo(workerInfo->client_addr, workerInfo->client_addrlen, ip_address_str, sizeof(ip_address_str), port_str, sizeof(port_str), NI_NUMERICHOST | NI_NUMERICSERV);
    if (status) {
        Log_gai(LOG_ERROR, status, "can't resolve client address");
        return NULL;
    }

    Log_println(LOG_DEBUG, "Connection from client %s (%s), service %s (%s)", host_str, ip_address_str, service_str, port_str);

    Log_println(LOG_DEBUG, "Connection from client %s (%s), service %s (%s)", host_str, ip_address_str, service_str, port_str);

    close(workerInfo->connectfd);

    /* receives a raw message and copies it to a structured message */
    CharsReceived = recv(CommunicationSocket, Buffer, sizeof(Buffer), 0);
    while (CharsReceived > 0) {
        Buffer[CharsReceived] = 0;
        fprintf(stdout,"%s", Buffer);
        CharsReceived = recv(CommunicationSocket, Buffer, sizeof(Buffer), 0);
    }


    /*
    n = recv(ConnectedSocket, Buffer, sizeof(Buffer), 0);
    while (n > 0) {
        Buffer[n] = 0;
        fprintf(stdout,"%d> %s", Visits, Buffer);
        n = recv(ListeningSocket, Buffer, sizeof(Buffer), 0);
    }

    sprintf(Buffer, "%d. Verbindung von Node %s, Port %d\n", Visits, inet_ntop(AF_INET6, &(ClientAddr.sin6_addr), AddressBuffer, INET6_ADDRSTRLEN), ClientAddr.sin6_port);

    Status = send(ConnectedSocket, Buffer, strlen(Buffer), 0);
    ExitOnError(Status, "send fehlgeschlagen: ", strerror(errno));

    Status = close(ConnectedSocket);
    ExitOnError(Status, "close fehlgeschlagen: ", strerror(errno));

*/

    return NULL;
}

