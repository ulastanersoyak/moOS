#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "SDL_image.h"

// IMG_Load for xgalaga's images: non interlaced png with 8 or 16 bits per
// channel, grey, grey with alpha, rgb or rgba. inflate is a plain one, the
// images are small

// the bits of the compressed data, least significant first
struct bits
{
  const uint8_t *data;
  uint32_t len;
  uint32_t pos; // in bits
};

static int
getbits (struct bits *in, int count)
{
  int value = 0;
  for (int i = 0; i < count; i++, in->pos++)
    {
      if (in->pos / 8 >= in->len)
        {
          return -1;
        }
      value |= ((in->data[in->pos / 8] >> (in->pos % 8)) & 1) << i;
    }
  return value;
}

// a canonical huffman code: how many codes each length has and the symbols
// in code order
struct huffman
{
  uint16_t counts[16];
  uint16_t symbols[288];
};

static void
huffman_build (struct huffman *code, const uint8_t *lengths, int count)
{
  uint16_t offsets[16];
  memset (code->counts, 0, sizeof (code->counts));
  for (int i = 0; i < count; i++)
    {
      code->counts[lengths[i]]++;
    }
  code->counts[0] = 0;
  offsets[1] = 0;
  for (int i = 1; i < 15; i++)
    {
      offsets[i + 1] = offsets[i] + code->counts[i];
    }
  for (int i = 0; i < count; i++)
    {
      if (lengths[i])
        {
          code->symbols[offsets[lengths[i]]++] = i;
        }
    }
}

static int
huffman_decode (struct bits *in, const struct huffman *code)
{
  int value = 0, first = 0, index = 0;
  for (int len = 1; len < 16; len++)
    {
      int bit = getbits (in, 1);
      if (bit < 0)
        {
          return -1;
        }
      value |= bit;
      int count = code->counts[len];
      if (value - first < count)
        {
          return code->symbols[index + value - first];
        }
      index += count;
      first = (first + count) << 1;
      value <<= 1;
    }
  return -1;
}

static const uint16_t length_base[29]
    = { 3,  4,  5,  6,  7,  8,  9,  10, 11,  13,  15,  17,  19,  23, 27,
        31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258 };
static const uint8_t length_extra[29] = { 0, 0, 0, 0, 0, 0, 0, 0, 1, 1,
                                          1, 1, 2, 2, 2, 2, 3, 3, 3, 3,
                                          4, 4, 4, 4, 5, 5, 5, 5, 0 };
static const uint16_t dist_base[30]
    = { 1,    2,    3,    4,    5,    7,     9,     13,    17,  25,
        33,   49,   65,   97,   129,  193,   257,   385,   513, 769,
        1025, 1537, 2049, 3073, 4097, 6145,  8193,  12289, 16385, 24577 };
static const uint8_t dist_extra[30]
    = { 0, 0, 0, 0, 1, 1, 2, 2,  3,  3,  4,  4,  5,  5,  6,
        6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13 };

// decompresses the zlib stream in into out, returns how many bytes it made
// or -1
static int
inflate (const uint8_t *data, uint32_t len, uint8_t *out, uint32_t max)
{
  // skip the zlib header, the adler32 at the end isnt checked
  struct bits in = { data, len, 16 };
  uint32_t pos = 0;
  int last;
  do
    {
      last = getbits (&in, 1);
      int type = getbits (&in, 2);
      if (type == 0)
        {
          // stored: from the next byte on, length and its complement
          in.pos = (in.pos + 7) & ~7u;
          uint32_t at = in.pos / 8;
          if (at + 4 > len)
            {
              return -1;
            }
          uint32_t count = data[at] | data[at + 1] << 8;
          at += 4;
          if (at + count > len || pos + count > max)
            {
              return -1;
            }
          memcpy (out + pos, data + at, count);
          pos += count;
          in.pos = (at + count) * 8;
          continue;
        }
      struct huffman lit, dist;
      uint8_t lengths[288 + 32];
      if (type == 1)
        {
          // the fixed codes
          for (int i = 0; i < 288; i++)
            {
              lengths[i] = i < 144 ? 8 : i < 256 ? 9 : i < 280 ? 7 : 8;
            }
          huffman_build (&lit, lengths, 288);
          memset (lengths, 5, 30);
          huffman_build (&dist, lengths, 30);
        }
      else if (type == 2)
        {
          // dynamic codes, their lengths are themselves huffman coded
          static const uint8_t order[19] = { 16, 17, 18, 0, 8,  7, 9,
                                             6,  10, 5,  11, 4, 12, 3,
                                             13, 2,  14, 1,  15 };
          int nlit = getbits (&in, 5) + 257;
          int ndist = getbits (&in, 5) + 1;
          int nlen = getbits (&in, 4) + 4;
          uint8_t len_lengths[19] = { 0 };
          for (int i = 0; i < nlen; i++)
            {
              len_lengths[order[i]] = getbits (&in, 3);
            }
          struct huffman lencode;
          huffman_build (&lencode, len_lengths, 19);
          for (int i = 0; i < nlit + ndist;)
            {
              int symbol = huffman_decode (&in, &lencode);
              int repeat, value = 0;
              if (symbol < 0)
                {
                  return -1;
                }
              if (symbol < 16)
                {
                  lengths[i++] = symbol;
                  continue;
                }
              if (symbol == 16)
                {
                  if (i == 0)
                    {
                      return -1;
                    }
                  value = lengths[i - 1];
                  repeat = 3 + getbits (&in, 2);
                }
              else if (symbol == 17)
                {
                  repeat = 3 + getbits (&in, 3);
                }
              else
                {
                  repeat = 11 + getbits (&in, 7);
                }
              if (i + repeat > nlit + ndist)
                {
                  return -1;
                }
              while (repeat--)
                {
                  lengths[i++] = value;
                }
            }
          huffman_build (&lit, lengths, nlit);
          huffman_build (&dist, lengths + nlit, ndist);
        }
      else
        {
          return -1;
        }
      while (1)
        {
          int symbol = huffman_decode (&in, &lit);
          if (symbol < 0)
            {
              return -1;
            }
          if (symbol < 256)
            {
              if (pos >= max)
                {
                  return -1;
                }
              out[pos++] = symbol;
              continue;
            }
          if (symbol == 256)
            {
              break;
            }
          symbol -= 257;
          if (symbol >= 29)
            {
              return -1;
            }
          uint32_t count = length_base[symbol]
                           + getbits (&in, length_extra[symbol]);
          int d = huffman_decode (&in, &dist);
          if (d < 0 || d >= 30)
            {
              return -1;
            }
          uint32_t back = dist_base[d] + getbits (&in, dist_extra[d]);
          if (back > pos || pos + count > max)
            {
              return -1;
            }
          for (uint32_t i = 0; i < count; i++, pos++)
            {
              out[pos] = out[pos - back];
            }
        }
    }
  while (!last);
  return pos;
}

static uint32_t
be32 (const uint8_t *p)
{
  return (uint32_t)p[0] << 24 | p[1] << 16 | p[2] << 8 | p[3];
}

static int
paeth (int a, int b, int c)
{
  int p = a + b - c;
  int pa = abs (p - a), pb = abs (p - b), pc = abs (p - c);
  if (pa <= pb && pa <= pc)
    {
      return a;
    }
  return pb <= pc ? b : c;
}

// undoes the per row filters in place, rows of stride bytes after a filter
// byte each
static int
unfilter (uint8_t *data, int height, int stride, int bpp)
{
  uint8_t *prev = 0;
  for (int y = 0; y < height; y++)
    {
      uint8_t *row = data + y * (stride + 1);
      int filter = row[0];
      row++;
      for (int x = 0; x < stride; x++)
        {
          int a = x >= bpp ? row[x - bpp] : 0;
          int b = prev ? prev[x] : 0;
          int c = prev && x >= bpp ? prev[x - bpp] : 0;
          switch (filter)
            {
            case 0:
              break;
            case 1:
              row[x] += a;
              break;
            case 2:
              row[x] += b;
              break;
            case 3:
              row[x] += (a + b) / 2;
              break;
            case 4:
              row[x] += paeth (a, b, c);
              break;
            default:
              return -1;
            }
        }
      prev = row;
    }
  return 0;
}

static uint8_t *
read_file (const char *path, uint32_t *len)
{
  FILE *file = fopen (path, "rb");
  if (!file)
    {
      return 0;
    }
  fseek (file, 0, SEEK_END);
  long size = ftell (file);
  fseek (file, 0, SEEK_SET);
  uint8_t *data = size > 0 ? malloc (size) : 0;
  if (data && fread (data, 1, size, file) != (size_t)size)
    {
      free (data);
      data = 0;
    }
  fclose (file);
  *len = size;
  return data;
}

SDL_Surface *
IMG_Load (const char *path)
{
  static const uint8_t signature[8] = { 137, 'P', 'N', 'G', 13, 10, 26, 10 };
  uint32_t len;
  uint8_t *file = read_file (path, &len);
  if (!file)
    {
      return 0;
    }
  SDL_Surface *surface = 0;
  uint8_t *idat = 0, *raw = 0;
  uint32_t idat_len = 0;
  int width = 0, height = 0, depth = 0, type = -1;
  if (len < 8 || memcmp (file, signature, 8))
    {
      goto done;
    }
  // the header, then all image data chunks joined
  for (uint32_t pos = 8; pos + 12 <= len;)
    {
      uint32_t size = be32 (file + pos);
      const uint8_t *chunk = file + pos + 8;
      if (size > len - pos - 12)
        {
          goto done;
        }
      if (!memcmp (file + pos + 4, "IHDR", 4) && size >= 13)
        {
          width = be32 (chunk);
          height = be32 (chunk + 4);
          depth = chunk[8];
          type = chunk[9];
          if (chunk[12])
            {
              // interlaced
              goto done;
            }
        }
      else if (!memcmp (file + pos + 4, "IDAT", 4))
        {
          uint8_t *more = realloc (idat, idat_len + size);
          if (!more)
            {
              goto done;
            }
          idat = more;
          memcpy (idat + idat_len, chunk, size);
          idat_len += size;
        }
      pos += size + 12;
    }
  int channels = type == 0 ? 1 : type == 2 ? 3 : type == 4 ? 2 : type == 6 ? 4 : 0;
  if (!channels || (depth != 8 && depth != 16) || width <= 0 || height <= 0)
    {
      goto done;
    }
  int bpp = channels * depth / 8;
  int stride = width * bpp;
  uint32_t raw_len = (uint32_t)(stride + 1) * height;
  raw = malloc (raw_len);
  if (!raw || inflate (idat, idat_len, raw, raw_len) != (int)raw_len
      || unfilter (raw, height, stride, bpp) < 0)
    {
      goto done;
    }
  // images without alpha get a colour key from xgalaga instead
  int alpha = channels == 2 || channels == 4;
  surface = SDL_CreateRGBSurface (alpha ? SDL_SRCALPHA : 0, width, height, 32,
                                  0x00FF0000, 0x0000FF00, 0x000000FF,
                                  alpha ? 0xFF000000 : 0);
  if (!surface)
    {
      goto done;
    }
  // 16 bit channels keep their high byte
  int step = depth / 8;
  for (int y = 0; y < height; y++)
    {
      const uint8_t *in = raw + y * (stride + 1) + 1;
      Uint32 *out = (Uint32 *)((uint8_t *)surface->pixels + y * surface->pitch);
      for (int x = 0; x < width; x++, in += bpp)
        {
          int r, g, b, a = 0;
          if (channels <= 2)
            {
              r = g = b = in[0];
              if (channels == 2)
                {
                  a = in[step];
                }
            }
          else
            {
              r = in[0];
              g = in[step];
              b = in[2 * step];
              if (channels == 4)
                {
                  a = in[3 * step];
                }
            }
          out[x] = (Uint32)a << 24 | r << 16 | g << 8 | b;
        }
    }
done:
  free (raw);
  free (idat);
  free (file);
  return surface;
}
