#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "client.h"
#include "../protocol.h"
#include "../synch.h"

void Client_start(Client* client) {
    client->socket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_address = {};
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_address.sin_port = htons(client->port);

    if (connect(client->socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        fprintf(stderr, "Connection failed.\n");
        return;
    }
    
    //char* message = "Hello World!\n";
    /*
    size_t sentBytes = 0;
    while (sentBytes < strlen(message)) {
        size_t bytes = send(client->socket, message, strlen(message) - sentBytes, 0);
        printf("Sent '%d' bytes.",  bytes);
        sentBytes += bytes;
    }
    */
    
    //Client_send_challenge_response(client);
    //Client_send_hello_request(client, MESSAGEINTENT_PULL);

    Client_send_push_request(client, TEST_Client_comparison());

    //close(client->socket);
    return;
}

bool Client_message_contains_buffer(MessageType type) {
    return (    type == SERVER_CHALLENGE_RESPONSE || 
                type == CLIENT_CHALLENGE_RESPONSE || 
                type == SERVER_CHALLENGE_REQUEST );
}

void Client_terminate_connection(Client* client) {
    return;
}

ManifestComparison* TEST_Client_comparison() {
    FileTree* tree = (FileTree*) FileTree_generate(".", true);
    ManifestDescription* manifest = Manifest_generate(tree);
    for (int i = 0; i < MD5_LEN; i++){
        printf("%c", (char) manifest->digest[i]);
    }
    printf("\n");
    // Generate the FileRegister live
    FileRegister* fRegister = FileRegister_generate_from_FileTree(manifest->tree);
    // Read the FileRegisteer 
    ManifestDescription* manifestOld = Manifest_read_from_file(".manifest");
    FileRegister* fRegisterOld = FileRegister_generate_from_FileTree(manifestOld->tree);
    FileRegister_print(fRegister);
    FileRegister_print(fRegisterOld);

    ManifestComparison* comparison = ManifestComparison_generate(manifestOld, manifest);
    ManifestComparison_print(comparison);
    // exit(15);
    return comparison;
}

void Client_send_message(Client* client, MessageHeader* header, MessageBuffer* buffer) {
    size_t sentBytes = 0;
    unsigned int offset = 0;
    if (buffer != NULL) {
       offset = (void*) header - (void*) buffer;
    }
    int bytesToSend = (header->length  + sizeof(buffer->length) - offset);
    while (sentBytes < bytesToSend) {
        size_t bytes = send(client->socket, header + sentBytes, bytesToSend - sentBytes, 0);
        printf("Sent '%d' bytes.\n",  bytes);
        sentBytes += bytes;
    }

    if (buffer != NULL) {
        sentBytes = 0;
        while (sentBytes < buffer->length) {
            size_t bytes = send(client->socket, buffer->buffer + sentBytes, buffer->length - sentBytes, 0);
            printf("Sent '%d' bytes.\n",  bytes);
            sentBytes += bytes;
        }
    }
    return;
}

void Client_send_message_changes(Client* client, MessageHeader* header, ChangeHeader* changes) {
    size_t sentBytes = 0;
    unsigned int offset = 0;
    if (changes != NULL) {
        offset = (void*) header - (void*) changes;
    }

    int bytesToSend = (header->length - offset);
    while (sentBytes < bytesToSend) {
        printf("Sending Bytes: %d\n", bytesToSend);
        size_t bytes = send(client->socket, header + sentBytes, bytesToSend - sentBytes, 0);
        printf("Sent '%d' bytes.\n",  bytes);
        sentBytes += bytes;
    }

    for (size_t i = 0; i < ((ClientPushRequest*) header)->changeCount; i++) {
        ChangeHeader* addr = ((ClientPushRequest*) header)->changeIntent[i];
        
        sentBytes = 0;
        bytesToSend = sizeof(ChangeHeader) - sizeof(addr->path.buffer);
        while (sentBytes < bytesToSend) {
            printf("Sending Bytes: %d\n", bytesToSend);
            size_t bytes = send(client->socket, addr, bytesToSend - sentBytes, 0);
            printf("Sent '%d' bytes.\n",  bytes);
            sentBytes += bytes;
        }
        sentBytes = 0;
        while (sentBytes < addr->path.length) {
            printf("Sending Bytes: %d\n", bytesToSend);
            size_t bytes = send(client->socket, addr->path.buffer + sentBytes, addr->path.length - sentBytes, 0);
            printf("Sent '%d' bytes.\n",  bytes);
            sentBytes += bytes;
        }
    }
    return;
}

void Client_send_hello_request(Client* client, MessageIntent intent) {
    ClientHelloRequest* message = calloc(1, sizeof(ClientHelloRequest));
    message->header.status = STATUSCODE_SUCCESS;
    message->header.type = CLIENT_HELLO_REQUEST;

    message->version = SYNCH_VERSION;
    message->intent = intent;

    message->header.length = sizeof(*message);
    Client_send_message(client, message, NULL);
}

void Client_send_challenge_response(Client* client) {
    ClientChallengeResponse* message = calloc(1, sizeof(ClientChallengeResponse));
    message->header.status = STATUSCODE_SUCCESS;
    message->header.type = CLIENT_CHALLENGE_RESPONSE;

    message->data.length = sizeof("Hello");
    message->data.buffer = calloc(1, sizeof("Hello"));
    strcpy(message->data.buffer, "Hello");

    message->header.length = sizeof(*message);
    // unsigned int offset = &(message->data) - message
    unsigned int offset = message->header.length - sizeof(message->data);
    Client_send_message(client, message, offset);
}

void Client_send_push_request(Client* client, ManifestComparison* comparison) {
    ClientPushRequest* message = calloc(1, sizeof(ClientPushRequest));
    message->header.status = STATUSCODE_SUCCESS;
    message->header.type = CLIENT_PUSH_REQUEST;

    if (comparison->manifestDiffers != true) {
        Client_terminate_connection(client);
        return;
    }

    message->changeCount = comparison->differenceCount;
    message->changeIntent = calloc(comparison->differenceCount, sizeof(ChangeHeader*));

    for (size_t i = 0; i < comparison->differenceCount; i++) {
        ChangeHeader* header = calloc(1, sizeof(ChangeHeader));
        message->changeIntent[i] = header;
        ManifestDifference* dif = comparison->differences[i];
        
        header->index = (unsigned int) i;
        header->isDirectory = dif->isDirectory;
        header->type = dif->type;
        header->size = dif->size;

        header->path.length = strlen(dif->path) + 1;
        header->path.buffer = calloc(1, header->path.length + 1);
        memcpy(header->path.buffer, dif->path, header->path.length + 1);
    }

    // printf("%p\n", message->changeIntent);
    Client_send_message_changes(client, message, message->changeIntent);
}