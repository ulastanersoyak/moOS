#ifndef DISK_STREAM_H
#define DISK_STREAM_H

#include "disk.h"

#include <stdint.h>
#include <stddef.h>

struct disk_stream{
  int32_t pos;
  struct disk_t *disk;
};

struct disk_stream *get_disk_stream(uint32_t disk_id);

int32_t disk_stream_read(struct disk_stream *stream, void *buffer, size_t len);

void stream_seek(struct disk_stream *stream, int32_t pos);

void stream_free(struct disk_stream *stream);

#endif// !DISK_STREAM_H
