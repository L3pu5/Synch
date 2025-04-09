#include <stdio.h>
#include <stdlib.h>
#include "function.h"

#include "server.h"


void client() {

}

void server() {
    printf("Creating server\n");
    Server* server = calloc(1, sizeof(Server));
    server->port = DEFAULT_SYNCH_SERVER_PORT;
    printf("Starting server on %d.\n", server->port);
    Server_start(server); 
}

void test_fd(const char* path){
    FileDescription* fDescription = FileDescription_generate(path);
    FileDescription_generate_hash(path, fDescription);
    FileDescription_print_stats(fDescription);
    free(fDescription);
}

void test_ld(const char* path){
    FILE* f = fopen(path, "rb");
    FileDescription* fDesc = FileDescription_deserialise(f);
    printf("Dumping deserialised fd\n");
    FileDescription_print_stats(fDesc);
    free(fDesc);
    fclose(f);
}

void test_ft(const char* path){
    FileTree* tree = (FileTree*) FileTree_generate(path);
    FileTree_print(tree);

    FILE* f = fopen("test.txt", "wb");
    FileNode_serialise(tree->root, f);
    fflush(f);
    fclose(f);
}

int main () {
    //test_fd("./Makefile");
    test_ft(".");
    //test_ld("test.txt");
}