#include <stdio.h>
#include <stdlib.h>
#include <string.h> 

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

void test_serialise_string(const char* path){
    FILE* f = fopen(path, "wb");
    char* string = malloc(strlen("asd"));
    strncpy(string, "asd", strlen("asd"));
    String_serialise(string, f);
    fclose(f);

    f = fopen(path, "rb");
    free(string);
    string = calloc(1, strlen("asd"));
    String_deserialise_to_buffer(string, strlen("asd"), f);
    fclose(f);
    printf("Read: '%s'\n", string);
}

void test_ft(const char* path){
    FileTree* tree = (FileTree*) FileTree_generate(path, true);
    FileTree_print(tree);

    FILE* f = fopen(".manifest", "wb");
    FileNode_serialise(tree->root, f);
    fclose(f);
}

void test_ld_ft(){
    FileTree* tree = FileTree_read_from_file(".manifest");
    //FileTree_print(tree);
}

int main () {
    //test_fd("./Makefile");
    //test_serialise_string("asd.txt");
    //exit(1);
    
    test_ft(".");
    printf("Reading tree.\n");
    test_ld_ft();
    //test_ld("test.txt");
}