
#include "protobuf.h"

struct Protobuf Protobuf_createNew(unsigned char *buffer, unsigned long capacity) {
  return (struct Protobuf) {
    .buffer = { .write = buffer },
    .capacity = capacity,
  };
}

struct Protobuf Protobuf_wrapNew(const unsigned char *buffer, unsigned long capacity) {
  return (struct Protobuf) {
    .buffer = { .read = buffer },
    .capacity = capacity,
  };
}

static inline bool Protobuf_put(struct Protobuf *proto, unsigned char value) {
  if (proto->position >= proto->capacity) {
    return false;
  }

  proto->buffer.write[proto->position] = value;
  proto->position++;

  return true;
}

static inline bool Protobuf_putAll(struct Protobuf *proto, const unsigned char *values, unsigned long length) {
  while (length > 0) {
    if (!Protobuf_put(proto, *values)) {
      return false;
    }

    values++; length--;
  }

  return true;
}

static inline bool Protobuf_get(struct Protobuf *proto, unsigned char *value) {
  if (proto->position >= proto->capacity) {
    return false;
  }

  if (value != NULL) {
    *value = proto->buffer.read[proto->position];
  }
  proto->position++;

  return true;
}

static inline bool Protobuf_getAll(struct Protobuf *proto, unsigned char *values, unsigned long length) {
  while (length > 0) {
    if (!Protobuf_get(proto, values)) {
      return false;
    }

    values++; length--;
  }

  return true;
}

static inline bool Protobuf_writeVarint(struct Protobuf *proto, unsigned long value) {
  const unsigned long MSB = 0x80;
  const unsigned long MASK = 0x7F;

  while (value >= MSB) {
    unsigned char next = (unsigned char) ((value & MASK) | MSB);
    if (!Protobuf_put(proto, next)) {
      return false;
    }

    value >>= 7;
  }

  unsigned char last = (unsigned char) (value & MASK);
  if (!Protobuf_put(proto, last)) {
    return false;
  }

  return true;
}

static inline bool Protobuf_readVarint(struct Protobuf *proto, unsigned long *value) {
  const unsigned long MSB = 0x80;
  const unsigned long MASK = 0x7F;

  int shift = 0;
  unsigned char next;

  *value = 0;
  do {
    if (!Protobuf_get(proto, &next)) {
      return false;
    }

    *value += (next & MASK) << shift;
    shift += 7;
  } while (next >= MSB);

  return true;
}

static inline bool Protobuf_writeTag(struct Protobuf *proto, enum ProtobufType type, unsigned int fieldnum) {
  unsigned long tag = (fieldnum << 3) | type;
  return Protobuf_writeVarint(proto, tag);
}

bool Protobuf_writeBuffer(struct Protobuf *proto, unsigned char *value, unsigned long length, unsigned int fieldnum) {
  return Protobuf_writeTag(proto, PROTOBUF_TYPE_LEN, fieldnum)
      && Protobuf_writeVarint(proto, length)
      && Protobuf_putAll(proto, value, length);
}

bool Protobuf_writeString(struct Protobuf *proto, const char *value, unsigned int fieldnum) {
  unsigned long length = strlen(value);
  return Protobuf_writeTag(proto, PROTOBUF_TYPE_LEN, fieldnum)
      && Protobuf_writeVarint(proto, length)
      && Protobuf_putAll(proto, (const unsigned char *) value, length);
}

bool Protobuf_writeUInt(struct Protobuf *proto, unsigned int value, unsigned int fieldnum) {
  return Protobuf_writeTag(proto, PROTOBUF_TYPE_VARINT, fieldnum)
      && Protobuf_writeVarint(proto, value);
}

bool Protobuf_writeULong(struct Protobuf *proto, unsigned int value, unsigned int fieldnum) {
  return Protobuf_writeTag(proto, PROTOBUF_TYPE_VARINT, fieldnum)
      && Protobuf_writeVarint(proto, value);
}

static inline bool Protobuf_readTag(struct Protobuf *proto, enum ProtobufType type, unsigned int fieldnum) {
  unsigned long tag;
  if (!Protobuf_readVarint(proto, &tag)) {
    return false;
  }

  return (tag & 0xb111) == type && (tag >> 3) == fieldnum;
}

bool Protobuf_readBuffer(struct Protobuf *proto, unsigned char *value, unsigned long length, unsigned int fieldnum) {
  if (!Protobuf_readTag(proto, PROTOBUF_TYPE_LEN, fieldnum)) {
    return false;
  }

  unsigned long size;
  if (!Protobuf_readVarint(proto, &size) || size > length) {
    return false;
  }

  if (!Protobuf_getAll(proto, value, size)) {
    return false;
  }

  return true;
}

bool Protobuf_readString(struct Protobuf *proto, char *value, unsigned long length, unsigned int fieldnum) {
  if (!Protobuf_readTag(proto, PROTOBUF_TYPE_LEN, fieldnum)) {
    return false;
  }

  unsigned long size;
  if (!Protobuf_readVarint(proto, &size) || (size + 1) > length) {
    return false;
  }

  if (!Protobuf_getAll(proto, (unsigned char *) value, size)) {
    return false;
  }

  value[size + 1] = '\0';
  return true;
}

bool Protobuf_readUInt(struct Protobuf *proto, unsigned int *value, unsigned int fieldnum) {
  if (!Protobuf_readTag(proto, PROTOBUF_TYPE_VARINT, fieldnum)) {
    return false;
  }

  unsigned long varint;
  if (!Protobuf_readVarint(proto, &varint)) {
    return false;
  }

  if (varint > UINT_MAX) {
    return false;
  }

  *value = varint;
  return true;
}

bool Protobuf_readULong(struct Protobuf *proto, unsigned long *value, unsigned int fieldnum) {
  if (!Protobuf_readTag(proto, PROTOBUF_TYPE_VARINT, fieldnum)) {
    return false;
  }

  unsigned long varint;
  if (!Protobuf_readVarint(proto, &varint)) {
    return false;
  }

  if (varint > ULONG_MAX) {
    return false;
  }

  *value = varint;
  return true;
}
