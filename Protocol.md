# Protocol Specification

## Messages 

### Header
All messages contain a header that contains the following information:
{
    enum StatusCode -> {SUCCESS, ERROR_UNAUTHENTICTED, ERROR_DENIED, ERROR_} 
    enum MessageType
    MessageLength
}

### Message Types
The following message types define the contents of the expected message.

### ClientHelloRequest
```
{ 
    struct Header
    CLIENT
    VERSION (short)
    enum Intent -> {RequestManifest, Pull, Push}
}
```
```
struct Challenge {
    enum ChallengeType -> {None, Password, []?}
    Challenge*?
}
```

### ServerHelloResponse
```
{
    struct Header
    SERVER
    VERSION (short)
    struct Challenge
}
```
```
struct ChallengeResponse {
    struct Heaer
    Data?
}
```

### ClientChallengeResponse 
```
{
    struct Header
    struct ChallengeResponse
}
```

### ServerChallengeReply 
```
{
    struct Header
}
```


### ClientRequestPush 
```
struct  changeHeader{
    unsigned int index;
    char* path;
    bool isDirectory;
    ull size;
}
```

```
{
    struct Header
    Count of Changes
    // If a file is missing, we write it entirely
    array WriteIntent -> {
        struct changeHeader;
    }

    // If a file exists, we compare
    array ChangeIntent -> 
    {
        struct changeHeader;
    }
    
    array DeleteINtent -> {
        
    }
}
```

### ServerRequestWriteData 
```
{
    struct Header
    unsigned int index
}
```
### ClientReplyWriteData 
```
{
    struct Header
    DATA
}
```

### RequestChangeComparison
```
{
    struct Header
    unsigned int index
    size_t chunkCount
    size_t chunkSize
    array {
        md5hashes
    }
}
```

### ReplyChangeComparison
``` struct overwrite {
    size_t chunkIndex
    size_t chunkSize
    DATA
}
```

```
{
    struct Header
    unsigned int index
    enum ChangeReplyType -> { CONTINUE, WRITE}
    if WRITE:
        size_t overwriteCount
        array overWrite
    if CONTINUE:
        //Server should send a new RequestComparison
}
```

### ClientRequestManifest
```
{
    struct Header
}
```

### ?ServerReplyError 
```
{
    struct Header
    char* errorMsg;
}
```

### ServerReplyManifest
```
{
    struct Header
    Manifest Struct
}
```

### ClientRequestManifest
```
{
    struct Header
    Manifest Struct 
}
```

## ClientRequestNodeContents 
```
{}
```