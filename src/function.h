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
    char            digest[MD5_LEN];
} FileTree;

FileTree* FileTree_generate(const char* rootPath, bool shouldGenerateHashes);
FileTree* FileTree_read_from_file(const char* path);
bool FileTree_eq(FileTree* a, FileTree* b);
bool FileTree_structure_eq(FileTree* a, FileTree* b);
void FileTree_save(FileTree* tree, const char* path);
void FileTree_print(FileTree* tree);

// File Register [List of flattened files within a tree]
typedef struct _FILE_REGISTER_ENTRY {
    size_t      pathLength;
    char*       path;
    FILETYPE    type;
    char        hash[MD5_LEN];
    // consideredDuringEvaluation is used by the client/server when processing changes between two file registries.
    // This field is false by default, true if it has already been seen
    // During the second loop, this is checked and skipped if the file has been seen, otherwise it might be new.
    bool        consideredDuringEvaluation;
} FileRegisterEntry;

typedef struct _FILE_REGISTER {
    unsigned int count;
    unsigned int capacity;
    FileRegisterEntry** entries;
} FileRegister;


FileRegister*       FileRegister_generate_from_FileTree(FileTree* tree);
void                FileRegister_free(FileRegister* fRegister);
void                FileRegister_print(FileRegister* fRegister);
void                FileRegister_add_FileRegisterEntry(FileRegister* fRegister, FileRegisterEntry* fEntry);

void                FileRegisterEntry_print(FileRegisterEntry* entry);
FileRegisterEntry*  FileRegisterEntry_generate_from_FileNode_to_FileRegister(FileNode* node, FileRegister* fRegister);
void                FileRegistryEntry_free(FileRegisterEntry* entry);
FileRegisterEntry*  FileRegister_find_by_hash(FileRegister* fRegister, char* hash);

// Contains a tree that describes the state of the files.
typedef enum _DIFFERENCE_TYPE {
    FILEDIFFERENCE_NEW,
    FILEDIFFERENCE_CONTENT,
    FILEDIFFERENCE_TYPE,
    FILEDIFFERENCE_DELETED
} FILE_DIFFERENCE_TYPE;

typedef struct _MANIFEST_DIFFERENCE {
    char                        path[FILE_PATH_LENGTH];
    bool                        isDirectory;
    unsigned long long          size;
    FILE_DIFFERENCE_TYPE       type;
} ManifestDifference;

typedef struct _MANIFEEST_COMPARISON {
    bool                        manifestDiffers;
    bool                        treeDiffers;
    FileRegister*               fRegisterOld;
    FileRegister*               fRegisterNew;
    unsigned int                differenceCount;
    unsigned int                differenceCapacity;
    ManifestDifference**        differences;
} ManifestComparison;

typedef struct _MANIFEST_DESCRIPTION {
    FileTree*           tree;
    char                digest[MD5_LEN];
} ManifestDescription;

// ManifestDescription
ManifestDescription*    Manifest_generate(FileTree* tree);
void                    Manifest_save_to_file(ManifestDescription* manifest, const char* path);
ManifestDescription*    Manifest_read_from_file(const char* path);
void                    Manifest_generate_digest(ManifestDescription* manifest);
ManifestComparison*     ManifestComparison_generate(ManifestDescription* old, ManifestDescription* new);
void                    ManifestComparison_print(ManifestComparison* comparison);
void                    ManifestComparison_free(ManifestComparison* comparison);
#endif