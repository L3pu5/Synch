#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#include "server.h"

// https://gist.github.com/browny/5211329
void Server_start(Server* server) {
    server->socket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_address = {};
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(server->port);

    if (bind(server->socket, (struct sockaddr*) &server_address, sizeof(server_address)) != 0){
        fprintf(stderr, "Could not bind to port.\n");
        return;
    }

    while (1) {
        if (listen(server->socket, 2) != 0){
            fprintf(stderr, "Could not listen to socket.\n");
            return;        
        }
        int clientSocket = accept(server->socket, NULL, NULL);
        
        ThreadData* data = (ThreadData*) calloc(1, sizeof(ThreadData));
        data->server = server;
        data->clientSocket = clientSocket;
        
        pthread_t garbage;
        pthread_create(&garbage, NULL, Server_serve, data);
    }
}

void* Server_serve(void* data){
    printf("Closing the port.\n");
    ThreadData* tData = (ThreadData*) data;
    close(tData->clientSocket);
    free(data);
    fflush(stdout);
    return NULL;
}