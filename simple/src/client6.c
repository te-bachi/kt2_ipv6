/*
---------------------------------------------------------------------------
 Program:   client

 Zweck:     Erstellt eine Verbindung zu einem Server und zeigt die 
            vom Server gesendeten Daten an.
 
 Syntax:    client [ server [port] ]
 
               server - IP-Adresse des Servers
               port   - Protokoll-Port-Nummer des Servers
 
 Anmerkung: Beide Parameter sind fakultativ.  
            - Falls "server" fehlt, wird "LocalHost" angenommen
            - Falls "port" fehlt, wird der Default-Wert
              "DefaultPortNumber" verwendet.

 --------------------------------------------------------------------------
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

/* Konstanten definieren                                                 */
const char *DefaultPortNumber   = "4711";               /* Default-Protokoll-Port */
const char *LocalHost           = "ip6-localhost";      /* Default-Server-Name    */

/* Macro um eine beliebige Datenstruktur (mittels Nullen) zu löschen     */
#define ClearMemory(s) memset((char*)&(s), 0, sizeof(s))

void ExitOnError(int Status, char* Text, char *ErrorText);

/* Prozedur zur Fehlerabfrage und Behandlung                             */
void ExitOnError(int Status, char* Text, char *ErrorText) {
    if (Status < 0) {
        fprintf(stderr, "%s%s\n", Text, ErrorText);
        exit(1);
    }
}

int main(int ArgumentCount, char* ArgumentValue[]) {

    struct addrinfo     hints;
    struct addrinfo    *addrinfo = NULL;

    const char*     ServerName;          /* Temp.Pointer auf Server-Namen */
    int             CommunicationSocket; /* Socket(-Descriptor)           */
    const char     *Service;             /* Service bzw. Port-Nummer (wie eingegeben)  */
    char            Buffer[1000];        /* Daten-Buffer                  */
    int             Status;              /* Status-Zwischenspeicher       */
    int             CharsReceived;       /* Anzahl empfangener Zeichen    */
    int             status;

    /* 1. Parameter der Kommandozeile verarbeiten:
     *
     * Falls eine Server-Adresse angegeben wurde, soll diese verwendet werden,
     * sonst der Default-Wert (Konstante "LocalHost"). */

    if (ArgumentCount > 1) {           /* Falls Server-Argument angegeben */
        ServerName = ArgumentValue[1];
    } else {
        ServerName = LocalHost;
    }

    /* 2. Parameter der Kommandozeile verarbeiten:
     *
     * Falls eine Port-Nummer angegeben wurde, soll diese für das Protokoll
     * verwendet werden, sonst der Default-Wert (Konstante DefaultPortNumber). */

    if (ArgumentCount > 2) {                         /* Falls Port-Parameter angegeben */
        Service = ArgumentValue[2];                  /* String binär wandeln */
    } else {
        Service = DefaultPortNumber;                 /* Default verwenden */
    }

    /* Es wird versucht die numerische oder symbolische angegebene Server-
     * Adresse in eine binäre Adresse umzuwandeln. Falls dies nicht gelingt,
     * wird das Programm mit einer Fehlermeldung beendet, sonst wird der
     * Wert in der Variablen "addrinfo->ai_addr" abgelegt.
     * Anmerkung: Unter Unix genügt gethostbyname (inet_addr ist unnötig) */

    /* Hinweise setzen, was wir unbedingt benötigen */
    hints.ai_family   = AF_INET6;        /* Wir möchten eine IPv6 Verbindung öffnen */
    hints.ai_socktype = SOCK_STREAM;     /* mit TCPv6 */
    hints.ai_flags    = 0;               /* die anderen Werte auf 0 initialisieren sonst können */
    hints.ai_protocol = 0;               /* sie beliebige Werte annehmen (= unvorhersehbares Verhalten) */

    /* Adress-Informationen zu diesen Hinweisen ermitteln */
    status = getaddrinfo(ServerName, Service, &hints, &addrinfo);

    if (status) {
        fprintf(stderr,"Ungueltiger Server-Adresse %s: %s\n", ServerName, gai_strerror(status));
        exit(1);
    }

    /* Socket für Verbindungsaufbau und Datentransfer erzeugen            */
    CommunicationSocket = socket(addrinfo->ai_family, addrinfo->ai_socktype, 0);
    ExitOnError(CommunicationSocket, "socket fehlgeschlagen: ", strerror(errno));

    /* Verbindung zum Server und Dienst erstellen                         */
    Status = connect(CommunicationSocket, addrinfo->ai_addr, addrinfo->ai_addrlen);
    ExitOnError(Status, "connect fehlgeschlagen: ", strerror(errno));

    /* Wiederholt Daten vom Server lesen und am Bildschirm anzeigen       */
    CharsReceived = recv(CommunicationSocket, Buffer, sizeof(Buffer), 0);
    while (CharsReceived > 0) {
        Buffer[CharsReceived] = 0;
        fprintf(stdout,"%s", Buffer);
        CharsReceived = recv(CommunicationSocket, Buffer, sizeof(Buffer), 0);
    }

    /* Close the socket                                                   */
    close(CommunicationSocket);
    ExitOnError(Status, "close fehlgeschlagen: ", strerror(errno));

    /* Programm mit positivem Status beenden */
    exit(0);
}

