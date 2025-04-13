#ifndef _SERVER
#define _SERVER 1


#define DEFAULT_SYNCH_SERVER_PORT 6969

typedef struct _SYNCH_SERVER {
    unsigned int port;
    int socket;
    char* path;
} Server;

typedef struct _ThreadData {
    Server* server;
    int clientSocket;
} ThreadData;

void Server_start(Server* server);
void* Server_serve();

#endif