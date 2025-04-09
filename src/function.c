#include <stdio.h>
#include <stdlib.h>
#include <openssl/evp.h>
#include <string.h>
#include <stdbool.h>
#include <dirent.h>
#include <sys/stat.h>

#include "function.h"


// Strings
void String_serialise(const char* str, FILE* fd){
    size_t length = strlen(str);
    fwrite(&length, sizeof(length),1, fd);
    fwrite(str, length, 1, fd);
}

// requires freee
char* String_deserialise_to_new_buffer(FILE* fd){
    size_t length = 0;
    fread(&length, sizeof(length), 1, fd);
    char* string = calloc(1, length+1);
    fread(string, length, 1, fd);
    return string;
}

bool String_deserialise_to_buffer(char* buff, unsigned int maxLenth, FILE* fd){
    size_t length = 0;
    fread(&length, sizeof(size_t), 1, fd);
    if (length > maxLenth){
        return false;
    }
    fread(buff, length, 1, fd);
    buff[length+1] = 0x0;
    return true;
}


// File Nodes

FileNode* FileNode_generate(const char* path, FILETYPE fileType){
    FileNode* node = calloc(1, sizeof(FileNode));
    strncpy(node->path, path, strlen(path));
    node->type = fileType;
    if (node->type == FILETYPE_FOLDER) {
        node->count = 0;
        node->capacity = 8;
        node->children = calloc(node->capacity, sizeof(FileNode*));
    }
    return node;
}

void FileNode_populate_from_directory(FileNode* node, const char* path, bool shouldGenerateHash){
    // Recursively generate the tree from the file directory.
    DIR*            fd;
    struct dirent*  in_file;
    char            pathBuffer[FILE_PATH_LENGTH];
    unsigned int    szPath = strlen(path);
    strncpy(pathBuffer, path, strlen(path));
    pathBuffer[szPath] = '/';
    szPath++;

      fd = opendir(path);
      if (fd== NULL){
          fprintf(stderr, "Error: Failed open directory '%s'.\n", path);
          return;
      }
  
      while( (in_file = readdir(fd))){
        memset(pathBuffer + szPath, 0x0, FILE_PATH_LENGTH - szPath);
        
        if (in_file->d_type == DT_REG ){
            strncpy(pathBuffer + szPath, in_file->d_name, strlen(in_file->d_name));
            FileNode* newNode = FileNode_generate(pathBuffer, FILETYPE_FILE);
            FileNode_add_child(node, newNode);
            // Generate the fDescription
            newNode->fDescription = FileDescription_generate(newNode->path);
            // Generate the file md5 hash
            if (shouldGenerateHash)
                FileDescription_generate_hash(pathBuffer, newNode->fDescription);

          } else if (in_file->d_type == DT_DIR) {
            if ((strncmp(in_file->d_name, "..", 2) == 0) || (strncmp(in_file->d_name, ".", 1) == 0) ) {
                continue;
            }
            strncpy(pathBuffer + szPath, in_file->d_name, strlen(in_file->d_name));
            FileNode* newNode = FileNode_generate(pathBuffer, FILETYPE_FOLDER);
            FileNode_add_child(node, newNode);
            FileNode_populate_from_directory(newNode, pathBuffer, shouldGenerateHash);
          }
      }

      closedir(fd);
}

void FileNode_resize_children(FileNode* node){
    if ((node->count) > (node->capacity/2)) {
        printf("COUNT: %d\n CAPACITY (%d)\n");
        node->capacity *= 2;
        if(node->children == NULL)
            return;
        node->children = realloc(node->children, node->capacity * sizeof(FileNode*));
    }
}

void FileNode_add_child(FileNode* node, FileNode* child){
    node->children[node->count] = (struct FileNode*) child;
    node->count = node->count + 1;
    // Resize if necessary.
    FileNode_resize_children(node);
}

void FileNode_print(FileNode* node){
    if( node->type == FILETYPE_FOLDER) {
        printf("Folder '%s' (%d) contents:\n", node->path, node->count);
        for (unsigned int i = 0; i < node->count; i++){
            FileNode_print((FileNode*) node->children[i]);
        }
    } else {
        printf("File: '%s'\n", node->path);
        if (node->fDescription != NULL){
            FileDescription_print_stats(node->fDescription);
        }
    }
}

void FileNode_free(FileNode* node){
    for(int i = 0; i < node->count; i++) {
        FileNode_free((FileNode*) node->children[i]);
    }
    if(node->fDescription != NULL)
        free(node->fDescription);
    free(node->children);
    free(node);
}

void FileNode_serialise(FileNode* node, FILE* fd){
    String_serialise(node->path, fd);
    fwrite(&node->type, sizeof(FILETYPE), 1, fd);
    if (node->type == FILETYPE_FOLDER){
        fwrite(&node->count, sizeof(node->count), 1, fd);
        for (unsigned int i = 0; i < node->count; i++)
        {
            FileNode_serialise((FileNode*) node->children[i], fd);
        }
    } else if (node->type == FILETYPE_FILE) {
        FileDescription_serialise(node->fDescription, fd);
    }
    
}
FileNode* FileNode_deserialise(FILE* fd){
    char* pathBuffer = String_deserialise_to_new_buffer(fd);
    printf("Deserialsied string: '%s'\n", pathBuffer);
    FILETYPE fileType;
    fread(&fileType, sizeof(FILETYPE), 1, fd);
    printf("deserialised fileType: '%d'\n", fileType);
    
    FileNode* node = FileNode_generate(pathBuffer, fileType); 
    if (node->type == FILETYPE_FOLDER){
        unsigned int childrenCount = 0;
        fread(&childrenCount, sizeof(node->count), 1, fd);
        printf("node cnt: %u\n", node->count);
        for(unsigned int i = 0; i < childrenCount; i++){
            FileNode* newNode = FileNode_deserialise(fd);
            FileNode_add_child(node, newNode);
        }
    } else if (node->type == FILETYPE_FILE) {
        node->fDescription = FileDescription_deserialise(fd);
        FileDescription_print_stats(node->fDescription);
    } 
    free(pathBuffer);
    printf("Read a node!\n");
    FileNode_print(node);
    return node;
}

// File Tree
FileTree* FileTree_generate(const char* rootPath, bool shouldGenerateHashes) {
    FileTree* tree = (FileTree*) calloc(1, sizeof(FileTree));
    strncpy(tree->path, rootPath, strlen(rootPath)); 
    tree->root = FileNode_generate(".", FILETYPE_FOLDER); 

    // Recursively generate the tree from the file directory.
    DIR*            fd;
    struct dirent*  in_file;
    char            pathBuffer[FILE_PATH_LENGTH];

    fd = opendir(rootPath);
    if (fd== NULL){
        fprintf(stderr, "Error: Failed open directory '%s'.\n", rootPath);
        return NULL;
    }

    closedir(fd);
    FileNode_populate_from_directory(tree->root, rootPath, shouldGenerateHashes);
    return tree;
}



// Returns true/0 if same, undefined if not
bool FileTree_compare(FileTree* a, FileTree* b){
    
    return true;
}



FileTree* FileTree_read_from_file(const char* path){
    FileTree* tree = calloc(1, sizeof(FileTree));
    FILE* f = fopen(path, "rb");
    tree->root = FileNode_deserialise(f);
    fclose(f);
    return tree;
}

// Savees a file tree to a file
void FileTree_save(FileTree* tree, const char* path){
    FILE* f = fopen(path, "wb");

    fclose(f);
}

void FileTree_print(FileTree* tree){
    printf("Printing Tree '%s'.", tree->path);
    FileNode_print(tree->root);
}

// File Descriptions
// Requires free
FileDescription* FileDescription_generate(const char* path){
    FileDescription* fileDescription = calloc(1, sizeof(FileDescription));
    struct stat status;

    if (stat(path, &status) == 0){
        fileDescription->length = status.st_size;
        fileDescription->lastModified = status.st_mtime;
        return fileDescription;
    }
    

    fprintf(stderr, "Error: Could not perform stat on path '%s'\n.", path);
    free(fileDescription);
    return NULL;
}

// Returns the bytes written/allocated, allocates memory
void FileDescription_serialise(FileDescription* fDescription, FILE* fd){
    fwrite(fDescription, sizeof(FileDescription), 1, fd);
}

//Requires free
FileDescription* FileDescription_deserialise(FILE* fd){
    FileDescription* fileDescription = calloc(1, sizeof(FileDescription));
    fread(fileDescription, sizeof(FileDescription), 1, fd);
    printf("%d\n", fileDescription->length);
    return fileDescription;
}

// ss
void FileDescription_generate_hash(const char* path, FileDescription* fDescription){
    // Read all bytes from the file;
    char* data = (char*) calloc(1, fDescription->length);
    FILE* fd = fopen(path, "rb");
    fread(data, fDescription->length, 1, fd);
    fclose(fd);

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    const EVP_MD* md_type = NULL;
    md_type = EVP_MD_fetch(NULL, "MD5", NULL);
    EVP_DigestInit_ex2(ctx, md_type, NULL);
    EVP_DigestUpdate(ctx, data, fDescription->length);
    EVP_DigestFinal_ex(ctx, fDescription->hash, NULL);
    EVP_MD_CTX_free(ctx);
    free(data);
}

bool FileDescription_changed(FileDescription* a, FileDescription* b){
    if (a->lastModified != b->lastModified)
        return true;
    return false;
}

bool FileDescription_eq(FileDescription* a, FileDescription* b){
    if (a->length != b->length)
        return false;

    for(unsigned int i = 0; i < MD5_LEN; i++){
        if(a->hash[i] != b->hash[i])
            return i;
    }
    return true;
}


void FileDescription_print_stats(FileDescription* fDescription){
    printf("size: %lld\n", fDescription->length);
    printf("modt: %ld\n", fDescription->lastModified);
    printf("hash: ");
    for (int i = 0; i < MD5_LEN; i++){
        printf("%x", fDescription->hash[i]);
    }
    printf("\n");
}