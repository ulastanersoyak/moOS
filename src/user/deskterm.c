#include "desk.h"

// the terminal of the desktop's shell windows: a grid of chars that keeps
// the lines scrolled off the top, written to like the text screen

#define TEXT_LEFT 4
#define TEXT_TOP 2

static int
min (int a, int b)
{
  return a < b ? a : b;
}

static int
max (int a, int b)
{
  return a > b ? a : b;
}

void
term_clear (struct terminal *t)
{
  memset (t->cells, ' ', sizeof (t->cells));
  t->row = 0;
  t->column = 0;
}

static void
newline (struct terminal *t)
{
  t->column = 0;
  if (t->row < TERM_LINES - 1)
    {
      t->row++;
      return;
    }
  // the oldest line goes
  memmove (t->cells[0], t->cells[1], (TERM_LINES - 1) * TERM_COLUMNS);
  memset (t->cells[TERM_LINES - 1], ' ', TERM_COLUMNS);
}

void
term_write (struct terminal *t, const char *data, int len)
{
  for (int i = 0; i < len; i++)
    {
      char c = data[i];
      switch (c)
        {
        case '\n':
          newline (t);
          break;
        case '\r':
          t->column = 0;
          break;
        case '\b':
          // erases the char before, back onto the line above if needed
          if (t->column > 0)
            {
              t->column--;
            }
          else if (t->row > 0)
            {
              t->row--;
              t->column = TERM_COLUMNS - 1;
            }
          t->cells[t->row][t->column] = ' ';
          break;
        case '\f':
          term_clear (t);
          break;
        case '\t':
          t->column += 4;
          if (t->column >= TERM_COLUMNS)
            {
              newline (t);
            }
          break;
        default:
          if (c < ' ' || c > '~')
            {
              break;
            }
          t->cells[t->row][t->column] = c;
          if (++t->column == TERM_COLUMNS)
            {
              newline (t);
            }
          break;
        }
    }
}

int
term_height (const struct terminal *t)
{
  return TEXT_TOP * 2 + (t->row + 1) * TERM_ROW_H;
}

int
term_width (void)
{
  return TEXT_LEFT * 2 + TERM_COLUMNS * TERM_CELL_W;
}

void
term_draw (const struct terminal *t, struct box view, int scroll, int active)
{
  draw_clip (view);
  int first = max ((scroll - TEXT_TOP) / TERM_ROW_H, 0);
  int last = min ((scroll + view.h) / TERM_ROW_H + 1, t->row);
  for (int r = first; r <= last; r++)
    {
      int y = view.y + TEXT_TOP + r * TERM_ROW_H - scroll;
      for (int c = 0; c < TERM_COLUMNS; c++)
        {
          char str[2] = { t->cells[r][c], 0 };
          if (str[0] == ' ')
            {
              continue;
            }
          // the font is proportional, every char sits in the middle of
          // its cell
          int x = view.x + TEXT_LEFT + c * TERM_CELL_W;
          draw_text (x + (TERM_CELL_W - text_width (str)) / 2, y, str, INK);
        }
    }
  if (active)
    {
      int x = view.x + TEXT_LEFT + t->column * TERM_CELL_W;
      int y = view.y + TEXT_TOP + t->row * TERM_ROW_H - scroll;
      invert_box ((struct box){ x, y - 1, TERM_CELL_W, TERM_ROW_H });
    }
  draw_unclip ();
}
