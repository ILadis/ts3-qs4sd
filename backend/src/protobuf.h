#ifndef PROTOBUF_H
#define PROTOBUF_H

#include <stdbool.h>

#include <limits.h>
#include <string.h>

struct Protobuf {
  union {
    unsigned char *write;
    const unsigned char *read;
  } buffer;
  unsigned long position;
  unsigned long capacity;
};

// only includes supported types
enum ProtobufType {
  PROTOBUF_TYPE_VARINT = 0,
  PROTOBUF_TYPE_LEN = 2,
};

struct Protobuf Protobuf_createNew(unsigned char *buffer, unsigned long capacity);
struct Protobuf Protobuf_wrapNew(const unsigned char *buffer, unsigned long capacity);

bool Protobuf_writeBuffer(struct Protobuf *proto, unsigned char *value, unsigned long length, unsigned int fieldnum);
bool Protobuf_writeString(struct Protobuf *proto, const char *value, unsigned int fieldnum);
bool Protobuf_writeUInt(struct Protobuf *proto, unsigned int value, unsigned int fieldnum);
bool Protobuf_writeULong(struct Protobuf *proto, unsigned int value, unsigned int fieldnum);

bool Protobuf_readBuffer(struct Protobuf *proto, unsigned char *value, unsigned long length, unsigned int fieldnum);
bool Protobuf_readString(struct Protobuf *proto, char *value, unsigned long length, unsigned int fieldnum);
bool Protobuf_readUInt(struct Protobuf *proto, unsigned int *value, unsigned int fieldnum);
bool Protobuf_readULong(struct Protobuf *proto, unsigned long *value, unsigned int fieldnum);

#endif
