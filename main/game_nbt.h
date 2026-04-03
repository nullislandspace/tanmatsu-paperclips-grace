#pragma once

#include <stdint.h>
#include <stdio.h>

// Tag type IDs
#define NBT_END      0x00
#define NBT_INT32    0x01
#define NBT_DOUBLE   0x02
#define NBT_STRING   0x03
#define NBT_COMPOUND 0x04
#define NBT_INT64    0x05

// File header constants
#define NBT_MAGIC_0       'C'
#define NBT_MAGIC_1       'L'
#define NBT_MAGIC_2       'I'
#define NBT_MAGIC_3       'P'
#define NBT_ENDIAN_CHECK  0x01020304
#define NBT_FORMAT_VERSION 4

// --- Writer ---

typedef struct {
    FILE* f;
    int   error;      // non-zero if any write failed
    int   swap;       // non-zero if byte-swapping needed (always 0 when writing)
} NbtWriter;

// Open a file for writing and emit the file header.
void nbt_write_open(NbtWriter* w, FILE* f);

// Write a compound open tag (children follow, close with nbt_write_end).
void nbt_write_compound(NbtWriter* w, const char* name);

// Write an END tag to close a compound.
void nbt_write_end(NbtWriter* w);

// Write scalar tags.
void nbt_write_int32(NbtWriter* w, const char* name, int32_t value);
void nbt_write_int64(NbtWriter* w, const char* name, int64_t value);
void nbt_write_double(NbtWriter* w, const char* name, double value);
void nbt_write_string(NbtWriter* w, const char* name, const char* value);

// --- Reader ---

typedef struct {
    FILE*    f;
    int      error;
    int      swap;     // non-zero if file endianness differs from host
    uint16_t version;  // format version from header
} NbtReader;

// Open a file for reading and validate the file header.
// Returns 0 on success, -1 on error.
int nbt_read_open(NbtReader* r, FILE* f);

// Peek at the next tag type without consuming it.
// Returns the tag type, or -1 on error/EOF.
int nbt_peek_type(NbtReader* r);

// Read a tag header (type + name). Caller provides name buffer.
// Returns the tag type, writes name into name_buf (null-terminated).
// Returns -1 on error.
int nbt_read_tag(NbtReader* r, char* name_buf, int name_buf_size);

// Read payloads (call after nbt_read_tag returned the matching type).
int32_t nbt_read_int32(NbtReader* r);
int64_t nbt_read_int64(NbtReader* r);
double  nbt_read_double(NbtReader* r);
// Reads string into buf (null-terminated). Returns length, or -1 on error.
int     nbt_read_string(NbtReader* r, char* buf, int buf_size);

// Skip an entire tag's payload (including nested compounds).
// tag_type is the type returned by nbt_read_tag.
void nbt_skip_payload(NbtReader* r, int tag_type);
