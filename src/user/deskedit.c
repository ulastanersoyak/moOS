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

// in normal mode the cursor rests on a char, not on the end of a line,
// unless the line is empty. in insert mode it sits between chars
static void
clamp_cursor (struct editor *ed)
{
  ed->cursor = min (max (ed->cursor, 0), ed->len);
  if (ed->mode == EDIT_NORMAL && ed->cursor > line_start (ed, ed->cursor)
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

// 0 for blanks, 1 for letters, digits and _, 2 for other chars. vim's
// words are runs of the same class
static int
char_class (char c)
{
  if (c == ' ' || c == '\n')
    {
      return 0;
    }
  if (isalpha (c) || isdigit (c) || c == '_')
    {
      return 1;
    }
  return 2;
}

// w: the start of the next word
static void
word_forward (struct editor *ed)
{
  int pos = ed->cursor;
  int c = char_class (ed->text[pos]);
  while (c && pos < ed->len && char_class (ed->text[pos]) == c)
    {
      pos++;
    }
  while (pos < ed->len && char_class (ed->text[pos]) == 0)
    {
      pos++;
    }
  ed->cursor = min (pos, max (ed->len - 1, 0));
}

// b: the start of this word or the one before
static void
word_back (struct editor *ed)
{
  int pos = ed->cursor - 1;
  while (pos > 0 && char_class (ed->text[pos]) == 0)
    {
      pos--;
    }
  int c = char_class (ed->text[max (pos, 0)]);
  while (pos > 0 && char_class (ed->text[pos - 1]) == c)
    {
      pos--;
    }
  ed->cursor = max (pos, 0);
}

// e: the end of this word or the next one
static void
word_end (struct editor *ed)
{
  int pos = ed->cursor + 1;
  while (pos < ed->len && char_class (ed->text[pos]) == 0)
    {
      pos++;
    }
  int c = char_class (ed->text[min (pos, ed->len - 1)]);
  while (pos + 1 < ed->len && char_class (ed->text[pos + 1]) == c)
    {
      pos++;
    }
  ed->cursor = min (pos, max (ed->len - 1, 0));
}

// the first char of the line that isnt a space
static int
first_nonblank (const struct editor *ed, int pos)
{
  pos = line_start (ed, pos);
  while (pos < ed->len && ed->text[pos] == ' ')
    {
      pos++;
    }
  return pos;
}

static void
goto_line (struct editor *ed, int line)
{
  ed->cursor = first_nonblank (ed, line_pos (ed, line));
  clamp_cursor (ed);
  set_column (ed);
}

// puts text at pos, returns 0 when the buffer is full
static int
insert_text (struct editor *ed, int pos, const char *text, int len)
{
  if (ed->len + len > EDIT_MAX)
    {
      snprintf (ed->message, sizeof (ed->message), "the file is too big");
      return 0;
    }
  memmove (ed->text + pos + len, ed->text + pos, ed->len - pos);
  memcpy (ed->text + pos, text, len);
  ed->len += len;
  ed->modified = 1;
  return 1;
}

static void
delete_text (struct editor *ed, int from, int to)
{
  memmove (ed->text + from, ed->text + to, ed->len - to);
  ed->len -= to - from;
  ed->modified = 1;
}

static void
start_insert (struct editor *ed, int pos)
{
  ed->mode = EDIT_INSERT;
  ed->cursor = pos;
}

void
edit_load (struct editor *ed, const char *data, int len)
{
  memset (ed, 0, sizeof (*ed));
  len = min (max (len, 0), EDIT_MAX);
  // like vim the last line ends the file, saving puts its '\n' back
  if (len && data[len - 1] == '\n')
    {
      len--;
    }
  for (int i = 0; i < len; i++)
    {
      char c = data[i];
      // tabs and other control chars have no glyphs
      ed->text[i] = c == '\n' || (c >= ' ' && c <= '~') ? c : ' ';
    }
  ed->len = len;
  snprintf (ed->message, sizeof (ed->message), "%d lines, %d bytes",
            len ? edit_line_count (ed) : 0, len + (len > 0));
}

// a key in normal mode. motions take a count typed before them
static void
normal_key (struct editor *ed, int key)
{
  if ((key >= '1' && key <= '9') || (key == '0' && ed->count))
    {
      ed->count = min (ed->count * 10 + key - '0', 99999);
      return;
    }
  int count = ed->count;
  int n = max (count, 1);
  ed->count = 0;
  if (ed->pending == 'g')
    {
      ed->pending = 0;
      if (key == 'g')
        {
          goto_line (ed, count ? count - 1 : 0);
        }
      return;
    }
  switch (key)
    {
    case 'h':
    case KEY_LEFT:
      ed->cursor = max (ed->cursor - n, line_start (ed, ed->cursor));
      set_column (ed);
      break;
    case 'l':
    case KEY_RIGHT:
      ed->cursor = min (ed->cursor + n, line_end (ed, ed->cursor));
      clamp_cursor (ed);
      set_column (ed);
      break;
    case 'j':
    case KEY_DOWN:
      move_lines (ed, n);
      break;
    case 'k':
    case KEY_UP:
      move_lines (ed, -n);
      break;
    case KEY_PAGE_DOWN:
      move_lines (ed, 20 * n);
      break;
    case KEY_PAGE_UP:
      move_lines (ed, -20 * n);
      break;
    case '0':
    case KEY_HOME:
      ed->cursor = line_start (ed, ed->cursor);
      set_column (ed);
      break;
    case '^':
      ed->cursor = first_nonblank (ed, ed->cursor);
      clamp_cursor (ed);
      set_column (ed);
      break;
    case '$':
    case KEY_END:
      move_lines (ed, n - 1);
      ed->cursor = line_end (ed, ed->cursor);
      clamp_cursor (ed);
      ed->column = EDIT_MAX;
      break;
    case 'w':
    case 'b':
    case 'e':
      for (int i = 0; i < n; i++)
        {
          if (key == 'w')
            {
              word_forward (ed);
            }
          else if (key == 'b')
            {
              word_back (ed);
            }
          else
            {
              word_end (ed);
            }
        }
      clamp_cursor (ed);
      set_column (ed);
      break;
    case 'g':
      ed->pending = 'g';
      ed->count = count;
      break;
    case 'G':
      goto_line (ed, count ? count - 1 : edit_line_count (ed) - 1);
      break;
    case 'i':
      start_insert (ed, ed->cursor);
      break;
    case 'a':
      start_insert (ed, min (ed->cursor + 1, line_end (ed, ed->cursor)));
      break;
    case 'I':
      start_insert (ed, first_nonblank (ed, ed->cursor));
      break;
    case 'A':
      start_insert (ed, line_end (ed, ed->cursor));
      break;
    case 'o':
      {
        // a new line below, or the first one of an empty file
        int end = line_end (ed, ed->cursor);
        if (insert_text (ed, end, "\n", 1))
          {
            start_insert (ed, end + 1);
          }
        break;
      }
    case 'O':
      {
        int start = line_start (ed, ed->cursor);
        if (insert_text (ed, start, "\n", 1))
          {
            start_insert (ed, start);
          }
        break;
      }
    }
}

// a key in insert mode: chars go into the text until esc
static void
insert_key (struct editor *ed, int key)
{
  switch (key)
    {
    case 27:
      // like vim the cursor steps back onto the last typed char
      ed->mode = EDIT_NORMAL;
      if (ed->cursor > line_start (ed, ed->cursor))
        {
          ed->cursor--;
        }
      clamp_cursor (ed);
      set_column (ed);
      return;
    case '\b':
      if (ed->cursor > 0)
        {
          delete_text (ed, ed->cursor - 1, ed->cursor);
          ed->cursor--;
        }
      set_column (ed);
      return;
    case KEY_DELETE:
      if (ed->cursor < ed->len)
        {
          delete_text (ed, ed->cursor, ed->cursor + 1);
        }
      return;
    case '\t':
      // the font has no tab, two spaces it is
      if (insert_text (ed, ed->cursor, "  ", 2))
        {
          ed->cursor += 2;
        }
      set_column (ed);
      return;
    case KEY_LEFT:
    case KEY_RIGHT:
    case KEY_UP:
    case KEY_DOWN:
    case KEY_HOME:
    case KEY_END:
    case KEY_PAGE_UP:
    case KEY_PAGE_DOWN:
      normal_key (ed, key);
      return;
    }
  if (key == '\n' || (key >= ' ' && key <= '~'))
    {
      char c = key;
      if (insert_text (ed, ed->cursor, &c, 1))
        {
          ed->cursor++;
        }
      set_column (ed);
    }
}

int
edit_key (struct editor *ed, int key)
{
  ed->message[0] = 0;
  if (ed->mode == EDIT_INSERT)
    {
      insert_key (ed, key);
    }
  else
    {
      normal_key (ed, key);
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
              // a block over the char under the cursor, a bar between
              // chars while inserting
              int x = text.x + TEXT_LEFT;
              for (int i = pos; i < ed->cursor; i++)
                {
                  x += char_width (ed->text[i]);
                }
              int w = ed->cursor < end ? char_width (ed->text[ed->cursor])
                                       : char_width (' ');
              if (ed->mode == EDIT_INSERT)
                {
                  w = 1;
                }
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
  const char *status = ed->message;
  if (!status[0] && ed->mode == EDIT_INSERT)
    {
      status = "-- INSERT --";
    }
  draw_text (view.x + TEXT_LEFT, sy + 2, status, INK);
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
