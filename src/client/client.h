#ifndef _CLIENT
#define _CLIENT 1
#include "../function.h"

#define DEFAULT_SYNCH_CLIENT_PORT 6969

typedef struct _SYNCH_CLIENT {
    unsigned int port;
    int socket;
    char* path;
} Client;

void Client_start(Client* client);
void* Client_connect();
ManifestComparison* TEST_Client_comparison();

#endif