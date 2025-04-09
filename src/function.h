#ifndef _FUNCTION
#define _FUNCTION 1

#include <sys/stat.h>
#include <stddef.h>
#include <openssl/evp.h>
#include <stdbool.h>

// STRINGS
void String_serialise(const char* str, FILE* fd);
// Returns false if too big for buff
bool String_deserialise_to_buffer(char* buff, unsigned int maxLenth, FILE* fd);
// Requires free
char* String_deserialise_to_new_buffer(FILE* fd);


// FILES 
#define FILE_PATH_LENGTH 256
#define MD5_LEN 16


typedef enum _FILE_TYPE {
    FILETYPE_FILE,
    FILETYPE_FOLDER
} FILETYPE;

typedef struct _FILE_DESCRIPTION {
    unsigned long long int  length;
    size_t                  lastModified;
    unsigned char           hash[MD5_LEN];
} FileDescription;

// FileDescription
void FileDescription_print_stats(FileDescription* fDescription);
FileDescription* FileDescription_generate(const char* path);
void FileDescription_generate_hash(const char* path, FileDescription* fDescription);
bool FileDescription_eq(FileDescription* a, FileDescription* b);
void FileDescription_serialise(FileDescription* fDescription, FILE* fd);
FileDescription* FileDescription_deserialise(FILE* fd);

// FileNode

typedef struct _FILE_NODE {
    FILETYPE            type;
    unsigned int        count;
    unsigned int        capacity;
    struct FileNode**   children;
    FileDescription*    fDescription;
    char                path[FILE_PATH_LENGTH];
} FileNode;

FileNode* FileNode_generate(const char* path, FILETYPE fileType);
void FileNode_populate_from_directory(FileNode* node, const char* path, bool shouldGenerateHash);
void FileNode_resize_children(FileNode* node);
void FileNode_add_child(FileNode* node, FileNode* child);
void FileNode_print(FileNode* node);
void FileNode_free(FileNode* node);
void FileNode_serialise(FileNode* node, FILE* fd);
FileNode* FileNode_deserialise(FILE* fd);



typedef struct _FILE_TREE {
    char            path[FILE_PATH_LENGTH];
    FileNode*       root;
} FileTree;

FileTree* FileTree_generate(const char* rootPath, bool shouldGenerateHashes);
FileTree* FileTree_read_from_file(const char* path);
bool FileTree_eq(FileTree* a, FileTree* b);
void FileTree_save(FileTree* tree, const char* path);
void FileTree_print(FileTree* tree);

// Contains a tree that describes the state of the files.
typedef struct _MANIFEST_DESCRIPTION {
    FileTree*           tree;
} ManifestDescription;

// ManifestDescription
ManifestDescription* Manifest_generate(FileTree* tree);

#endif