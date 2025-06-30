#include "../../drivers/keyboard/keyboard.h"
#include "../../drivers/screen/terminal.h"
#include "../../drivers/screen/vga.h"
#include "../../libc/ctype/ctype.h"
#include "../../libc/stdio/stdio.h"
#include "../../libc/string/string.h"
#include "../task/task.h"
#include "shell_internal.h"

#define HISTORY_SIZE 16

static char history[HISTORY_SIZE][SHELL_LINE_MAX];
// lines ever added, the newest is history[(history_count - 1) % HISTORY_SIZE]
static uint32_t history_count = 0;

struct editor
{
  char *buf;
  uint32_t max;
  uint32_t len;
  uint32_t cursor;
  // screen position of the first char, fixed up when the screen scrolls
  size_t start_row;
  size_t start_column;
  uint32_t start_scrolls;
};

static void
editor_start_here (struct editor *e)
{
  terminal_get_cursor (&e->start_row, &e->start_column);
  e->start_scrolls = terminal_scroll_count ();
}

// moves the terminal cursor to char idx of the line
static void
editor_move (struct editor *e, uint32_t idx)
{
  uint32_t linear = e->start_column + idx;
  int32_t row = e->start_row + linear / VGA_WIDTH
                - (terminal_scroll_count () - e->start_scrolls);
  terminal_set_cursor (row < 0 ? 0 : row, linear % VGA_WIDTH);
}

// redraws the line from char from. chars up to old_len are blanked when
// the line got shorter
static void
editor_redraw (struct editor *e, uint32_t from, uint32_t old_len)
{
  editor_move (e, from);
  for (uint32_t i = from; i < e->len; i++)
    {
      terminal_putchar (e->buf[i]);
    }
  for (uint32_t i = e->len; i < old_len; i++)
    {
      terminal_putchar (' ');
    }
  editor_move (e, e->cursor);
}

static void
editor_insert (struct editor *e, char c)
{
  if (e->len >= e->max - 1)
    {
      return;
    }
  memmove (e->buf + e->cursor + 1, e->buf + e->cursor, e->len - e->cursor);
  e->buf[e->cursor++] = c;
  e->len++;
  if (e->cursor == e->len)
    {
      // typing at the end of the line, no redraw needed
      terminal_putchar (c);
    }
  else
    {
      editor_redraw (e, e->cursor - 1, e->len);
    }
}

// removes the char at idx
static void
editor_remove (struct editor *e, uint32_t idx)
{
  memmove (e->buf + idx, e->buf + idx + 1, e->len - idx - 1);
  e->len--;
  if (e->cursor > idx)
    {
      e->cursor--;
    }
  if (idx == e->len && e->cursor == e->len)
    {
      terminal_putchar ('\b');
    }
  else
    {
      editor_redraw (e, idx, e->len + 1);
    }
}

static void
editor_set_line (struct editor *e, const char *text)
{
  uint32_t old_len = e->len;
  e->len = strlen (text);
  if (e->len > e->max - 1)
    {
      e->len = e->max - 1;
    }
  memcpy (e->buf, text, e->len);
  e->cursor = e->len;
  editor_redraw (e, 0, old_len);
}

static void
history_add (const char *line)
{
  if (!line[0])
    {
      return;
    }
  if (history_count > 0
      && strcmp (history[(history_count - 1) % HISTORY_SIZE], line) == 0)
    {
      return;
    }
  strcpy (history[history_count % HISTORY_SIZE], line);
  history_count++;
}

static void
editor_complete (struct editor *e)
{
  uint32_t word_start = e->cursor;
  while (word_start > 0 && e->buf[word_start - 1] != ' ')
    {
      word_start--;
    }
  uint32_t name_start = 0;
  char matches[SHELL_MAX_COMPLETIONS][SHELL_COMPLETION_LEN];
  int total = shell_complete (e->buf + word_start, e->cursor - word_start,
                              word_start == 0, &name_start, matches,
                              SHELL_MAX_COMPLETIONS);
  if (total == 0)
    {
      return;
    }
  uint32_t typed = e->cursor - word_start - name_start;

  // longest prefix shared by every match
  uint32_t common = strlen (matches[0]);
  for (int i = 1; i < total; i++)
    {
      uint32_t j = 0;
      while (j < common && matches[i][j]
             && tolower (matches[i][j]) == tolower (matches[0][j]))
        {
          j++;
        }
      common = j;
    }

  if (common > typed)
    {
      for (uint32_t i = typed; i < common; i++)
        {
          editor_insert (e, matches[0][i]);
        }
      // a single file or command is done, directories can go on
      if (total == 1 && matches[0][common - 1] != '/')
        {
          editor_insert (e, ' ');
        }
      return;
    }
  if (total == 1)
    {
      return;
    }

  // ambiguous, list the candidates and redraw the line below them
  editor_move (e, e->len);
  terminal_putchar ('\n');
  for (int i = 0; i < total; i++)
    {
      printf ("%s  ", matches[i]);
    }
  terminal_putchar ('\n');
  shell_print_prompt ();
  editor_start_here (e);
  editor_redraw (e, 0, 0);
}

void
shell_readline (char *line, uint32_t max)
{
  struct editor e = { .buf = line, .max = max, .len = 0, .cursor = 0 };
  editor_start_here (&e);
  // history position, history_count is the line being typed
  uint32_t history_pos = history_count;
  uint32_t history_oldest
      = history_count > HISTORY_SIZE ? history_count - HISTORY_SIZE : 0;
  char draft[SHELL_LINE_MAX];
  draft[0] = '\0';

  while (1)
    {
      int32_t key = task_wait_key ();
      switch (key)
        {
        case '\n':
          editor_move (&e, e.len);
          terminal_putchar ('\n');
          line[e.len] = '\0';
          history_add (line);
          return;
        case KEY_CTRL_C:
          editor_move (&e, e.len);
          printf ("^C\n");
          line[0] = '\0';
          return;
        case '\b':
          if (e.cursor > 0)
            {
              editor_remove (&e, e.cursor - 1);
            }
          break;
        case KEY_DELETE:
          if (e.cursor < e.len)
            {
              editor_remove (&e, e.cursor);
            }
          break;
        case KEY_LEFT:
          if (e.cursor > 0)
            {
              editor_move (&e, --e.cursor);
            }
          break;
        case KEY_RIGHT:
          if (e.cursor < e.len)
            {
              editor_move (&e, ++e.cursor);
            }
          break;
        case KEY_HOME:
          e.cursor = 0;
          editor_move (&e, 0);
          break;
        case KEY_END:
          e.cursor = e.len;
          editor_move (&e, e.len);
          break;
        case KEY_UP:
          if (history_pos > history_oldest)
            {
              if (history_pos == history_count)
                {
                  // keep what was typed so down can bring it back
                  line[e.len] = '\0';
                  strcpy (draft, line);
                }
              history_pos--;
              editor_set_line (&e, history[history_pos % HISTORY_SIZE]);
            }
          break;
        case KEY_DOWN:
          if (history_pos < history_count)
            {
              history_pos++;
              editor_set_line (&e, history_pos == history_count
                                       ? draft
                                       : history[history_pos % HISTORY_SIZE]);
            }
          break;
        case '\t':
          editor_complete (&e);
          break;
        default:
          if (key >= 0 && isprint (key))
            {
              editor_insert (&e, key);
            }
          break;
        }
    }
}
