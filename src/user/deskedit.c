#include "desk.h"

// the text editor of the desktop windows: a buffer with a cursor, drawn
// with the desktop font

#define TEXT_LEFT 4 // pixels between the window edge and the text
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

// first char of the line pos is in
static int
line_start (const struct editor *ed, int pos)
{
  while (pos > 0 && ed->text[pos - 1] != '\n')
    {
      pos--;
    }
  return pos;
}

// the '\n' ending the line pos is in, or the end of the text
static int
line_end (const struct editor *ed, int pos)
{
  while (pos < ed->len && ed->text[pos] != '\n')
    {
      pos++;
    }
  return pos;
}

// first char of line n, counted from 0. past the last line it is the last
static int
line_pos (const struct editor *ed, int n)
{
  int pos = 0;
  for (; n > 0; n--)
    {
      int end = line_end (ed, pos);
      if (end == ed->len)
        {
          break;
        }
      pos = end + 1;
    }
  return pos;
}

int
edit_line_count (const struct editor *ed)
{
  int lines = 1;
  for (int i = 0; i < ed->len; i++)
    {
      lines += ed->text[i] == '\n';
    }
  return lines;
}

int
edit_cursor_line (const struct editor *ed)
{
  int line = 0;
  for (int i = 0; i < ed->cursor; i++)
    {
      line += ed->text[i] == '\n';
    }
  return line;
}

static int
char_width (char c)
{
  char str[2] = { c, 0 };
  return text_width (str);
}

// the cursor rests on a char, not on the end of a line, unless it is empty
static void
clamp_cursor (struct editor *ed)
{
  ed->cursor = min (max (ed->cursor, 0), ed->len);
  if (ed->cursor > line_start (ed, ed->cursor)
      && ed->cursor == line_end (ed, ed->cursor))
    {
      ed->cursor--;
    }
}

static void
set_column (struct editor *ed)
{
  ed->column = ed->cursor - line_start (ed, ed->cursor);
}

// up or down by lines, keeping the column the cursor had
static void
move_lines (struct editor *ed, int delta)
{
  int line = max (edit_cursor_line (ed) + delta, 0);
  int start = line_pos (ed, line);
  ed->cursor = min (start + ed->column, line_end (ed, start));
  clamp_cursor (ed);
}

void
edit_load (struct editor *ed, const char *data, int len)
{
  memset (ed, 0, sizeof (*ed));
  len = min (max (len, 0), EDIT_MAX);
  for (int i = 0; i < len; i++)
    {
      char c = data[i];
      // tabs and other control chars have no glyphs
      ed->text[i] = c == '\n' || (c >= ' ' && c <= '~') ? c : ' ';
    }
  ed->len = len;
  snprintf (ed->message, sizeof (ed->message), "%d lines, %d bytes",
            edit_line_count (ed) - (len && data[len - 1] == '\n'), len);
}

int
edit_key (struct editor *ed, int key)
{
  ed->message[0] = 0;
  switch (key)
    {
    case KEY_LEFT:
      if (ed->cursor > line_start (ed, ed->cursor))
        {
          ed->cursor--;
        }
      set_column (ed);
      break;
    case KEY_RIGHT:
      if (ed->cursor < line_end (ed, ed->cursor))
        {
          ed->cursor++;
        }
      clamp_cursor (ed);
      set_column (ed);
      break;
    case KEY_UP:
      move_lines (ed, -1);
      break;
    case KEY_DOWN:
      move_lines (ed, 1);
      break;
    case KEY_HOME:
      ed->cursor = line_start (ed, ed->cursor);
      set_column (ed);
      break;
    case KEY_END:
      ed->cursor = line_end (ed, ed->cursor);
      clamp_cursor (ed);
      ed->column = EDIT_MAX;
      break;
    }
  return EDIT_DONE;
}

void
edit_click (struct editor *ed, struct box view, int scroll, int x, int y)
{
  int line = max ((y - view.y - TEXT_TOP + scroll) / FONT_HEIGHT, 0);
  if (line >= edit_line_count (ed))
    {
      line = edit_line_count (ed) - 1;
    }
  int pos = line_pos (ed, line);
  int end = line_end (ed, pos);
  int left = view.x + TEXT_LEFT;
  while (pos < end && left + char_width (ed->text[pos]) / 2 < x)
    {
      left += char_width (ed->text[pos]);
      pos++;
    }
  ed->cursor = pos;
  clamp_cursor (ed);
  set_column (ed);
  ed->message[0] = 0;
}

void
edit_draw (const struct editor *ed, struct box view, int scroll, int active)
{
  struct box text = { view.x, view.y, view.w, view.h - EDIT_STATUS_H };
  draw_clip (text);
  int cursor_line = edit_cursor_line (ed);
  int pos = 0;
  for (int line = 0;; line++)
    {
      int y = text.y + TEXT_TOP - scroll + line * FONT_HEIGHT;
      int end = line_end (ed, pos);
      if (y >= text.y + text.h)
        {
          break;
        }
      if (y + FONT_HEIGHT > text.y)
        {
          char buf[160];
          int len = min (end - pos, (int)sizeof (buf) - 1);
          memcpy (buf, ed->text + pos, len);
          buf[len] = 0;
          draw_text (text.x + TEXT_LEFT, y, buf, INK);
          if (active && line == cursor_line)
            {
              // a block over the char under the cursor
              int x = text.x + TEXT_LEFT;
              for (int i = pos; i < ed->cursor; i++)
                {
                  x += char_width (ed->text[i]);
                }
              int w = ed->cursor < end ? char_width (ed->text[ed->cursor])
                                       : char_width (' ');
              invert_box ((struct box){ x - 1, y - 1, w + 1, FONT_HEIGHT });
            }
        }
      if (end == ed->len)
        {
          break;
        }
      pos = end + 1;
    }
  // the status line: a message on the left, where the cursor is on the right
  draw_clip (view);
  int sy = text.y + text.h;
  fill_box ((struct box){ view.x, sy, view.w, EDIT_STATUS_H }, PAPER);
  hline (view.x, sy, view.w, INK);
  draw_text (view.x + TEXT_LEFT, sy + 2, ed->message, INK);
  char where[24];
  snprintf (where, sizeof (where), "%d,%d", cursor_line + 1,
            ed->cursor - line_start (ed, ed->cursor) + 1);
  draw_text (view.x + view.w - 8 - text_width (where), sy + 2, where, INK);
  draw_unclip ();
}

int
edit_cursor_y (const struct editor *ed)
{
  return TEXT_TOP + edit_cursor_line (ed) * FONT_HEIGHT;
}
