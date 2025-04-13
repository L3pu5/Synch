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
            // Ignore the manifest itself
            if ((strncmp(in_file->d_name, ".manifest", 10) == 0))
                continue;

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
    FILETYPE fileType;
    fread(&fileType, sizeof(FILETYPE), 1, fd);
    
    FileNode* node = FileNode_generate(pathBuffer, fileType); 
    if (node->type == FILETYPE_FOLDER){
        unsigned int childrenCount = 0;
        fread(&childrenCount, sizeof(node->count), 1, fd);
        for(unsigned int i = 0; i < childrenCount; i++){
            FileNode* newNode = FileNode_deserialise(fd);
            FileNode_add_child(node, newNode);
        }
    } else if (node->type == FILETYPE_FILE) {
        node->fDescription = FileDescription_deserialise(fd);
        //FileDescription_print_stats(node->fDescription);
    } 
    free(pathBuffer);
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

FileTree* FileTree_read_from_file(const char* path){
    FileTree* tree = calloc(1, sizeof(FileTree));
    FILE* f = fopen(path, "rb");
    tree->root = FileNode_deserialise(f);
    fclose(f);
    return tree;
}

FileTree* FileTree_deserialise(FILE* fd){
    FileTree* tree = calloc(1, sizeof(FileTree));
    tree->root = FileNode_deserialise(fd);
    return tree;
}

bool _node_compare_type_and_name(FileNode* a, FileNode* b) {
    if (a->type != b->type)
        return false;
    
    if( a->type == FILETYPE_FOLDER){
        if (a->count != b->count)
            return false;
        // Compare the children
        for (unsigned int i = 0; i < a->count; i++){
            if (_node_compare_type_and_name((FileNode*) a->children[i], (FileNode*) b->children[i]) == false)
                return false;
        }

    } else {
        // Check if strlen eq.
        size_t sza = strlen(a->path);
        size_t szb = strlen(b->path);
        if( sza != szb)
            return false;
        return (memcmp(a->path, b->path, sza) == 0);
    }

    return true;
}



bool FileTree_structure_eq(FileTree* a, FileTree* b){
    // Recursively walk down each of the trees, they should all return the same value for names of files and directories.
    return _node_compare_type_and_name(a->root, b->root);
}

void FileTree_serialise(FileTree* tree, FILE* fd){
    FileNode_serialise(tree->root, fd);
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
    //intf("%lld\n", fileDescription->length);
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

// Manifests
// Requires Free
ManifestDescription* Manifest_generate(FileTree* tree){
    ManifestDescription* manifest = calloc(1, sizeof(ManifestDescription));
    manifest->tree = tree;
    Manifest_generate_digest(manifest);
    return manifest;
}

void Manifest_save_to_file(ManifestDescription* manifest, const char* path){
    FILE* f = fopen(path, "wb");
    fwrite(&manifest->digest, 1, MD5_LEN, f);
    FileTree_serialise(manifest->tree, f);
    fclose(f);
}

ManifestDescription* Manifest_read_from_file(const char* path){
    ManifestDescription* manifest = calloc(1, sizeof(ManifestDescription));
    FILE* fd = fopen(path, "rb");
    fread(&manifest->digest, MD5_LEN, 1, fd);
    manifest->tree = FileTree_deserialise(fd);
    fclose(fd);
    return manifest;
}


void _node_modify_digest(FileNode* node, char* digest) {
    if (node->type == FILETYPE_FILE){
        if(node->fDescription != NULL){
            for(unsigned int i = 0; i < MD5_LEN; i++) {
                digest[i] ^= node->fDescription->hash[i];
            }
        }
    } else if (node->type == FILETYPE_FOLDER) {
        for(unsigned int i = 0; i < node->count; i++) {
            _node_modify_digest((FileNode*) node->children[i], digest);
        }
    }
} 

void Manifest_generate_digest(ManifestDescription* manifest){
    _node_modify_digest(manifest->tree->root, manifest->digest);
}


// File Reigster Entry
// Requries free, handled in FileRegister_free();
FileRegisterEntry* FileRegisterEntry_generate_from_FileNode_to_FileRegister(FileNode* node, FileRegister* fRegister){
    if (node == NULL)
        return NULL;
    
    FileRegisterEntry* entry = calloc(1, sizeof(FileRegisterEntry));
    if (node->fDescription != NULL)
        memcpy(entry->hash, node->fDescription->hash, MD5_LEN);
    entry->pathLength = strlen(node->path);
    entry->type = node->type;
    entry->path = calloc(1, entry->pathLength+1);
    strncpy(entry->path, node->path, entry->pathLength);
    FileRegister_add_FileRegisterEntry(fRegister, entry);
    return entry;
}

void _node_to_FileRegisterEntry (FileNode* node, FileRegister* fRegister) {
    if (node->type == FILETYPE_FOLDER){
        for (unsigned int i = 0; i < node->count; i++){
            _node_to_FileRegisterEntry((FileNode*) node->children[i], fRegister);
        }
    } 
    FileRegisterEntry_generate_from_FileNode_to_FileRegister(node, fRegister);
}


void FileRegisterEntry_free(FileRegisterEntry* entry){
    if (entry == NULL)
        return;
    
    if (entry->path == NULL){
        free(entry);
        return;
    }
    
    free(entry->path);
    free(entry);
    return;
}

void FileRegisterEntry_print(FileRegisterEntry* entry){
    printf("%.*s\n",  (int) entry->pathLength, entry->path);
}

// FIle Register
// Requires Free
FileRegister* FileRegister_generate_from_FileTree(FileTree* tree){
    FileRegister* fRegister = calloc(1, sizeof(FileRegister));
    fRegister->capacity = 8;
    fRegister->entries = calloc(8, sizeof(FileRegisterEntry*));
    _node_to_FileRegisterEntry(tree->root, fRegister);
    return fRegister;
}


void FileRegister_print(FileRegister* fRegister){
    printf("---------\n");
    printf("File Register\n");
    printf("---------\n");
    printf("Entries:\n");
    for(unsigned int i = 0; i < fRegister->count; i++){
        FileRegisterEntry_print(fRegister->entries[i]);
    }
    printf("---------\n");
}

FileRegisterEntry* FileRegisterEntry_find_by_path(FileRegister* fRegister, char* path) {
    size_t szb = strlen(path);
    for(unsigned int i = 0; i < fRegister->count; i++){
        size_t sza = strlen(fRegister->entries[i]->path);
        if (sza != szb)
            continue;

        if (memcmp(fRegister->entries[i]->path, path, sza) == 0)
            return fRegister->entries[i];
    }
    
    return NULL;
}

FileRegisterEntry* FileRegister_find_by_hash(FileRegister* fRegister, char* hash){
    for(unsigned int i = 0; i < fRegister->count; i++){
        if (memcmp(fRegister->entries[i]->hash, hash, MD5_LEN) == 0)
            return fRegister->entries[i];
    }
    
    return NULL;
}


void FileRegister_add_FileRegisterEntry(FileRegister* fRegister, FileRegisterEntry* fEntry) {
    // Realloc if needed
    if (fRegister->count > (fRegister->capacity/2)) {
        fRegister->capacity *=  2;
        fRegister->entries = realloc(fRegister->entries, fRegister->capacity * sizeof(FileRegisterEntry*));
    }

    // Add
    fRegister->entries[fRegister->count] = fEntry;
    fRegister->count += 1;
}

void FileRegister_free(FileRegister* fRegister){
    if (fRegister->entries == NULL) {
        free(fRegister);
        return;
    }

    for(unsigned int i = 0; i < fRegister->count; i++){
        if (fRegister->entries[i] != NULL)
            free(fRegister->entries[i]);
    }
    free(fRegister->entries);
    free(fRegister);
}




// Manifest Comparison

void _ManifestComparison_add_difference(ManifestComparison* comparison, ManifestDifference* difference) {
    if (comparison->differenceCount > comparison->differenceCapacity/2){
        comparison->differenceCapacity *= 2;
        comparison->differences = realloc(comparison->differences, comparison->differenceCapacity * sizeof(ManifestDifference*));
    }
    
    comparison->differences[comparison->differenceCount] = difference;
    comparison->differenceCount += 1;
}

void _ManifestComparison_generate_differences(ManifestComparison* comparison){
    comparison->differenceCapacity = 8;
    comparison->differences = calloc(8, sizeof(ManifestDifference*));
    
    // Consider all the old files. Check that they exist 
    for (size_t i = 0; i < comparison->fRegisterOld->count; i++){
        FileRegisterEntry* thisRegistry = comparison->fRegisterOld->entries[i];
        FileRegisterEntry* matchingRegistry = NULL; 
        ManifestDifference* difference = NULL;
        
        // Check to see if it is in the new register
        if ( (matchingRegistry = FileRegisterEntry_find_by_path(comparison->fRegisterNew, thisRegistry->path) )) {
            // Mark the changed as seen as seen
            matchingRegistry->consideredDuringEvaluation = true;
            
            if ( matchingRegistry->type == thisRegistry->type && matchingRegistry->type == FILETYPE_FILE ){
                // If it is, compare the hashes. If they match, no changes.
                if ( memcpy(matchingRegistry->hash, thisRegistry->hash, MD5_LEN) == 0){
                    continue;
                }
            }
        } 
        
        // Allocate because there is now a change.
        difference = calloc(1, sizeof(ManifestDifference));
        memcpy(difference->path, thisRegistry->path, thisRegistry->pathLength);
        
        // If matchingRegistry is NULL, this is a delete
        if (matchingRegistry == NULL){
            difference->type = FILEDIFFERENCE_DELETED;
            _ManifestComparison_add_difference(comparison, difference);
            continue;
        }
        
        // Consider filetype change as a FILEDIFFERENCE_TYPE
        if (matchingRegistry->type != thisRegistry->type ){
            difference->type = FILEDIFFERENCE_TYPE;
            _ManifestComparison_add_difference(comparison, difference);
            continue;
        }
        
        // Else, content is different
        difference->type = FILEDIFFERENCE_CONTENT;
        _ManifestComparison_add_difference(comparison, difference);
        continue;
    }
    
    // Consider all the new files.
    for (size_t i = 0; i < comparison->fRegisterNew->count; i++)
    {
        FileRegisterEntry* newEntry = comparison->fRegisterNew->entries[i];
        // Disregard if seen
        if (newEntry->consideredDuringEvaluation)
            continue;
        
        // Add as a new file
        ManifestDifference* difference = calloc(1, sizeof(ManifestDifference));
        memcpy(difference->path, newEntry->path, newEntry->pathLength);
        difference->type = FILEDIFFERENCE_NEW;
        _ManifestComparison_add_difference(comparison, difference);
        continue;
    }
}

ManifestComparison* ManifestComparison_generate(ManifestDescription* old, ManifestDescription* new){
    ManifestComparison* comparison = calloc(1, sizeof(ManifestComparison));
    if (memcmp(old->digest, new->digest, MD5_LEN) == 0){
        comparison->manifestDiffers = false;
        return comparison;
    }

    comparison->manifestDiffers = true;
    // Walk the trees to determine if the tree structures are the same
    if (!FileTree_structure_eq(old->tree, new->tree)){
        comparison->treeDiffers = true;
    }
    
    // Generate FileRegisters
    comparison->fRegisterNew = FileRegister_generate_from_FileTree(new->tree);
    comparison->fRegisterOld = FileRegister_generate_from_FileTree(old->tree);
    
    // Generate Differences
    _ManifestComparison_generate_differences(comparison);
    
    
    return comparison;
}

void ManifestComparison_print(ManifestComparison* comparison) {
    printf("------------\n");
    printf("Manifest Comparison:\n");
    printf(" Manifest Differs: ");
    if( comparison->manifestDiffers)
        printf("TRUE\n");
    else
        printf("FALSE\n");

    printf(" Tree Differs: ");
    if( comparison->treeDiffers)
        printf("TRUE\n");
    else
        printf("FALSE\n");
        

    printf("  Differences:\n");
    printf("  Count: %u\n", comparison->differenceCount);
    for(size_t i = 0; i < comparison->differenceCount; i++){
        printf("Difference '%s': %d\n", comparison->differences[i]->path, comparison->differences[i]->type);
    }
    printf("------------\n");
}

void ManifestComparison_free(ManifestComparison* comparison){
    if (comparison == NULL)
        return;

    if (comparison->differences == NULL){
        free(comparison);
        return;
    }

    for (unsigned int i = 0; i < comparison->differenceCount; i++){
        free(comparison->differences[i]);
    }
    free (comparison);
    return;
}
