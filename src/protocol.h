#ifndef _PROTOCOL
#define _PROTOCOL 1

#include <stddef.h>
#include <stdbool.h>
#include "function.h"

typedef enum _STATUS_CODE {
    STATUSCODE_SUCCESS,
    STATUSCODE_ERROR_UNAUTHENTICATED,
    STATUSCODE_ERROR_DENIED,
    STATUSCODE_ERROR_
} StatusCode;

typedef enum _MESSAGE_TYPE {
    CLIENT_HELLO_REQUEST,
    SERVER_HELLO_RESPONSE,
    CLIENT_CHALLENGE_RESPONSE,
    SERVER_CHALLENGE_RESPONSE,
    SERVER_CHALLENGE_REQUEST,
    CLIENT_PUSH_REQUEST,
    SERVER_WRITE_REQUEST,
    CLIENT_WRITE_RESPONSE,
    CHANGE_COMPARISON_REQUEST,
    CHANGE_COMPARISON_RESPONSE,
    CLIENT_MANIFEST_REQUEST,
    SERVER_MANIFEST_RESPONSE,
    SERVER_ERROR_RESPONSE,
    CLIENT_NODE_COUNT_REQUEST
} MessageType;

typedef enum _MESSAGE_INTENT {
    MESSAGEINTENT_REQUEST_MANIFEST,
    MESSAGEINTENT_PULL,
    MESSAGEINTENT_PUSH
} MessageIntent;

typedef enum _CHALLENGE_TYPE {
    CHALLENGETYPE_NONE,
    CHALLENGETYPE_PASSWORD
} ChallengeType;

typedef enum _CHANGE_REPLY_TYPE {
    REPLY_CONTINUE,
    REPLY_WRITE
} ChangeReplyType;

typedef struct _MESSAGE_BUFFER {
    size_t length;
    char* buffer;
} MessageBuffer;

typedef struct _MESSAGE_HEADER {
    StatusCode      status;
    MessageType     type;
    size_t          length;
} MessageHeader;

typedef struct _CHANGE_HEADER {
    unsigned int index;
    bool isDirectory;
    FILE_DIFFERENCE_TYPE type;
    unsigned long long size;
    MessageBuffer path;
} ChangeHeader;

typedef struct _CHANGE_OVERWRITE {
    size_t chunkIndex;
    size_t chunkSize;
    MessageBuffer data;
} ChangeOverwrite;


typedef struct _SERVER_CHALLENGE_REQUEST {
    MessageHeader header;
    ChallengeType type;
    MessageBuffer data;   
} ServerChallengeRequest;

typedef struct _CLIENT_CHALLENGE_RESPONSE {
    MessageHeader header;
    MessageBuffer data;
} ClientChallengeResponse;

typedef struct _SERVER_CHALLENGE_RESPONSE {
    MessageHeader header;
    MessageBuffer data;
} ServerChallengeResponse;


typedef struct _CLIENT_HELLO_REQUEST {
    MessageHeader header;
    short version;
    MessageIntent intent;
} ClientHelloRequest;

typedef struct _CLIENT_PUSH_REQUEST {
    MessageHeader   header;
    unsigned int    changeCount;
    ChangeHeader**  changeIntent;
} ClientPushRequest;

typedef struct _SERVER_WRITE_REQUEST {
    MessageHeader header;
    unsigned int index;
} ServerWriteRequest;

typedef struct _CLIENT_WRITE_RESPONSE {
    MessageHeader header;
    //DATA???
} ClientWriteResponse;

typedef struct _CHANGE_COMPARISON_REQUEST {
    MessageHeader header;
    unsigned int index;
    size_t chunkCount;
    size_t chunkSize;
    char** hashes;
} ChangeComparisonRequest;

typedef struct _CHANGE_COMPARISON_RESPONSE {
    MessageHeader header;
    ChangeOverwrite** overwrites;
} ChangeComparisonReply;

typedef struct _CHANGE_COMPARISON_OVERWRITE { //maybe not best name? idk
    MessageHeader header;
    unsigned int index;
    ChangeReplyType type;
} ChangeComparisonOverwrite;

typedef struct _CLIENT_MANIFEST_REQUEST {
    MessageHeader header;
} ClientManifestRequest;

typedef struct _SERVER_MANIFEST_RESPONSE {
    MessageHeader header;
    ManifestDescription manifest; //change type?
} ServerManifestResponse;

typedef struct _SERVER_ERROR_RESPONSE {
    MessageHeader header;
    char* error;
} ServerErrorResponse;

typedef struct _CLIENT_NODE_REQUEST {
    int yep;
} ClientNodeRequest;

#endif