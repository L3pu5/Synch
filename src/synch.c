#include <stdio.h>
#include <stdlib.h>
#include <string.h> 

#include "function.h"
#include "./server/server.h"
#include "./client/client.h"

void client() {
    printf("Establishing connection.\n");
    Client* client = calloc(1, sizeof(Client));
    client->port = DEFAULT_SYNCH_CLIENT_PORT;
    Client_start(client);
    printf("Connection on %d.\n", client->port);
}

void server() {
    printf("Creating server\n");
    Server* server = calloc(1, sizeof(Server));
    server->port = DEFAULT_SYNCH_SERVER_PORT;
    printf("Starting server on %d.\n", server->port);
    Server_start(server); 
}

void test_fr(const char* path){
    // FileTree* tree = (FileTree*) FileTree_generate(path, true);
    // ManifestDescription* manifest = Manifest_generate(tree);
    // for (int i = 0; i < MD5_LEN; i++){
    //     printf("%c", (char) manifest->digest[i]);
    // }
    // printf("\n");
    // // Generate the FileRegister live
    // FileRegister* fRegister = FileRegister_generate_from_FileTree(manifest->tree);
    // // Read the FileRegisteer 
    // ManifestDescription* manifestOld = Manifest_read_from_file(".manifest");
    // FileRegister* fRegisterOld = FileRegister_generate_from_FileTree(manifestOld->tree);
    // FileRegister_print(fRegister);
    // FileRegister_print(fRegisterOld);

    // ManifestComparison* comparison = ManifestComparison_generate(manifestOld, manifest);
    // // BUPPY CODE
    client();
    //ManifestComparison_print(comparison);
    exit(69);
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

    FILE* f = fopen(".manifest", "wb");
    FileNode_serialise(tree->root, f);
    fclose(f);
}

void test_mf(const char* path){
    FileTree* tree = (FileTree*) FileTree_generate(path, true);
    ManifestDescription* manifest = Manifest_generate(tree);
    for (int i = 0; i < MD5_LEN; i++){
        printf("%c", (char) manifest->digest[i]);
    }
    printf("\n");
    Manifest_save_to_file(manifest, ".manifest");
    ManifestDescription* manifest2 = Manifest_read_from_file(".manifest");
    for (int i = 0; i < MD5_LEN; i++){
        printf("%c", (char) manifest2->digest[i]);
    }
    printf("\n");
    exit(1);
}

void test_ld_ft(){
    FileTree* tree = FileTree_read_from_file(".manifest");
    //FileTree_print(tree);
}

int main () {
    //test_fd("./Makefile");
    //test_serialise_string("asd.txt");
    //exit(1);
    //test_mf(".");
    
    //client();
    test_fr(".");
    
    // test_ft(".");
    // printf("Reading tree.\n");
    // test_ld_ft();
    //test_ld("test.txt");
}