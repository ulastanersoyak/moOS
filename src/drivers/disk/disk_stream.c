#include "disk_stream.h"
#include "../../kernel/config.h"
#include "../../libc/stdio/stdio.h"
#include "../../libc/stdlib/stdlib.h"
#include "disk.h"

struct disk_stream *
get_disk_stream (uint32_t disk_id)
{
  struct disk *disk = get_disk (disk_id);
  if (!disk)
    {
      return 0;
    }
  struct disk_stream *disk_stream = calloc (sizeof (struct disk_stream));
  disk_stream->disk = disk;
  disk_stream->pos = 0;
  return disk_stream;
}

int32_t
disk_stream_read (struct disk_stream *stream, void *buffer, size_t len)
{
  int32_t sector_idx = stream->pos / MASTER_MAIN_DISK_SECTOR_SIZE;
  int32_t offset_idx = stream->pos % MASTER_MAIN_DISK_SECTOR_SIZE;
  char local_buffer[MASTER_MAIN_DISK_SECTOR_SIZE];
  int32_t rs = disk_read_block (stream->disk, sector_idx, 1, local_buffer);
  if (rs < 0)
    {
      return rs;
    }
  size_t total = len > MASTER_MAIN_DISK_SECTOR_SIZE
                     ? MASTER_MAIN_DISK_SECTOR_SIZE
                     : len;
  for (size_t i = 0; i < total; i++)
    {
      *(char *)buffer++ = local_buffer[offset_idx + i];
    }
  stream->pos += total;
  if (total > MASTER_MAIN_DISK_SECTOR_SIZE)
    {
      disk_stream_read (stream, buffer, len - MASTER_MAIN_DISK_SECTOR_SIZE);
    }
  return rs;
}

void
stream_seek (struct disk_stream *stream, int32_t pos)
{
  stream->pos = pos;
}

void
stream_free (struct disk_stream *stream)
{
  free (stream);
}
