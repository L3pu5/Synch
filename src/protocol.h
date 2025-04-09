#ifndef _PROTOCOL
#define _PROTOCOL 1

typedef enum MESSAGE_TYPES {
    CONNECTION_START,
    CONNECTION_END
} Message_Types;

typedef struct _MESSAGE {
   MESSAGE_TYPES TYPE;
   int LENGTH;
   void* DATA;
} Message;

#endif