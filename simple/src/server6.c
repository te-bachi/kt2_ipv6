/*
---------------------------------------------------------------------------

 Program:   server

 Zweck:     Erzeugt einen Socket und führt wiederholt aus:
              1) Warte auf eine Verbindung von einem Client
              2) Sende eine Meldung an den Client
              3) Beende die Verbindung

 Syntax:    server [ port ]
                port  - Protokoll-Port

 Anmerkung: Die Angabe von "port" ist fakultativ. Falls nicht angegeben,
            wird der Default-Wert "DefaultPortNumber" verwendet.

---------------------------------------------------------------------------
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
const int DefaultPortNumber     = 4711;  /* Default-Protokoll-Port       */
const int QueueLength           = 10;    /* Laenge der Request Queue     */

/* Macro um eine beliebige Datenstruktur (mittels Nullen) zu löschen     */
#define ClearMemory(s) memset((char*)&(s),0,sizeof(s))

void ExitOnError(int Status, char* Text, char *ErrorText);

/* Prozedur zur Fehlerabfrage und Behandlung                             */
void ExitOnError(int Status, char* Text, char *ErrorText) {
    if (Status < 0) {
        fprintf(stderr, "%s%s\n", Text, ErrorText);
        exit(1);
    }
}

int main(int ArgumentCount, char* ArgumentValue[]) {

    typedef struct sockaddr* SockAddrPtr; /* Pointer auf sockaddr         */

    /* Struktur für Server-Adresse und Pointer für Parameter-Übergabe     */
    struct sockaddr_in6 ServerAddr;
    const  SockAddrPtr ServerAddrPtr = (SockAddrPtr)&ServerAddr;

    /* Struktur für Client-Adresse und Pointer für Parameter-Übergabe     */
    struct sockaddr_in6 ClientAddr;
    const  SockAddrPtr ClientAddrPtr = (SockAddrPtr)&ClientAddr;

    int              ListeningSocket;   /* Socket für Verbindungsaufbau   */
    int              ConnectedSocket;   /* Socket für Datenübertragung    */
    int              UntestedPort;      /* Port Nummer (wie eingegeben)   */
    unsigned short   Port;              /* Protokoll-Port-Nummer          */
    unsigned         AddrLen;           /* Laenge der Adresse             */
    char             Buffer[1000];      /* Daten-Buffer                   */
    int              Status;            /* Status-Zwischenspeicher        */
    int              Visits;            /* bisherige Anzahl Verbindungen  */
    char             AddressBuffer[INET6_ADDRSTRLEN];

    /* Kommandozeile verarbeiten:
     *
     * Falls eine Port-Nummer angegeben wurde, soll diese für das Protokoll
     * verwendet werden, sonst der Default-Wert (Konstante DefaultPortNumber).
     * Falls der Wert ungültig ist, wird das Programm mit einer Fehlermeldung
     * beendet, sonst wird der Wert in der Variablen Port abgelegt.
     */

    if (ArgumentCount > 1) {                        /* Falls Port-Parameter angegeben */
        UntestedPort = atoi(ArgumentValue[1]);      /* String binär wandeln */
    } else {
        UntestedPort = DefaultPortNumber;           /* sonst Default-Port verwenden */
    }

    if (UntestedPort > 0 && UntestedPort < 65536) { /* gueltige Port-Nummer?  */
        Port = (unsigned short) UntestedPort;       /* Typ anpassen, übernehmen */
    } else {                                        /* Fehlermeldung ausgeben und beenden */
        fprintf(stderr, "Ungültige Port-Nummer %d\n", UntestedPort);
        exit(1);
    }

    /* Socket für Verbindungsaufbau erzeugen                              */
    ListeningSocket = socket(PF_INET6, SOCK_STREAM, 0);
    ExitOnError(ListeningSocket, "socket fehlgeschlagen: ", strerror(errno));

    /* Server Adresse und Port für den Dienst (Protokoll) definieren      */
    ClearMemory(ServerAddr);                        /* Alles mit Nullen loeschen */
    ServerAddr.sin6_family  = AF_INET6;             /* Address Family InterNET   */
    ServerAddr.sin6_addr    = in6addr_any;          /* Beliebiges Interface      */
    ServerAddr.sin6_port    = htons(Port);          /* Port, ggf. Bytes tauschen */

    /* Dem Socket die lokale Adresse und Port-Nummer zuordnen             */
    Status = bind(ListeningSocket, ServerAddrPtr, sizeof(ServerAddr));
    ExitOnError(Status, "bind fehlgeschlagen: ", strerror(errno));

    /* Socket in passiv Modus versetzen und Warteschlagengrösse festlegen */
    Status = listen(ListeningSocket, QueueLength);
    ExitOnError(Status, "listen fehlgeschlagen: ", strerror(errno));

    Visits = 0; /* Noch keine Verbindungen */
    printf("Server wartet an Port %d auf die erste Verbindung\n", Port);

    while (1) { /* Server Loop */

        AddrLen = sizeof(ClientAddr);     /* ... wird von accept verändert */

        /* Auf Client-Verbindung (connect) warten                             */
        ConnectedSocket = accept(ListeningSocket, ClientAddrPtr, &AddrLen);
        ExitOnError(ConnectedSocket, "accept fehlgeschlagen: ", strerror(errno));

        Visits++;
        printf("%d. Verbindung von %s, Port %d\n", Visits, inet_ntop(AF_INET6, &(ClientAddr.sin6_addr), AddressBuffer, INET6_ADDRSTRLEN), ClientAddr.sin6_port);

        /* Daten-Buffer aufbereiten                                           */
        sprintf(Buffer,"Dies ist die %d. Verbindung.\n",Visits);

        /* Daten-Buffer senden                                                */
        Status = send(ConnectedSocket, Buffer, strlen(Buffer), 0);
        ExitOnError(Status, "send fehlgeschlagen: ", strerror(errno));

        /* Client-Verbindung beenden                                          */
        Status = close(ConnectedSocket);
        ExitOnError(Status, "close fehlgeschlagen: ", strerror(errno));

    } /* Server Loop */
}
