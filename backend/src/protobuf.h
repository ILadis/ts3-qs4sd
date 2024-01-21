#ifndef PROTOBUF_H
#define PROTOBUF_H

#include "../vnd/sha1.h"
#include "../vnd/pb_encode.h"
#include "../vnd/pb_decode.h"

static inline void
pb_ostream_write_buffer(pb_ostream_t *stream, const unsigned char *value, unsigned long size, unsigned int fieldnum) {
  pb_encode_tag(stream, PB_WT_STRING, fieldnum);
  pb_encode_string(stream, (const pb_byte_t *) value, size);
}

static inline void
pb_ostream_write_string(pb_ostream_t *stream, const char *value, unsigned int fieldnum) {
  pb_encode_tag(stream, PB_WT_STRING, fieldnum);
  pb_encode_string(stream, (const pb_byte_t *) value, strlen(value));
}

static inline void
pb_ostream_write_varint(pb_ostream_t *stream, unsigned int value, unsigned int fieldnum) {
  pb_encode_tag(stream, PB_WT_VARINT, fieldnum);
  pb_encode_varint(stream, (uint64_t) value);
}

static inline void
pb_ostream_write_varlong(pb_ostream_t *stream, unsigned long value, unsigned int fieldnum) {
  pb_encode_tag(stream, PB_WT_VARINT, fieldnum);
  pb_encode_varint(stream, (uint64_t) value);
}

static inline bool
pb_istream_read_fielddef(pb_istream_t *stream, pb_wire_type_t expected_type, unsigned int expected_fieldnum) {
  pb_wire_type_t type;
  unsigned int fieldnum;
  bool eof;

  if (!pb_decode_tag(stream, &type, &fieldnum, &eof) && !eof) return false;
  if (type != expected_type || fieldnum != expected_fieldnum) return false;

  return true;
}

static inline bool
pb_istream_read_varint(pb_istream_t *stream, unsigned int *value, unsigned int fieldnum) {
  if (!pb_istream_read_fielddef(stream, PB_WT_VARINT, fieldnum)) {
    return false;
  }

  unsigned int temp = 0;
  return pb_decode_varint32(stream, value == NULL ? &temp : value);
}

static inline bool
pb_istream_read_varlong(pb_istream_t *stream, unsigned long *value, unsigned int fieldnum) {
  if (!pb_istream_read_fielddef(stream, PB_WT_VARINT, fieldnum)) {
    return false;
  }

  unsigned long temp = 0;
  return pb_decode_varint(stream, value == NULL ? &temp : value);
}

static inline int
pb_istream_read_buffer(pb_istream_t *stream, unsigned char *value, unsigned long size, unsigned int fieldnum) {
  if (!pb_istream_read_fielddef(stream, PB_WT_STRING, fieldnum)) {
    return -1;
  }

  pb_istream_t substream;
  if (!pb_make_string_substream(stream, &substream)) {
    return -1;
  }

  if (size > substream.bytes_left) {
    size = substream.bytes_left;
  }

  if (!pb_read(&substream, value, size)) {
    return -1;
  }

  pb_close_string_substream(stream, &substream);
  return size;
}

static inline bool
pb_istream_read_string(pb_istream_t *stream, char *value, unsigned long size, unsigned int fieldnum) {
  int length = pb_istream_read_buffer(stream, (unsigned char *) value, size - 1, fieldnum);

  if (length > 0 && length < size) {
    value[length] = '\0';
    return true;
  }

  return false;
}

#endif
