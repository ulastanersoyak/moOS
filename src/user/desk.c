#include "desk.h"

// a desktop in the style of apple ii desktop: a menu bar, the disk and the
// trash on a grey desktop, folder windows with icons, a text viewer, a
// calculator and a clock. it runs in the black and white graphics mode and
// is driven by the mouse

#define MENU_H 11   // menu bar with its bottom line
#define TITLE_H 11  // window title bar with its bottom line
#define HEADER_H 13 // item count above the icons of a folder window
#define SB_W 13     // scroll bar on the right of folder and text windows
#define ARROW_H 9   // arrow boxes and grow box of the scroll bar
#define CELL_W 84   // room for an icon and its name in a folder window
#define CELL_H 30
#define LIST_ROW 10 // rows of the by name view
#define MIN_W 140
#define MIN_H 60

#define MAX_WINDOWS 8
#define MAX_ENTRIES 128
#define PATH_LEN 192
#define DOUBLE_CLICK_MS 450
#define DRAG_START 3 // pixels the mouse moves before an icon gets dragged

enum
{
  WIN_FOLDER,
  WIN_TEXT,
  WIN_CALC,
  WIN_CLOCK,
};

struct window
{
  int used;
  int kind;
  struct box frame;
  char title[NAME_MAX + 1];
  char path[PATH_LEN];
  int by_name; // folder windows: list view instead of icons
  int scroll;  // pixels the content is scrolled up
  struct dirent *entries;
  int count;
  struct editor *edit; // text windows
};

static struct window windows[MAX_WINDOWS];
static struct dirent entry_store[MAX_WINDOWS][MAX_ENTRIES];
static struct editor edit_store[MAX_WINDOWS];
// window numbers from back to front
static int order[MAX_WINDOWS];
static int order_len = 0;

// the selected icon: in a window, or on the desktop when win is -1. index
// -1 means nothing is selected
struct target
{
  int win;
  int index;
};
static struct target selection = { -1, -1 };

enum
{
  DESK_DISK,
  DESK_TRASH,
  DESK_ICONS,
};

// pixel art, '#' is ink, '.' paper and ' ' leaves the background
static const char *const art_folder[] = {
  " ######               ",
  "#......#              ",
  "#.......############# ",
  "#....................#",
  "#....................#",
  "#....................#",
  "#....................#",
  "#....................#",
  "#....................#",
  "#....................#",
  "######################",
  0,
};

static const char *const art_document[] = {
  "##########    ",
  "#........##   ",
  "#........#.#  ",
  "#........#..# ",
  "#........#####",
  "#............#",
  "#..#######...#",
  "#............#",
  "#..########..#",
  "#............#",
  "##############",
  0,
};

static const char *const art_program[] = {
  "         #         ",
  "       ##.##       ",
  "     ##.....##     ",
  "   ##.........##   ",
  " ##.....###.....## ",
  "#.......###.......#",
  " ##.....###.....## ",
  "   ##.........##   ",
  "     ##.....##     ",
  "       ##.##       ",
  "         #         ",
  0,
};

static const char *const art_disk[] = {
  " ###################### ",
  "#......................#",
  "#......................#",
  "#......................#",
  "#......................#",
  "#.................###..#",
  "#......................#",
  " ###################### ",
  0,
};

static const char *const art_trash[] = {
  "     ####     ",
  "##############",
  "#............#",
  "##############",
  " #..#..#..#.# ",
  " #..#..#..#.# ",
  " #..#..#..#.# ",
  " #..#..#..#.# ",
  " #..#..#..#.# ",
  " #..#..#..#.# ",
  "  ##########  ",
  0,
};

// the moOS moose where the apple ii had its apple
static const char *const art_logo[] = {
  "# # #   # # #",
  "#####   #####",
  "   #######   ",
  "   #.###.#   ",
  "   #######   ",
  "    #...#    ",
  "    #.#.#    ",
  "     ###     ",
  0,
};

static const char *const art_pointer[] = {
  "#        ",
  "##       ",
  "#.#      ",
  "#..#     ",
  "#...#    ",
  "#....#   ",
  "#.....#  ",
  "#..#####  ",
  "#.#      ",
  "##       ",
  0,
};

static const char *const art_up[] = {
  "   #   ",
  "  #.#  ",
  " #...# ",
  "###.###",
  "  #.#  ",
  "  ###  ",
  0,
};

static const char *const art_down[] = {
  "  ###  ",
  "  #.#  ",
  "###.###",
  " #...# ",
  "  #.#  ",
  "   #   ",
  0,
};

static const char *const art_check[] = {
  "     #",
  "    # ",
  "#  #  ",
  " ##   ",
  " #    ",
  0,
};

// menus. "-" is a line between groups of items
#define MENU_ITEMS 10
struct menu
{
  const char *title;
  const char *items[MENU_ITEMS];
};

enum
{
  MENU_LOGO,
  MENU_FILE,
  MENU_VIEW,
  MENU_SPECIAL,
  MENU_SELECTOR,
  MENU_COUNT,
};

// items of the file menu
enum
{
  FILE_OPEN,
  FILE_NEW_FOLDER,
  FILE_NEW_FILE,
  FILE_CLOSE,
  FILE_CLOSE_ALL,
  FILE_INFO = 6,
  FILE_QUIT = 8,
};

static const struct menu menus[MENU_COUNT] = {
  { 0, { "About moOS DeskTop", "-", "Calculator", "Clock", 0 } },
  { "File",
    { "Open", "New Folder", "New File", "Close", "Close All", "-", "Get Info",
      "-", "Quit", 0 } },
  { "View", { "By Icon", "By Name", 0 } },
  { "Special", { "Delete", "-", "Restart", 0 } },
  { "Selector", { "Doom", "Gfx Demo", "Moose", 0 } },
};

// programs of the selector menu
static const char *const selector_paths[] = {
  "0:/bin/doom.elf",
  "0:/bin/gfxdemo.elf",
  "0:/bin/moose.elf",
};

// what the mouse is doing while a button is held
enum
{
  IDLE,
  MENU,        // a menu is pulled down
  DRAG_WINDOW, // moving the outline of a window
  GROW_WINDOW, // resizing the outline of a window
  DRAG_ICON,   // pressed on an icon, maybe moving it
  CLOSE_BOX,   // pressed on the close box of a window
  CALC_KEY,    // pressed on a key of the calculator
};

static int mode = IDLE;
static int mouse_x = SCREEN_W / 2;
static int mouse_y = SCREEN_H / 2;
static int mouse_y2 = SCREEN_H; // twice mouse_y, vertical moves count half
static uint8_t buttons = 0;

static int open_menu = -1;
static int menu_item = -1;   // the item under the mouse
static int menu_sticky = 0;  // stays down after a click on its title
static int track_win = -1;   // window being dragged, grown or closed
static int track_inside = 0; // the mouse is still on the pressed button
static int track_key = -1;   // calculator key being pressed
static int grab_x, grab_y;   // where the mouse grabbed it
static struct box outline;
static struct target drag_source;
static int drag_moved = 0;

static struct target last_click = { -2, -2 };
static uint32_t last_click_ms = 0;

static int shift_held = 0;
static int ctrl_held = 0;
static int dirty = 1; // the canvas has to be drawn again
static int quit = 0;

// a message box in the middle of the screen. while it is up nothing else
// takes clicks
enum
{
  ALERT_NONE,
  ALERT_INFO,   // just an ok button
  ALERT_DELETE, // ok deletes alert_path
};
static int alert = ALERT_NONE;
static char alert_lines[3][80];
static char alert_path[PATH_LEN];
static int alert_pressed = -1; // button held down

// the calculator
static struct
{
  double acc;
  double shown;
  char entry[16];
  int entering;
  char op;
  int error;
} calc;

static const char calc_keys[] = "CE=*789/456-123+0.";
#define CALC_KEY_W 16
#define CALC_KEY_H 9
#define CALC_GAP_X 5
#define CALC_GAP_Y 3
#define CALC_W (4 * CALC_KEY_W + 3 * CALC_GAP_X + 18)
#define CALC_H (TITLE_H + 6 + 11 + 5 + 5 * CALC_KEY_H + 4 * CALC_GAP_Y + 7)

static const char *const month_names[] = { "Jan", "Feb", "Mar", "Apr",
                                           "May", "Jun", "Jul", "Aug",
                                           "Sep", "Oct", "Nov", "Dec" };

// ---------------------------------------------------------------- helpers

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

static int
iabs (int a)
{
  return a < 0 ? -a : a;
}

static int
ends_with (const char *str, const char *suffix)
{
  uint32_t len = strlen (str);
  uint32_t slen = strlen (suffix);
  return len >= slen && strncasecmp (str + len - slen, suffix, slen) == 0;
}

static void
join_path (char *out, const char *dir, const char *name)
{
  uint32_t len = strlen (dir);
  snprintf (out, PATH_LEN, len && dir[len - 1] == '/' ? "%s%s" : "%s/%s",
            dir, name);
}

static void
format_date (char *out, uint32_t size, uint16_t date)
{
  int month = (date >> 5) & 0x0F;
  snprintf (out, size, "%2d %s %d", date & 0x1F,
            month >= 1 && month <= 12 ? month_names[month - 1] : "???",
            1980 + (date >> 9));
}

static const char *const *
entry_art (const struct dirent *entry)
{
  if (entry->is_dir)
    {
      return art_folder;
    }
  return ends_with (entry->name, ".elf") ? art_program : art_document;
}

// greys out drawn text or art by clearing every other pixel
static void
grey_out (struct box box)
{
  for (int y = box.y; y < box.y + box.h; y++)
    {
      for (int x = box.x + ((y + 1) & 1); x < box.x + box.w; x += 2)
        {
          put_pixel (x, y, PAPER);
        }
    }
}

// ---------------------------------------------------------------- windows

static struct window *
front_window (void)
{
  return order_len ? &windows[order[order_len - 1]] : 0;
}

static int
front_index (void)
{
  return order_len ? order[order_len - 1] : -1;
}

static void
bring_to_front (int w)
{
  int i = 0;
  while (i < order_len && order[i] != w)
    {
      i++;
    }
  for (; i < order_len - 1; i++)
    {
      order[i] = order[i + 1];
    }
  order[order_len - 1] = w;
  dirty = 1;
}

static int
scrollable (const struct window *win)
{
  return win->kind == WIN_FOLDER || win->kind == WIN_TEXT;
}

static int
header_h (const struct window *win)
{
  return win->kind == WIN_FOLDER ? HEADER_H : 0;
}

// the window without its frame and title bar
static struct box
win_body (const struct window *win)
{
  struct box f = win->frame;
  return (struct box){ f.x + 1, f.y + TITLE_H, f.w - 2, f.h - TITLE_H - 1 };
}

// where the content is drawn, below the header and left of the scroll bar
static struct box
win_view (const struct window *win)
{
  struct box b = win_body (win);
  int top = b.y + header_h (win);
  int w = scrollable (win) ? win->frame.w - SB_W - 1 : b.w;
  return (struct box){ b.x, top, w, b.y + b.h - top };
}

static struct box
win_scrollbar (const struct window *win)
{
  struct box f = win->frame;
  int top = f.y + TITLE_H + header_h (win);
  return (struct box){ f.x + f.w - SB_W, top, SB_W - 1, f.y + f.h - 1 - top };
}

static struct box
win_close_box (const struct window *win)
{
  return (struct box){ win->frame.x + 8, win->frame.y + 2, 11, TITLE_H - 4 };
}

static struct box
win_title_bar (const struct window *win)
{
  return (struct box){ win->frame.x, win->frame.y, win->frame.w, TITLE_H };
}

static int
folder_columns (const struct window *win)
{
  return max (1, win_view (win).w / CELL_W);
}

static int
content_height (const struct window *win)
{
  switch (win->kind)
    {
    case WIN_FOLDER:
      if (win->by_name)
        {
          return win->count * LIST_ROW + 4;
        }
      return (win->count + folder_columns (win) - 1) / folder_columns (win)
                 * CELL_H
             + 4;
    case WIN_TEXT:
      return edit_line_count (win->edit) * FONT_HEIGHT + 4 + EDIT_STATUS_H;
    }
  return 0;
}

static int
max_scroll (const struct window *win)
{
  return max (0, content_height (win) - win_view (win).h);
}

static void
scroll_by (struct window *win, int dy)
{
  win->scroll = min (max (win->scroll + dy, 0), max_scroll (win));
  dirty = 1;
}

// boxes of the icon art and the name of an item in a folder window
static void
item_boxes (const struct window *win, int i, struct box *icon,
            struct box *name)
{
  struct box view = win_view (win);
  const struct dirent *entry = &win->entries[i];
  if (win->by_name)
    {
      int y = view.y + 2 + i * LIST_ROW - win->scroll;
      *icon = (struct box){ view.x + 2, y, view.w - 4, LIST_ROW };
      *name = *icon;
      return;
    }
  int cols = folder_columns (win);
  int cx = view.x + (i % cols) * CELL_W;
  int cy = view.y + 4 + (i / cols) * CELL_H - win->scroll;
  const char *const *art = entry_art (entry);
  int aw = art_width (art);
  int ah = art_height (art);
  *icon = (struct box){ cx + (CELL_W - aw) / 2, cy + (11 - ah) / 2, aw, ah };
  int tw = min (text_width (entry->name), CELL_W - 6) + 4;
  *name = (struct box){ cx + (CELL_W - tw) / 2, cy + 13, tw, FONT_HEIGHT + 1 };
}

// scrolls a folder window so that item i can be seen
static void
reveal_item (struct window *win, int i)
{
  struct box view = win_view (win);
  struct box icon, name;
  item_boxes (win, i, &icon, &name);
  if (icon.y < view.y)
    {
      scroll_by (win, icon.y - view.y - 2);
    }
  else if (name.y + name.h > view.y + view.h)
    {
      scroll_by (win, name.y + name.h - view.y - view.h + 2);
    }
}

// scrolls a text window so that the line of the cursor can be seen
static void
reveal_cursor (struct window *win)
{
  int text_h = win_view (win).h - EDIT_STATUS_H;
  int y = edit_cursor_y (win->edit);
  if (y < win->scroll)
    {
      win->scroll = y - 2;
    }
  else if (y + FONT_HEIGHT > win->scroll + text_h)
    {
      win->scroll = y + FONT_HEIGHT - text_h;
    }
  scroll_by (win, 0);
}

static int
compare_entries (const struct dirent *a, const struct dirent *b)
{
  if (a->is_dir != b->is_dir)
    {
      return b->is_dir - a->is_dir;
    }
  const char *x = a->name;
  const char *y = b->name;
  while (*x && tolower (*x) == tolower (*y))
    {
      x++;
      y++;
    }
  return tolower (*x) - tolower (*y);
}

static void
load_folder (struct window *win)
{
  int n = listdir (win->path, win->entries, MAX_ENTRIES);
  win->count = n < 0 ? 0 : n;
  // folders first, then by name. insertion sort, folders are small
  for (int i = 1; i < win->count; i++)
    {
      struct dirent entry = win->entries[i];
      int j = i - 1;
      while (j >= 0 && compare_entries (&win->entries[j], &entry) > 0)
        {
          win->entries[j + 1] = win->entries[j];
          j--;
        }
      win->entries[j + 1] = entry;
    }
  win->scroll = min (win->scroll, max_scroll (win));
}

// reloads every window showing the folder, after something in it changed
static void
reload_path (const char *path)
{
  for (int i = 0; i < MAX_WINDOWS; i++)
    {
      if (windows[i].used && windows[i].kind == WIN_FOLDER
          && strcmp (windows[i].path, path) == 0)
        {
          load_folder (&windows[i]);
        }
    }
  if (selection.win >= 0 && windows[selection.win].used
      && strcmp (windows[selection.win].path, path) == 0)
    {
      selection.index = -1;
    }
  dirty = 1;
}

static int
find_window (int kind, const char *path)
{
  for (int i = 0; i < MAX_WINDOWS; i++)
    {
      if (windows[i].used && windows[i].kind == kind
          && strcmp (windows[i].path, path) == 0)
        {
          return i;
        }
    }
  return -1;
}

// a new window, staggered below the last one. -1 when there are too many
static int
new_window (int kind, const char *title, const char *path, int w, int h)
{
  int i = find_window (kind, path);
  if (i >= 0)
    {
      bring_to_front (i);
      return -1;
    }
  for (i = 0; i < MAX_WINDOWS && windows[i].used; i++)
    ;
  if (i == MAX_WINDOWS)
    {
      return -1;
    }
  struct window *win = &windows[i];
  memset (win, 0, sizeof (*win));
  win->used = 1;
  win->kind = kind;
  snprintf (win->title, sizeof (win->title), "%s", title);
  snprintf (win->path, sizeof (win->path), "%s", path);
  win->entries = entry_store[i];
  win->edit = &edit_store[i];
  w = min (w, SCREEN_W - 40);
  h = min (h, SCREEN_H - MENU_H - 12);
  win->frame = (struct box){ 24 + order_len * 18, MENU_H + 6 + order_len * 10,
                             w, h };
  if (win->frame.x + w > SCREEN_W - 8)
    {
      win->frame.x = SCREEN_W - 8 - w;
    }
  if (win->frame.y + h > SCREEN_H - 2)
    {
      win->frame.y = SCREEN_H - 2 - h;
    }
  order[order_len++] = i;
  dirty = 1;
  return i;
}

static void
close_window (int w)
{
  int i = 0;
  while (i < order_len && order[i] != w)
    {
      i++;
    }
  for (; i < order_len - 1; i++)
    {
      order[i] = order[i + 1];
    }
  order_len--;
  windows[w].used = 0;
  if (selection.win == w)
    {
      selection = (struct target){ -1, -1 };
    }
  dirty = 1;
}

static void
open_folder (const char *path, const char *title)
{
  struct dirent probe[1];
  if (listdir (path, probe, 1) < 0)
    {
      return;
    }
  int w = new_window (WIN_FOLDER, title, path, 5 * CELL_W + SB_W + 1,
                      TITLE_H + HEADER_H + 4 * CELL_H + 6);
  if (w < 0)
    {
      return;
    }
  load_folder (&windows[w]);
  // small folders get a small window
  struct window *win = &windows[w];
  int cols = min (5, max (3, win->count));
  win->frame.w = cols * CELL_W + SB_W + 1;
  int rows = (win->count + cols - 1) / cols;
  win->frame.h = min (win->frame.h,
                      TITLE_H + HEADER_H + max (1, rows) * CELL_H + 6);
}

static void
open_text (const char *path, const char *title)
{
  int fd = open (path, O_READ);
  if (fd < 0)
    {
      return;
    }
  int w = new_window (WIN_TEXT, title, path, 400, 150);
  if (w < 0)
    {
      close (fd);
      return;
    }
  static char data[EDIT_MAX];
  int len = read (fd, data, EDIT_MAX);
  close (fd);
  edit_load (windows[w].edit, data, len);
}

static void
open_calc (void)
{
  if (new_window (WIN_CALC, "Calc", "calc", CALC_W, CALC_H) >= 0)
    {
      memset (&calc, 0, sizeof (calc));
    }
}

static void
open_clock (void)
{
  new_window (WIN_CLOCK, "Clock", "clock", 130, TITLE_H + 30);
}

// ---------------------------------------------------------------- alerts

static void
show_alert (int kind, const char *a, const char *b, const char *c)
{
  alert = kind;
  snprintf (alert_lines[0], sizeof (alert_lines[0]), "%s", a);
  snprintf (alert_lines[1], sizeof (alert_lines[1]), "%s", b ? b : "");
  snprintf (alert_lines[2], sizeof (alert_lines[2]), "%s", c ? c : "");
  alert_pressed = -1;
  dirty = 1;
}

static struct box
alert_box (void)
{
  return (struct box){ SCREEN_W / 2 - 160, SCREEN_H / 2 - 36, 320, 72 };
}

// button 0 is ok, 1 is cancel
static struct box
alert_button (int i)
{
  struct box a = alert_box ();
  int x = alert == ALERT_DELETE ? a.x + a.w - 150 + i * 72 : a.x + a.w - 78;
  return (struct box){ x, a.y + a.h - 18, 60, 12 };
}

static void
draw_button (struct box box, const char *label, int pressed)
{
  fill_box (box, PAPER);
  frame_box (box);
  draw_text (box.x + (box.w - text_width (label)) / 2, box.y + 2, label, INK);
  if (pressed)
    {
      invert_box (box);
    }
}

static void
draw_alert (void)
{
  struct box a = alert_box ();
  fill_box (a, PAPER);
  frame_box (a);
  frame_box ((struct box){ a.x + 2, a.y + 1, a.w - 4, a.h - 2 });
  // the attention sign of apple ii desktop alerts
  static const char *const art_alert[] = {
    "    ##    ", "   #..#   ", "   #..#   ", "  #.##.#  ", "  #.##.#  ",
    " #..##..# ", " #..##..# ", "#........#", "#...##...#", "##########",
    0,
  };
  draw_art (a.x + 14, a.y + 10, art_alert, 0);
  for (int i = 0; i < 3; i++)
    {
      draw_text (a.x + 40, a.y + 8 + i * 11, alert_lines[i], INK);
    }
  draw_button (alert_button (0), "OK", alert_pressed == 0);
  // the default button has a second border
  struct box ok = alert_button (0);
  frame_box ((struct box){ ok.x - 2, ok.y - 1, ok.w + 4, ok.h + 2 });
  if (alert == ALERT_DELETE)
    {
      draw_button (alert_button (1), "Cancel", alert_pressed == 1);
    }
}

static void
alert_done (int ok)
{
  if (ok && alert == ALERT_DELETE)
    {
      if (remove (alert_path) < 0)
        {
          show_alert (ALERT_INFO, "The file could not be deleted.",
                      "Folders have to be empty first.", 0);
          return;
        }
      // reload the folder the file was in
      char dir[PATH_LEN];
      snprintf (dir, sizeof (dir), "%s", alert_path);
      char *slash = dir + strlen (dir);
      while (slash > dir && *slash != '/')
        {
          slash--;
        }
      slash[slash > dir && slash[-1] != ':' ? 0 : 1] = 0;
      selection = (struct target){ -1, -1 };
      reload_path (dir);
    }
  alert = ALERT_NONE;
  dirty = 1;
}

// ---------------------------------------------------------------- actions

static struct dirent *
selected_entry (void)
{
  if (selection.index < 0 || selection.win < 0)
    {
      return 0;
    }
  struct window *win = &windows[selection.win];
  return selection.index < win->count ? &win->entries[selection.index] : 0;
}

static void
run_program (const char *path)
{
  gfx_mode (GFX_TEXT);
  clear ();
  // argv[0] is the name without the folder and .elf
  char name[NAME_MAX + 1];
  const char *base = path;
  for (const char *p = path; *p; p++)
    {
      if (*p == '/')
        {
          base = p + 1;
        }
    }
  snprintf (name, sizeof (name), "%s", base);
  if (ends_with (name, ".elf"))
    {
      name[strlen (name) - 4] = 0;
    }
  char *argv[] = { name, 0 };
  int pid = spawn (path, argv, 0);
  if (pid < 0)
    {
      print_error (name, pid);
    }
  else
    {
      int code = wait (pid);
      printf ("\n[%s exited with %d]\n", name, code);
    }
  setcolor (LIGHT_GREY);
  print ("press a key to go back to the desktop\n");
  setcolor (WHITE);
  key_event (1);
  for (;;)
    {
      int key = key_event (0);
      if (key >= 0 && !(key & KEY_RELEASED))
        {
          break;
        }
      sleep (20);
    }
  gfx_mode (GFX_MONO);
  key_event (1);
  mouse_event (0, 1);
  buttons = 0;
  shift_held = ctrl_held = 0;
  mode = IDLE;
  dirty = 1;
}

static void
open_target (struct target t)
{
  if (t.index < 0)
    {
      return;
    }
  if (t.win < 0)
    {
      if (t.index == DESK_DISK)
        {
          open_folder ("0:/", "MOOS");
        }
      return;
    }
  struct window *win = &windows[t.win];
  if (t.index >= win->count)
    {
      return;
    }
  struct dirent *entry = &win->entries[t.index];
  char path[PATH_LEN];
  join_path (path, win->path, entry->name);
  if (entry->is_dir)
    {
      open_folder (path, entry->name);
    }
  else if (ends_with (entry->name, ".elf"))
    {
      run_program (path);
    }
  else
    {
      open_text (path, entry->name);
    }
}

static void
ask_delete (void)
{
  struct dirent *entry = selected_entry ();
  if (!entry)
    {
      return;
    }
  join_path (alert_path, windows[selection.win].path, entry->name);
  char line[80];
  snprintf (line, sizeof (line), "Delete \"%s\"?", entry->name);
  show_alert (ALERT_DELETE, line, "This can not be undone.", 0);
}

// makes an empty folder or file in the front folder window, named like
// "New Folder", "New Folder 2" or "Untitled.txt", "Untitled 2.txt"
static void
new_item (int folder)
{
  struct window *win = front_window ();
  if (!win || win->kind != WIN_FOLDER)
    {
      return;
    }
  const char *base = folder ? "New Folder" : "Untitled";
  const char *ext = folder ? "" : ".txt";
  char name[32];
  char path[PATH_LEN];
  for (int n = 1; n < 100; n++)
    {
      if (n == 1)
        {
          snprintf (name, sizeof (name), "%s%s", base, ext);
        }
      else
        {
          snprintf (name, sizeof (name), "%s %d%s", base, n, ext);
        }
      int taken = 0;
      for (int i = 0; i < win->count; i++)
        {
          taken |= strncasecmp (win->entries[i].name, name, NAME_MAX) == 0;
        }
      if (!taken)
        {
          break;
        }
    }
  join_path (path, win->path, name);
  int fd = folder ? mkdir (path) : open (path, O_WRITE);
  if (fd < 0)
    {
      show_alert (ALERT_INFO,
                  folder ? "The folder could not be made."
                         : "The file could not be made.",
                  0, 0);
      return;
    }
  if (!folder)
    {
      close (fd);
    }
  reload_path (win->path);
  // the new one is selected, ready to be opened
  for (int i = 0; i < win->count; i++)
    {
      if (strcmp (win->entries[i].name, name) == 0)
        {
          selection = (struct target){ win - windows, i };
          reveal_item (win, i);
        }
    }
}

static void
get_info (void)
{
  char a[80], b[80], c[80];
  if (selection.win < 0)
    {
      if (selection.index == DESK_DISK)
        {
          show_alert (ALERT_INFO, "MOOS", "fat16 volume, drive 0:", 0);
        }
      else if (selection.index == DESK_TRASH)
        {
          show_alert (ALERT_INFO, "Trash", "Drop a file here to delete it.",
                      0);
        }
      return;
    }
  struct dirent *entry = selected_entry ();
  if (!entry)
    {
      return;
    }
  char date[24];
  format_date (date, sizeof (date), entry->date);
  snprintf (a, sizeof (a), "%s", entry->name);
  if (entry->is_dir)
    {
      snprintf (b, sizeof (b), "Kind: folder");
    }
  else
    {
      snprintf (b, sizeof (b), "Kind: %s, %u bytes",
                ends_with (entry->name, ".elf") ? "program" : "file",
                entry->size);
    }
  snprintf (c, sizeof (c), "Modified: %s %02d:%02d", date, entry->time >> 11,
            (entry->time >> 5) & 0x3F);
  show_alert (ALERT_INFO, a, b, c);
}

static int
menu_item_enabled (int m, int i)
{
  struct window *front = front_window ();
  int folder_front = front && front->kind == WIN_FOLDER;
  switch (m)
    {
    case MENU_FILE:
      switch (i)
        {
        case FILE_OPEN:
          return selection.index >= 0
                 && !(selection.win < 0 && selection.index == DESK_TRASH);
        case FILE_NEW_FOLDER:
        case FILE_NEW_FILE:
          return folder_front;
        case FILE_CLOSE:
        case FILE_CLOSE_ALL:
          return front != 0;
        case FILE_INFO:
          return selection.index >= 0;
        }
      return 1;
    case MENU_VIEW:
      return folder_front;
    case MENU_SPECIAL:
      return i != 0 || selected_entry () != 0;
    }
  return 1;
}

static int
menu_item_checked (int m, int i)
{
  struct window *front = front_window ();
  return m == MENU_VIEW && front && front->kind == WIN_FOLDER
         && front->by_name == i;
}

static void
menu_action (int m, int i)
{
  if (!menu_item_enabled (m, i))
    {
      return;
    }
  struct window *front = front_window ();
  switch (m)
    {
    case MENU_LOGO:
      if (i == 0)
        {
          show_alert (ALERT_INFO, "moOS DeskTop",
                      "a desktop in the style of apple ii desktop",
                      "for moOS, a 32 bit hobby operating system");
        }
      else if (i == 2)
        {
          open_calc ();
        }
      else if (i == 3)
        {
          open_clock ();
        }
      break;
    case MENU_FILE:
      switch (i)
        {
        case FILE_OPEN:
          open_target (selection);
          break;
        case FILE_NEW_FOLDER:
        case FILE_NEW_FILE:
          new_item (i == FILE_NEW_FOLDER);
          break;
        case FILE_CLOSE:
          close_window (front_index ());
          break;
        case FILE_CLOSE_ALL:
          while (order_len)
            {
              close_window (order[0]);
            }
          break;
        case FILE_INFO:
          get_info ();
          break;
        case FILE_QUIT:
          quit = 1;
          break;
        }
      break;
    case MENU_VIEW:
      front->by_name = i;
      front->scroll = 0;
      break;
    case MENU_SPECIAL:
      if (i == 0)
        {
          ask_delete ();
        }
      else if (i == 2)
        {
          reboot ();
        }
      break;
    case MENU_SELECTOR:
      run_program (selector_paths[i]);
      break;
    }
  dirty = 1;
}

// ---------------------------------------------------------------- the calculator

static double
calc_parse (const char *str)
{
  double value = 0;
  double scale = 0;
  for (; *str; str++)
    {
      if (*str == '.')
        {
          scale = 1;
          continue;
        }
      value = value * 10 + (*str - '0');
      scale *= 10;
    }
  return scale > 0 ? value / scale : value;
}

static void
calc_format (char *out, uint32_t size, double value)
{
  double magnitude = value < 0 ? -value : value;
  if (magnitude >= 1e10)
    {
      snprintf (out, size, "Overflow");
      return;
    }
  // 6 decimals at most, trailing zeros dropped
  long long scaled = (long long)(magnitude * 1000000 + 0.5);
  long long whole = scaled / 1000000;
  long long frac = scaled % 1000000;
  int len = snprintf (out, size, "%s%lld", value < 0 && scaled ? "-" : "",
                      whole);
  if (frac)
    {
      char digits[8];
      snprintf (digits, sizeof (digits), "%06lld", frac);
      int n = 6;
      while (digits[n - 1] == '0')
        {
          n--;
        }
      digits[n] = 0;
      snprintf (out + len, size - len, ".%s", digits);
    }
}

static double
calc_apply (double a, char op, double b)
{
  switch (op)
    {
    case '+':
      return a + b;
    case '-':
      return a - b;
    case '*':
      return a * b;
    case '/':
      if (b == 0)
        {
          calc.error = 1;
          return 0;
        }
      return a / b;
    }
  return b;
}

static void
calc_press (char key)
{
  if (calc.error && key != 'C')
    {
      return;
    }
  if ((key >= '0' && key <= '9') || key == '.')
    {
      if (!calc.entering)
        {
          calc.entry[0] = 0;
          calc.entering = 1;
        }
      uint32_t len = strlen (calc.entry);
      if (len < 10 && !(key == '.' && strchr (calc.entry, '.')))
        {
          calc.entry[len] = key;
          calc.entry[len + 1] = 0;
        }
    }
  else if (key == 'C')
    {
      memset (&calc, 0, sizeof (calc));
    }
  else if (key == 'E')
    {
      calc.entry[0] = 0;
      calc.entering = 1;
    }
  else
    {
      double value = calc.entering ? calc_parse (calc.entry) : calc.shown;
      if (calc.op && calc.entering)
        {
          calc.acc = calc_apply (calc.acc, calc.op, value);
        }
      else if (!calc.op)
        {
          calc.acc = value;
        }
      calc.op = key == '=' ? 0 : key;
      calc.shown = calc.acc;
      calc.entering = 0;
    }
  dirty = 1;
}

static struct box
calc_key_box (const struct window *win, int i)
{
  struct box b = win_body (win);
  int x0 = b.x + 8;
  int y0 = b.y + 6 + 11 + 5;
  int col = i % 4;
  int row = i / 4;
  int w = CALC_KEY_W;
  int h = CALC_KEY_H;
  if (i == 16) // the wide 0
    {
      col = 0;
      row = 4;
      w = 2 * CALC_KEY_W + CALC_GAP_X;
    }
  else if (i == 17) // .
    {
      col = 2;
      row = 4;
    }
  else if (i == 15) // the tall +
    {
      h = 2 * CALC_KEY_H + CALC_GAP_Y;
    }
  return (struct box){ x0 + col * (CALC_KEY_W + CALC_GAP_X),
                       y0 + row * (CALC_KEY_H + CALC_GAP_Y), w, h };
}

static void
draw_calc (const struct window *win)
{
  struct box b = win_body (win);
  fill_pattern (b, pattern_light);
  struct box display = { b.x + 8, b.y + 6, b.w - 16, 11 };
  fill_box (display, PAPER);
  frame_box (display);
  char text[24];
  if (calc.error)
    {
      snprintf (text, sizeof (text), "Error");
    }
  else if (calc.entering)
    {
      snprintf (text, sizeof (text), "%s", calc.entry[0] ? calc.entry : "0");
    }
  else
    {
      calc_format (text, sizeof (text), calc.shown);
    }
  draw_text (display.x + display.w - 4 - text_width (text), display.y + 1,
             text, INK);
  for (int i = 0; i < (int)sizeof (calc_keys) - 1; i++)
    {
      struct box k = calc_key_box (win, i);
      char label[2] = { calc_keys[i], 0 };
      // a shadow on the right and below
      fill_box ((struct box){ k.x + 1, k.y + 1, k.w, k.h }, INK);
      fill_box (k, PAPER);
      frame_box (k);
      draw_text (k.x + (k.w - text_width (label)) / 2,
                 k.y + (k.h - FONT_ASCENT) / 2, label, INK);
      if (mode == CALC_KEY && track_win == win - windows && track_key == i
          && track_inside)
        {
          invert_box (k);
        }
    }
}

// ---------------------------------------------------------------- drawing

static struct box
desk_icon_box (int i)
{
  const char *const *art = i == DESK_DISK ? art_disk : art_trash;
  int w = art_width (art);
  int h = art_height (art);
  int y = i == DESK_DISK ? MENU_H + 12 : SCREEN_H - 32;
  return (struct box){ SCREEN_W - 50 - w / 2, y, w, h };
}

static struct box
desk_name_box (int i)
{
  const char *name = i == DESK_DISK ? "MOOS" : "Trash";
  struct box icon = desk_icon_box (i);
  int w = text_width (name) + 4;
  return (struct box){ icon.x + (icon.w - w) / 2, icon.y + icon.h + 2, w,
                       FONT_HEIGHT + 1 };
}

static int
is_selected (int win, int index)
{
  return selection.win == win && selection.index == index;
}

static void
draw_icon (struct box icon, struct box name_box, const char *const *art,
           const char *name, int selected)
{
  draw_art (icon.x, icon.y, art, selected);
  fill_box (name_box, selected ? INK : PAPER);
  draw_text_fit (name_box.x + 2, name_box.y + 1, name, name_box.w - 3,
                 selected ? PAPER : INK);
}

static void
draw_desktop (void)
{
  fill_pattern ((struct box){ 0, MENU_H, SCREEN_W, SCREEN_H - MENU_H },
                pattern_grey);
  for (int i = 0; i < DESK_ICONS; i++)
    {
      draw_icon (desk_icon_box (i), desk_name_box (i),
                 i == DESK_DISK ? art_disk : art_trash,
                 i == DESK_DISK ? "MOOS" : "Trash", is_selected (-1, i));
    }
}

static void
draw_scrollbar (const struct window *win, int active)
{
  struct box sb = win_scrollbar (win);
  vline (sb.x, sb.y, sb.h, INK);
  struct box inner = { sb.x + 1, sb.y, sb.w, sb.h };
  int track_y = inner.y + ARROW_H;
  int track_h = inner.h - 3 * ARROW_H;
  fill_box (inner, PAPER);
  if (!active)
    {
      return;
    }
  draw_art (inner.x + (inner.w - 7) / 2, inner.y + 2, art_up, 0);
  hline (inner.x, track_y - 1, inner.w, INK);
  draw_art (inner.x + (inner.w - 7) / 2, track_y + track_h + 2, art_down, 0);
  hline (inner.x, track_y + track_h, inner.w, INK);
  hline (inner.x, track_y + track_h + ARROW_H, inner.w, INK);
  // grow box: two overlapping squares
  struct box grow = { inner.x, track_y + track_h + ARROW_H, inner.w, ARROW_H };
  frame_box ((struct box){ grow.x + 2, grow.y + 2, 6, 4 });
  frame_box ((struct box){ grow.x + 4, grow.y + 3, 6, 5 });
  int range = max_scroll (win);
  if (range == 0 || track_h < 10)
    {
      return;
    }
  fill_pattern ((struct box){ inner.x, track_y, inner.w, track_h - 1 },
                pattern_grey);
  int thumb_y = track_y + (track_h - 8) * win->scroll / range;
  struct box thumb = { inner.x, thumb_y, inner.w, 8 };
  fill_box (thumb, PAPER);
  frame_box (thumb);
}

static void
draw_folder (const struct window *win)
{
  struct box b = win_body (win);
  char left[32];
  char right[32];
  uint32_t bytes = 0;
  for (int i = 0; i < win->count; i++)
    {
      bytes += win->entries[i].size;
    }
  snprintf (left, sizeof (left), win->count == 1 ? "%d Item" : "%d Items",
            win->count);
  snprintf (right, sizeof (right), "%uK in folder", (bytes + 1023) / 1024);
  draw_clip ((struct box){ b.x, b.y, b.w, HEADER_H });
  draw_text (b.x + 6, b.y + 1, left, INK);
  draw_text (b.x + b.w - 6 - text_width (right), b.y + 1, right, INK);
  hline (b.x, b.y + HEADER_H - 3, b.w, INK);
  hline (b.x, b.y + HEADER_H - 1, b.w, INK);
  struct box view = win_view (win);
  draw_clip (view);
  for (int i = 0; i < win->count; i++)
    {
      const struct dirent *entry = &win->entries[i];
      struct box icon, name;
      item_boxes (win, i, &icon, &name);
      if (name.y + name.h < view.y || icon.y > view.y + view.h)
        {
          continue;
        }
      if (!win->by_name)
        {
          draw_icon (icon, name, entry_art (entry), entry->name,
                     is_selected (win - windows, i));
          continue;
        }
      char size[16];
      char date[24];
      if (entry->is_dir)
        {
          snprintf (size, sizeof (size), "-");
        }
      else
        {
          snprintf (size, sizeof (size), "%uK", (entry->size + 1023) / 1024);
        }
      format_date (date, sizeof (date), entry->date);
      int y = icon.y + 1;
      draw_text (icon.x + 2, y, entry->is_dir ? ">" : " ", INK);
      draw_text_fit (icon.x + 10, y, entry->name, view.w - 190, INK);
      draw_text (view.x + view.w - 160 - text_width (size), y, size, INK);
      draw_text (view.x + view.w - 120, y,
                 entry->is_dir ? "folder" : "file", INK);
      draw_text (view.x + view.w - 70, y, date, INK);
      if (is_selected (win - windows, i))
        {
          invert_box (icon);
        }
    }
  draw_unclip ();
}

static void
draw_text_window (const struct window *win, int active)
{
  edit_draw (win->edit, win_view (win), win->scroll, active);
}

static void
draw_clock (const struct window *win)
{
  struct box b = win_body (win);
  struct datetime now;
  char line[40];
  time (&now);
  snprintf (line, sizeof (line), "%02d:%02d:%02d", now.hour, now.minute,
            now.second);
  draw_text (b.x + (b.w - text_width (line)) / 2, b.y + 4, line, INK);
  snprintf (line, sizeof (line), "%d %s %d", now.day,
            now.month >= 1 && now.month <= 12 ? month_names[now.month - 1]
                                              : "???",
            now.year);
  draw_text (b.x + (b.w - text_width (line)) / 2, b.y + 15, line, INK);
}

static void
draw_window (const struct window *win, int active)
{
  struct box f = win->frame;
  draw_clip (f);
  fill_box (f, PAPER);
  frame_box (f);
  struct box title = win_title_bar (win);
  hline (f.x, f.y + TITLE_H - 1, f.w, INK);
  if (active)
    {
      fill_pattern ((struct box){ f.x + 2, f.y + 2, f.w - 4, TITLE_H - 4 },
                    pattern_stripes);
      struct box close = win_close_box (win);
      fill_box ((struct box){ close.x - 2, close.y, close.w + 4, close.h },
                PAPER);
      frame_box (close);
      if (mode == CLOSE_BOX && track_win == win - windows && track_inside)
        {
          invert_box (close);
        }
    }
  int tw = min (text_width (win->title), f.w - 60);
  int tx = title.x + (title.w - tw) / 2;
  fill_box ((struct box){ tx - 5, f.y + 1, tw + 10, TITLE_H - 2 }, PAPER);
  draw_text_fit (tx, f.y + 1, win->title, tw, INK);
  draw_clip ((struct box){ f.x + 1, f.y + 1, f.w - 2, f.h - 2 });
  switch (win->kind)
    {
    case WIN_FOLDER:
      draw_folder (win);
      break;
    case WIN_TEXT:
      draw_text_window (win, active);
      break;
    case WIN_CALC:
      draw_calc (win);
      break;
    case WIN_CLOCK:
      draw_clock (win);
      break;
    }
  draw_clip ((struct box){ f.x + 1, f.y + 1, f.w - 2, f.h - 2 });
  if (scrollable (win))
    {
      draw_scrollbar (win, active);
    }
  draw_unclip ();
}

static int
menu_title_x (int m)
{
  int x = 4;
  for (int i = 0; i < m; i++)
    {
      x += (i == MENU_LOGO ? art_width (art_logo) : text_width (menus[i].title))
           + 14;
    }
  return x;
}

static struct box
menu_title_box (int m)
{
  int w = m == MENU_LOGO ? art_width (art_logo) : text_width (menus[m].title);
  return (struct box){ menu_title_x (m) - 5, 0, w + 10, MENU_H - 1 };
}

static int
menu_count (int m)
{
  int n = 0;
  while (n < MENU_ITEMS && menus[m].items[n])
    {
      n++;
    }
  return n;
}

static struct box
menu_box (int m)
{
  int w = 0;
  for (int i = 0; i < menu_count (m); i++)
    {
      w = max (w, text_width (menus[m].items[i]));
    }
  struct box title = menu_title_box (m);
  return (struct box){ title.x, MENU_H - 1, w + 26,
                       menu_count (m) * LIST_ROW + 3 };
}

static struct box
menu_item_box (int m, int i)
{
  struct box box = menu_box (m);
  return (struct box){ box.x + 1, box.y + 1 + i * LIST_ROW, box.w - 2,
                       LIST_ROW };
}

static void
draw_menu_bar (void)
{
  fill_box ((struct box){ 0, 0, SCREEN_W, MENU_H - 1 }, PAPER);
  hline (0, MENU_H - 1, SCREEN_W, INK);
  for (int m = 0; m < MENU_COUNT; m++)
    {
      int x = menu_title_x (m);
      if (m == MENU_LOGO)
        {
          draw_art (x, 1, art_logo, 0);
        }
      else
        {
          draw_text (x, 1, menus[m].title, INK);
        }
    }
  if (open_menu < 0)
    {
      return;
    }
  invert_box (menu_title_box (open_menu));
  struct box box = menu_box (open_menu);
  // a shadow on the right and below
  fill_box ((struct box){ box.x + 2, box.y + 1, box.w, box.h }, INK);
  fill_box (box, PAPER);
  frame_box (box);
  for (int i = 0; i < menu_count (open_menu); i++)
    {
      struct box item = menu_item_box (open_menu, i);
      const char *label = menus[open_menu].items[i];
      if (strcmp (label, "-") == 0)
        {
          for (int x = item.x; x < item.x + item.w; x += 2)
            {
              put_pixel (x, item.y + LIST_ROW / 2, INK);
            }
          continue;
        }
      if (menu_item_checked (open_menu, i))
        {
          draw_art (item.x + 3, item.y + 2, art_check, 0);
        }
      draw_text (item.x + 12, item.y + 1, label, INK);
      if (!menu_item_enabled (open_menu, i))
        {
          grey_out (item);
        }
      else if (i == menu_item)
        {
          invert_box (item);
        }
    }
}

static void
draw_scene (void)
{
  draw_to_frame (0);
  draw_unclip ();
  draw_desktop ();
  for (int i = 0; i < order_len; i++)
    {
      draw_window (&windows[order[i]], i == order_len - 1);
    }
  draw_menu_bar ();
  if (alert)
    {
      draw_alert ();
    }
  dirty = 0;
}

// the canvas plus what follows the mouse: outlines and the pointer
static void
present (void)
{
  frame_from_canvas ();
  draw_to_frame (1);
  draw_unclip ();
  if ((mode == DRAG_WINDOW || mode == GROW_WINDOW)
      || (mode == DRAG_ICON && drag_moved))
    {
      dotted_frame (outline);
    }
  draw_art (mouse_x, mouse_y, art_pointer, 0);
  draw_to_frame (0);
  show_frame ();
}

// ---------------------------------------------------------------- the mouse

// what is under the mouse, front to back
enum
{
  HIT_DESKTOP,
  HIT_MENU_BAR,
  HIT_DESK_ICON,
  HIT_TITLE,
  HIT_CLOSE,
  HIT_ITEM,  // an icon in a folder window, index -1 for empty space
  HIT_UP,
  HIT_DOWN,
  HIT_PAGE_UP,
  HIT_PAGE_DOWN,
  HIT_GROW,
  HIT_CALC_KEY,
  HIT_WINDOW, // anything else in a window
};

struct hit
{
  int what;
  int win;
  int index;
};

static struct hit
hit_test (int x, int y)
{
  if (y < MENU_H)
    {
      for (int m = 0; m < MENU_COUNT; m++)
        {
          if (box_contains (menu_title_box (m), x, y))
            {
              return (struct hit){ HIT_MENU_BAR, -1, m };
            }
        }
      return (struct hit){ HIT_MENU_BAR, -1, -1 };
    }
  for (int i = order_len - 1; i >= 0; i--)
    {
      int w = order[i];
      struct window *win = &windows[w];
      if (!box_contains (win->frame, x, y))
        {
          continue;
        }
      int active = i == order_len - 1;
      if (box_contains (win_title_bar (win), x, y))
        {
          struct box close = win_close_box (win);
          close = (struct box){ close.x - 2, close.y - 1, close.w + 4,
                                close.h + 2 };
          return (struct hit){ active && box_contains (close, x, y)
                                   ? HIT_CLOSE
                                   : HIT_TITLE,
                               w, -1 };
        }
      if (active && scrollable (win)
          && box_contains (win_scrollbar (win), x, y))
        {
          struct box sb = win_scrollbar (win);
          int track_h = sb.h - 3 * ARROW_H;
          int range = max_scroll (win);
          int thumb_y = sb.y + ARROW_H
                        + (range ? (track_h - 8) * win->scroll / range : 0);
          if (y < sb.y + ARROW_H)
            {
              return (struct hit){ HIT_UP, w, -1 };
            }
          if (y >= sb.y + sb.h - ARROW_H)
            {
              return (struct hit){ HIT_GROW, w, -1 };
            }
          if (y >= sb.y + sb.h - 2 * ARROW_H)
            {
              return (struct hit){ HIT_DOWN, w, -1 };
            }
          return (struct hit){ y < thumb_y ? HIT_PAGE_UP : HIT_PAGE_DOWN, w,
                               -1 };
        }
      if (active && win->kind == WIN_CALC)
        {
          for (int k = 0; k < (int)sizeof (calc_keys) - 1; k++)
            {
              if (box_contains (calc_key_box (win, k), x, y))
                {
                  return (struct hit){ HIT_CALC_KEY, w, k };
                }
            }
        }
      if (win->kind == WIN_FOLDER && box_contains (win_view (win), x, y))
        {
          for (int k = 0; k < win->count; k++)
            {
              struct box icon, name;
              item_boxes (win, k, &icon, &name);
              if (box_contains (icon, x, y) || box_contains (name, x, y))
                {
                  return (struct hit){ HIT_ITEM, w, k };
                }
            }
          return (struct hit){ HIT_ITEM, w, -1 };
        }
      return (struct hit){ HIT_WINDOW, w, -1 };
    }
  for (int i = 0; i < DESK_ICONS; i++)
    {
      if (box_contains (desk_icon_box (i), x, y)
          || box_contains (desk_name_box (i), x, y))
        {
          return (struct hit){ HIT_DESK_ICON, -1, i };
        }
    }
  return (struct hit){ HIT_DESKTOP, -1, -1 };
}

// the box of an icon, for the outline while it is dragged
static struct box
target_box (struct target t)
{
  if (t.win < 0)
    {
      return desk_icon_box (t.index);
    }
  struct box icon, name;
  item_boxes (&windows[t.win], t.index, &icon, &name);
  return icon;
}

static void
menu_track (void)
{
  struct hit hit = hit_test (mouse_x, mouse_y);
  if (hit.what == HIT_MENU_BAR && hit.index >= 0 && hit.index != open_menu)
    {
      open_menu = hit.index;
      menu_item = -1;
      dirty = 1;
      return;
    }
  int item = -1;
  for (int i = 0; i < menu_count (open_menu); i++)
    {
      if (box_contains (menu_item_box (open_menu, i), mouse_x, mouse_y)
          && strcmp (menus[open_menu].items[i], "-") != 0)
        {
          item = i;
        }
    }
  if (item != menu_item)
    {
      menu_item = item;
      dirty = 1;
    }
}

static void
close_menu (void)
{
  open_menu = -1;
  menu_item = -1;
  menu_sticky = 0;
  mode = IDLE;
  dirty = 1;
}

static void
alert_press (void)
{
  for (int i = 0; i < (alert == ALERT_DELETE ? 2 : 1); i++)
    {
      if (box_contains (alert_button (i), mouse_x, mouse_y))
        {
          alert_pressed = i;
          dirty = 1;
        }
    }
}

static void
alert_release (void)
{
  int pressed = alert_pressed;
  alert_pressed = -1;
  dirty = 1;
  if (pressed >= 0 && box_contains (alert_button (pressed), mouse_x, mouse_y))
    {
      alert_done (pressed == 0);
    }
}

static void
on_press (void)
{
  if (alert)
    {
      alert_press ();
      return;
    }
  if (mode == MENU)
    {
      // a click while a menu stays down picks an item or closes it
      menu_sticky = 0;
      menu_track ();
      return;
    }
  struct hit hit = hit_test (mouse_x, mouse_y);
  if (hit.win >= 0 && hit.win != front_index ())
    {
      bring_to_front (hit.win);
    }
  switch (hit.what)
    {
    case HIT_MENU_BAR:
      if (hit.index >= 0)
        {
          mode = MENU;
          open_menu = hit.index;
          menu_item = -1;
          dirty = 1;
        }
      break;
    case HIT_TITLE:
      mode = DRAG_WINDOW;
      track_win = hit.win;
      outline = windows[hit.win].frame;
      grab_x = mouse_x - outline.x;
      grab_y = mouse_y - outline.y;
      break;
    case HIT_CLOSE:
      mode = CLOSE_BOX;
      track_win = hit.win;
      track_inside = 1;
      dirty = 1;
      break;
    case HIT_GROW:
      mode = GROW_WINDOW;
      track_win = hit.win;
      outline = windows[hit.win].frame;
      break;
    case HIT_UP:
    case HIT_DOWN:
      scroll_by (&windows[hit.win],
                 (hit.what == HIT_UP ? -1 : 1)
                     * (windows[hit.win].kind == WIN_TEXT ? 3 * FONT_HEIGHT
                                                          : CELL_H / 2));
      break;
    case HIT_PAGE_UP:
    case HIT_PAGE_DOWN:
      scroll_by (&windows[hit.win], (hit.what == HIT_PAGE_UP ? -1 : 1)
                                        * (win_view (&windows[hit.win]).h - 10));
      break;
    case HIT_WINDOW:
      {
        struct window *win = &windows[hit.win];
        if (win->kind == WIN_TEXT)
          {
            edit_click (win->edit, win_view (win), win->scroll, mouse_x,
                        mouse_y);
            dirty = 1;
          }
        break;
      }
    case HIT_CALC_KEY:
      mode = CALC_KEY;
      track_win = hit.win;
      track_key = hit.index;
      track_inside = 1;
      dirty = 1;
      break;
    case HIT_DESK_ICON:
    case HIT_ITEM:
    case HIT_DESKTOP:
      {
        struct target t = { hit.win, hit.index };
        if (hit.what == HIT_DESKTOP)
          {
            t = (struct target){ -1, -1 };
          }
        if (selection.win != t.win || selection.index != t.index)
          {
            selection = t;
            dirty = 1;
          }
        if (t.index < 0)
          {
            break;
          }
        uint32_t now = uptime ();
        if (last_click.win == t.win && last_click.index == t.index
            && now - last_click_ms < DOUBLE_CLICK_MS)
          {
            last_click = (struct target){ -2, -2 };
            open_target (t);
            break;
          }
        last_click = t;
        last_click_ms = now;
        mode = DRAG_ICON;
        drag_source = t;
        drag_moved = 0;
        outline = target_box (t);
        grab_x = mouse_x - outline.x;
        grab_y = mouse_y - outline.y;
        break;
      }
    }
}

static void
on_move (void)
{
  switch (mode)
    {
    case MENU:
      menu_track ();
      break;
    case DRAG_WINDOW:
      outline.x = mouse_x - grab_x;
      // the title bar stays below the menu bar and on the screen
      outline.y = min (max (mouse_y - grab_y, MENU_H), SCREEN_H - TITLE_H);
      break;
    case GROW_WINDOW:
      outline.w = max (mouse_x - outline.x + 6, MIN_W);
      outline.h = max (mouse_y - outline.y + 4, MIN_H);
      break;
    case DRAG_ICON:
      if (iabs (mouse_x - grab_x - outline.x) >= DRAG_START
          || iabs (mouse_y - grab_y - outline.y) >= DRAG_START)
        {
          drag_moved = 1;
        }
      if (drag_moved)
        {
          outline.x = mouse_x - grab_x;
          outline.y = mouse_y - grab_y;
          last_click = (struct target){ -2, -2 };
        }
      break;
    case CLOSE_BOX:
      {
        struct box close = win_close_box (&windows[track_win]);
        int inside = box_contains (close, mouse_x, mouse_y);
        if (inside != track_inside)
          {
            track_inside = inside;
            dirty = 1;
          }
        break;
      }
    case CALC_KEY:
      {
        int inside = box_contains (calc_key_box (&windows[track_win], track_key),
                                   mouse_x, mouse_y);
        if (inside != track_inside)
          {
            track_inside = inside;
            dirty = 1;
          }
        break;
      }
    }
}

static void
on_release (void)
{
  if (alert)
    {
      alert_release ();
      return;
    }
  switch (mode)
    {
    case MENU:
      {
        struct hit hit = hit_test (mouse_x, mouse_y);
        // let go on the title after a click: the menu stays down
        if (hit.what == HIT_MENU_BAR && menu_item < 0 && !menu_sticky
            && hit.index == open_menu)
          {
            menu_sticky = 1;
            return;
          }
        int m = open_menu;
        int item = menu_item;
        close_menu ();
        if (item >= 0)
          {
            menu_action (m, item);
          }
        return;
      }
    case DRAG_WINDOW:
      windows[track_win].frame = outline;
      dirty = 1;
      break;
    case GROW_WINDOW:
      {
        struct window *win = &windows[track_win];
        win->frame = outline;
        win->scroll = min (win->scroll, max_scroll (win));
        dirty = 1;
        break;
      }
    case CLOSE_BOX:
      if (track_inside)
        {
          close_window (track_win);
        }
      dirty = 1;
      break;
    case CALC_KEY:
      if (track_inside)
        {
          calc_press (calc_keys[track_key]);
        }
      dirty = 1;
      break;
    case DRAG_ICON:
      {
        struct hit hit = hit_test (mouse_x, mouse_y);
        // files dropped on the trash get deleted, after asking
        if (drag_moved && drag_source.win >= 0 && hit.what == HIT_DESK_ICON
            && hit.index == DESK_TRASH)
          {
            selection = drag_source;
            ask_delete ();
          }
        dirty = 1;
        break;
      }
    }
  mode = IDLE;
}

// ---------------------------------------------------------------- keys

// key events carry the unshifted char, this is what shift makes of it on
// a us keyboard
static int
typed (int key)
{
  static const char plain[] = "`1234567890-=[]\\;',./";
  static const char shift[] = "~!@#$%^&*()_+{}|:\"<>?";
  if (!shift_held || key >= 0x80)
    {
      return key;
    }
  if (key >= 'a' && key <= 'z')
    {
      return key - 'a' + 'A';
    }
  const char *p = strchr (plain, key);
  return p ? shift[p - plain] : key;
}

static void
on_key (int key)
{
  if (key == KEY_SHIFT || key == (KEY_SHIFT | KEY_RELEASED))
    {
      shift_held = !(key & KEY_RELEASED);
      return;
    }
  if (key == KEY_CTRL || key == (KEY_CTRL | KEY_RELEASED))
    {
      ctrl_held = !(key & KEY_RELEASED);
      return;
    }
  if (key & KEY_RELEASED)
    {
      return;
    }
  if (alert)
    {
      if (key == '\n' || key == 27)
        {
          alert_done (key == '\n');
        }
      return;
    }
  if (mode == MENU)
    {
      if (key == 27)
        {
          close_menu ();
        }
      return;
    }
  struct window *front = front_window ();
  if (ctrl_held)
    {
      switch (key)
        {
        case 'q':
          quit = 1;
          break;
        case 'w':
          if (front)
            {
              close_window (front_index ());
            }
          break;
        case 'o':
          open_target (selection);
          break;
        }
      return;
    }
  if (front && front->kind == WIN_TEXT)
    {
      edit_key (front->edit, typed (key));
      reveal_cursor (front);
      return;
    }
  if (front && front->kind == WIN_CALC)
    {
      char c = typed (key);
      if (c == '\n')
        {
          c = '=';
        }
      else if (c == 27 || c == 'c')
        {
          c = 'C';
        }
      else if (c == 'e')
        {
          c = 'E';
        }
      if (strchr (calc_keys, c))
        {
          calc_press (c);
        }
      return;
    }
  if (front && scrollable (front))
    {
      int page = win_view (front).h - 10;
      switch (key)
        {
        case KEY_UP:
          scroll_by (front, -FONT_HEIGHT);
          return;
        case KEY_DOWN:
          scroll_by (front, FONT_HEIGHT);
          return;
        case KEY_PAGE_UP:
          scroll_by (front, -page);
          return;
        case KEY_PAGE_DOWN:
          scroll_by (front, page);
          return;
        }
    }
  if (key == '\n')
    {
      open_target (selection);
    }
}

// ---------------------------------------------------------------- main

static int
clock_open (void)
{
  for (int i = 0; i < MAX_WINDOWS; i++)
    {
      if (windows[i].used && windows[i].kind == WIN_CLOCK)
        {
          return 1;
        }
    }
  return 0;
}

int
main (void)
{
  if (gfx_mode (GFX_MONO) < 0)
    {
      eprint ("desk: the screen is in use\n");
      return 1;
    }
  key_event (1);
  mouse_event (0, 1);
  open_folder ("0:/", "MOOS");
  uint32_t last_second = 0;
  int moved = 1;
  while (!quit)
    {
      int busy = 0;
      int key;
      while (!quit && (key = key_event (0)) >= 0)
        {
          on_key (key);
          busy = 1;
        }
      struct mouse_event ev;
      while (!quit && mouse_event (&ev, 0) == 0)
        {
          busy = 1;
          int x = mouse_x;
          int y2 = mouse_y2;
          if (ev.absolute)
            {
              // the pointer of the host, so it stays under the real one
              x = ev.dx * SCREEN_W / (MOUSE_MAX + 1);
              y2 = ev.dy * SCREEN_H * 2 / (MOUSE_MAX + 1);
            }
          else
            {
              x += ev.dx;
              y2 += ev.dy;
            }
          x = min (max (x, 0), SCREEN_W - 1);
          y2 = min (max (y2, 0), SCREEN_H * 2 - 1);
          if (x != mouse_x || y2 != mouse_y2)
            {
              mouse_x = x;
              mouse_y2 = y2;
              mouse_y = mouse_y2 / 2;
              moved = 1;
              on_move ();
            }
          int pressed = (ev.buttons & MOUSE_LEFT) && !(buttons & MOUSE_LEFT);
          int released = !(ev.buttons & MOUSE_LEFT) && (buttons & MOUSE_LEFT);
          buttons = ev.buttons;
          if (pressed)
            {
              on_press ();
            }
          else if (released)
            {
              on_release ();
            }
        }
      // the clock ticks once a second
      uint32_t second = uptime () / 1000;
      if (second != last_second && clock_open ())
        {
          dirty = 1;
        }
      last_second = second;
      if (quit)
        {
          break;
        }
      if (dirty)
        {
          draw_scene ();
          moved = 1;
        }
      if (moved)
        {
          present ();
          moved = 0;
        }
      if (!busy)
        {
          sleep (10);
        }
    }
  gfx_mode (GFX_TEXT);
  return 0;
}
