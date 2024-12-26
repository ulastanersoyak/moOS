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
  char local_buffer[MASTER_MAIN_DISK_SECTOR_SIZE];
  char *out = buffer;
  while (len > 0)
    {
      int32_t sector_idx = stream->pos / MASTER_MAIN_DISK_SECTOR_SIZE;
      int32_t offset_idx = stream->pos % MASTER_MAIN_DISK_SECTOR_SIZE;
      int32_t rs = disk_read_block (stream->disk, sector_idx, 1, local_buffer);
      if (rs < 0)
        {
          return rs;
        }
      // dont read past the end of the sector that is currently buffered
      size_t total = MASTER_MAIN_DISK_SECTOR_SIZE - offset_idx;
      if (total > len)
        {
          total = len;
        }
      for (size_t i = 0; i < total; i++)
        {
          *out++ = local_buffer[offset_idx + i];
        }
      stream->pos += total;
      len -= total;
    }
  return OK;
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
