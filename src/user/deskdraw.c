#include "desk.h"

// drawing for the desktop: boxes, patterns, pixel art and a proportional
// bitmap font on a 1 bit canvas

#define STRIDE (SCREEN_W / 8)

const uint8_t pattern_grey[8] = { 0xCC, 0x66, 0x33, 0x99,
                                  0xCC, 0x66, 0x33, 0x99 };
const uint8_t pattern_light[8] = { 0x88, 0x00, 0x22, 0x00,
                                   0x88, 0x00, 0x22, 0x00 };
const uint8_t pattern_stripes[8] = { 0xFF, 0x00, 0xFF, 0x00,
                                     0xFF, 0x00, 0xFF, 0x00 };

static uint8_t canvas[STRIDE * SCREEN_H];
static uint8_t frame[STRIDE * SCREEN_H];
static uint8_t *target = canvas;

static struct box clip = { 0, 0, SCREEN_W, SCREEN_H };

// glyphs for chars 32 to 126: rows from the top separated by spaces, '#' is
// ink. the width of the first row is the width of the glyph, missing rows
// at the bottom are empty. rows 7 and 8 hang below the baseline
static const char *const glyphs[] = {
  "...",                                                   // ' '
  "# # # # # . #",                                         // !
  "#.# #.#",                                               // "
  ".#.#. .#.#. ##### .#.#. ##### .#.#. .#.#.",             // #
  "..#.. .#### #.#.. .###. ..#.# ####. ..#..",             // $
  "##... ##..# ...#. ..#.. .#... #..## ...##",             // %
  ".#... #.#.. #.#.. .#... #.#.# #..#. .##.#",             // &
  "# #",                                                   // '
  "..# .#. #.. #.. #.. .#. ..#",                           // (
  "#.. .#. ..# ..# ..# .#. #..",                           // )
  "..... ..#.. #.#.# .###. #.#.# ..#..",                   // *
  "..... ..#.. ..#.. ##### ..#.. ..#..",                   // +
  ".. .. .. .. .. .# .# #.",                               // ,
  ".... .... .... ####",                                   // -
  ". . . . . . #",                                         // .
  "..... ....# ...#. ..#.. .#... #....",                   // /
  ".###. #...# #..## #.#.# ##..# #...# .###.",             // 0
  ".#. ##. .#. .#. .#. .#. ###",                           // 1
  ".###. #...# ....# ..##. .#... #.... #####",             // 2
  "##### ...#. ..#.. ...#. ....# #...# .###.",             // 3
  "...#. ..##. .#.#. #..#. ##### ...#. ...#.",             // 4
  "##### #.... ####. ....# ....# #...# .###.",             // 5
  "..##. .#... #.... ####. #...# #...# .###.",             // 6
  "##### ....# ...#. ..#.. .#... .#... .#...",             // 7
  ".###. #...# #...# .###. #...# #...# .###.",             // 8
  ".###. #...# #...# .#### ....# ...#. .##..",             // 9
  ". . # . . # .",                                         // :
  ".. .. .# .. .. .# .# #.",                               // ;
  "...# ..#. .#.. #... .#.. ..#. ...#",                    // <
  ".... .... #### .... ####",                              // =
  "#... .#.. ..#. ...# ..#. .#.. #...",                    // >
  ".###. #...# ....# ...#. ..#.. ..... ..#..",             // ?
  ".###. #...# #.### #.#.# #.### #.... .###.",             // @
  ".###. #...# #...# ##### #...# #...# #...#",             // A
  "####. #...# #...# ####. #...# #...# ####.",             // B
  ".###. #...# #.... #.... #.... #...# .###.",             // C
  "####. #...# #...# #...# #...# #...# ####.",             // D
  "##### #.... #.... ####. #.... #.... #####",             // E
  "##### #.... #.... ####. #.... #.... #....",             // F
  ".###. #...# #.... #.### #...# #...# .####",             // G
  "#...# #...# #...# ##### #...# #...# #...#",             // H
  "### .#. .#. .#. .#. .#. ###",                           // I
  "..### ...#. ...#. ...#. ...#. #..#. .##..",             // J
  "#...# #..#. #.#.. ##... #.#.. #..#. #...#",             // K
  "#.... #.... #.... #.... #.... #.... #####",             // L
  "#...# ##.## #.#.# #.#.# #...# #...# #...#",             // M
  "#...# #...# ##..# #.#.# #..## #...# #...#",             // N
  ".###. #...# #...# #...# #...# #...# .###.",             // O
  "####. #...# #...# ####. #.... #.... #....",             // P
  ".###. #...# #...# #...# #.#.# #..#. .##.#",             // Q
  "####. #...# #...# ####. #.#.. #..#. #...#",             // R
  ".###. #...# #.... .###. ....# #...# .###.",             // S
  "##### ..#.. ..#.. ..#.. ..#.. ..#.. ..#..",             // T
  "#...# #...# #...# #...# #...# #...# .###.",             // U
  "#...# #...# #...# #...# #...# .#.#. ..#..",             // V
  "#...# #...# #...# #.#.# #.#.# #.#.# .#.#.",             // W
  "#...# #...# .#.#. ..#.. .#.#. #...# #...#",             // X
  "#...# #...# .#.#. ..#.. ..#.. ..#.. ..#..",             // Y
  "##### ....# ...#. ..#.. .#... #.... #####",             // Z
  "### #.. #.. #.. #.. #.. ###",                           // [
  "..... #.... .#... ..#.. ...#. ....#",                   // backslash
  "### ..# ..# ..# ..# ..# ###",                           // ]
  "..#.. .#.#. #...#",                                     // ^
  "..... ..... ..... ..... ..... ..... ..... #####",       // _
  "#. .#",                                                 // `
  "..... ..... .###. ....# .#### #...# .####",             // a
  "#.... #.... ####. #...# #...# #...# ####.",             // b
  ".... .... .### #... #... #... .###",                    // c
  "....# ....# .#### #...# #...# #...# .####",             // d
  "..... ..... .###. #...# ##### #.... .###.",             // e
  "..## .#.. #### .#.. .#.. .#.. .#..",                    // f
  "..... ..... .#### #...# #...# #...# .#### ....# .###.", // g
  "#.... #.... ####. #...# #...# #...# #...#",             // h
  "# . # # # # #",                                         // i
  "..# ... ..# ..# ..# ..# ..# #.# .#.",                   // j
  "#... #... #..# #.#. ##.. #.#. #..#",                    // k
  "# # # # # # #",                                         // l
  "..... ..... ##.#. #.#.# #.#.# #.#.# #.#.#",             // m
  "..... ..... ####. #...# #...# #...# #...#",             // n
  "..... ..... .###. #...# #...# #...# .###.",             // o
  "..... ..... ####. #...# #...# #...# ####. #.... #....", // p
  "..... ..... .#### #...# #...# #...# .#### ....# ....#", // q
  ".... .... #.## ##.. #... #... #...",                    // r
  "..... ..... .#### #.... .###. ....# ####.",             // s
  ".#.. .#.. #### .#.. .#.. .#.. ..##",                    // t
  "..... ..... #...# #...# #...# #...# .####",             // u
  "..... ..... #...# #...# #...# .#.#. ..#..",             // v
  "..... ..... #...# #...# #.#.# #.#.# .#.#.",             // w
  "..... ..... #...# .#.#. ..#.. .#.#. #...#",             // x
  "..... ..... #...# #...# #...# #...# .#### ....# .###.", // y
  "..... ..... ##### ...#. ..#.. .#... #####",             // z
  "..# .#. .#. #.. .#. .#. ..#",                           // {
  "# # # # # # # # #",                                     // |
  "#.. .#. .#. ..# .#. .#. #..",                           // }
  "..... ..... .#..# #.##.",                               // ~
};

#define GLYPH_FIRST 32
#define GLYPH_COUNT (sizeof (glyphs) / sizeof (glyphs[0]))

// glyph rows as bits, the leftmost pixel in bit 7
static uint8_t font_rows[GLYPH_COUNT][FONT_HEIGHT];
static uint8_t font_widths[GLYPH_COUNT];
static int font_ready = 0;

static void
font_init (void)
{
  for (uint32_t i = 0; i < GLYPH_COUNT; i++)
    {
      const char *p = glyphs[i];
      int row = 0;
      int column = 0;
      font_widths[i] = 0;
      for (; *p; p++)
        {
          if (*p == ' ')
            {
              row++;
              column = 0;
              continue;
            }
          if (row == 0)
            {
              font_widths[i]++;
            }
          if (*p == '#' && row < FONT_HEIGHT)
            {
              font_rows[i][row] |= 0x80 >> column;
            }
          column++;
        }
    }
  font_ready = 1;
}

static int
glyph_index (char c)
{
  if (!font_ready)
    {
      font_init ();
    }
  if (c < GLYPH_FIRST || c >= GLYPH_FIRST + (int)GLYPH_COUNT)
    {
      c = '?';
    }
  return c - GLYPH_FIRST;
}

void
draw_to_frame (int to_frame)
{
  target = to_frame ? frame : canvas;
}

void
draw_clip (struct box box)
{
  // never outside the screen
  int x1 = box.x + box.w;
  int y1 = box.y + box.h;
  clip.x = box.x < 0 ? 0 : box.x;
  clip.y = box.y < 0 ? 0 : box.y;
  clip.w = (x1 > SCREEN_W ? SCREEN_W : x1) - clip.x;
  clip.h = (y1 > SCREEN_H ? SCREEN_H : y1) - clip.y;
}

void
draw_unclip (void)
{
  clip = (struct box){ 0, 0, SCREEN_W, SCREEN_H };
}

int
box_contains (struct box box, int x, int y)
{
  return x >= box.x && y >= box.y && x < box.x + box.w && y < box.y + box.h;
}

// the pixels x0 to x1 (excluded) of row y become bits, or are flipped when
// invert is set
static void
span (int y, int x0, int x1, uint8_t bits, int invert)
{
  if (y < clip.y || y >= clip.y + clip.h)
    {
      return;
    }
  if (x0 < clip.x)
    {
      x0 = clip.x;
    }
  if (x1 > clip.x + clip.w)
    {
      x1 = clip.x + clip.w;
    }
  uint8_t *row = target + y * STRIDE;
  while (x0 < x1)
    {
      int byte = x0 / 8;
      int first = x0 % 8;
      int last = (byte + 1) * 8 < x1 ? 8 : x1 - byte * 8;
      uint8_t mask = (uint8_t)(0xFF >> first) & (uint8_t)(0xFF << (8 - last));
      if (invert)
        {
          row[byte] ^= mask;
        }
      else
        {
          row[byte] = (row[byte] & ~mask) | (bits & mask);
        }
      x0 = (byte + 1) * 8;
    }
}

void
fill_box (struct box box, int colour)
{
  for (int y = box.y; y < box.y + box.h; y++)
    {
      span (y, box.x, box.x + box.w, colour ? 0xFF : 0x00, 0);
    }
}

// patterns stay put on the screen, so neighbouring boxes line up
void
fill_pattern (struct box box, const uint8_t pattern[8])
{
  for (int y = box.y; y < box.y + box.h; y++)
    {
      span (y, box.x, box.x + box.w, pattern[y & 7], 0);
    }
}

void
invert_box (struct box box)
{
  for (int y = box.y; y < box.y + box.h; y++)
    {
      span (y, box.x, box.x + box.w, 0, 1);
    }
}

void
hline (int x, int y, int w, int colour)
{
  span (y, x, x + w, colour ? 0xFF : 0x00, 0);
}

void
vline (int x, int y, int h, int colour)
{
  for (int i = y; i < y + h; i++)
    {
      span (i, x, x + 1, colour ? 0xFF : 0x00, 0);
    }
}

void
frame_box (struct box box)
{
  hline (box.x, box.y, box.w, INK);
  hline (box.x, box.y + box.h - 1, box.w, INK);
  vline (box.x, box.y, box.h, INK);
  vline (box.x + box.w - 1, box.y, box.h, INK);
}

void
put_pixel (int x, int y, int colour)
{
  span (y, x, x + 1, colour ? 0xFF : 0x00, 0);
}

static void
flip_pixel (int x, int y)
{
  span (y, x, x + 1, 0, 1);
}

void
dotted_frame (struct box box)
{
  int x1 = box.x + box.w - 1;
  int y1 = box.y + box.h - 1;
  for (int x = box.x; x <= x1; x += 2)
    {
      flip_pixel (x, box.y);
      flip_pixel (x, y1);
    }
  for (int y = box.y + 1; y < y1; y += 2)
    {
      flip_pixel (box.x, y);
      flip_pixel (x1, y);
    }
}

void
draw_art (int x, int y, const char *const *rows, int inverted)
{
  for (int r = 0; rows[r]; r++)
    {
      for (int c = 0; rows[r][c]; c++)
        {
          char p = rows[r][c];
          if (p == '#' || p == '.')
            {
              put_pixel (x + c, y + r, (p == '#') ^ (inverted != 0));
            }
        }
    }
}

int
art_width (const char *const *rows)
{
  int w = 0;
  for (int r = 0; rows[r]; r++)
    {
      int len = strlen (rows[r]);
      w = len > w ? len : w;
    }
  return w;
}

int
art_height (const char *const *rows)
{
  int h = 0;
  while (rows[h])
    {
      h++;
    }
  return h;
}

static int
draw_glyph (int x, int y, char c, int colour)
{
  int g = glyph_index (c);
  for (int r = 0; r < FONT_HEIGHT; r++)
    {
      uint8_t bits = font_rows[g][r];
      for (int i = 0; bits; i++, bits <<= 1)
        {
          if (bits & 0x80)
            {
              put_pixel (x + i, y + r, colour);
            }
        }
    }
  return font_widths[g] + 1;
}

int
draw_text (int x, int y, const char *text, int colour)
{
  int start = x;
  for (; *text; text++)
    {
      x += draw_glyph (x, y, *text, colour);
    }
  return x - start;
}

int
text_width (const char *text)
{
  int w = 0;
  for (; *text; text++)
    {
      w += font_widths[glyph_index (*text)] + 1;
    }
  return w;
}

void
draw_text_fit (int x, int y, const char *text, int max_w, int colour)
{
  if (text_width (text) <= max_w)
    {
      draw_text (x, y, text, colour);
      return;
    }
  int dots = text_width ("...");
  int w = 0;
  for (; *text; text++)
    {
      int g = font_widths[glyph_index (*text)] + 1;
      if (w + g + dots > max_w)
        {
          break;
        }
      w += draw_glyph (x + w, y, *text, colour);
    }
  draw_text (x + w, y, "...", colour);
}

void
frame_from_canvas (void)
{
  memcpy (frame, canvas, sizeof (frame));
}

void
show_frame (void)
{
  gfx_blit (frame);
}
