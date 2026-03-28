#ifndef DESK_H
#define DESK_H

#include "lib/moos.h"

// the desktop draws on a 640x240 canvas in black and white. every row is
// shown twice on the 640x480 screen, so pixels are twice as tall as wide,
// like on the apple ii
#define SCREEN_W GFX_MONO_WIDTH
#define SCREEN_H (GFX_MONO_HEIGHT / 2)

// pixel colours, a set bit is black
#define PAPER 0
#define INK 1

// font: rows above the baseline, below it and the space between lines
#define FONT_ASCENT 7
#define FONT_HEIGHT 9

struct box
{
  int x, y, w, h;
};

// grey patterns, 8 rows of 8 pixels
extern const uint8_t pattern_grey[8];
extern const uint8_t pattern_light[8];
extern const uint8_t pattern_stripes[8];

// drawing goes to the canvas, or to the frame that adds the pointer and
// outlines on top of it before it gets shown
void draw_to_frame (int frame);

// limits drawing to the inside of the box, until draw_unclip
void draw_clip (struct box box);
void draw_unclip (void);

void fill_box (struct box box, int colour);
void fill_pattern (struct box box, const uint8_t pattern[8]);
void invert_box (struct box box);
void frame_box (struct box box); // 1 pixel black border
void hline (int x, int y, int w, int colour);
void vline (int x, int y, int h, int colour);
// an outline of dotted lines, drawn with xor so drawing it twice removes it
void dotted_frame (struct box box);
void put_pixel (int x, int y, int colour);

// draws pixel art: rows of '#' (ink), '.' (paper) and ' ' (left alone).
// inverted swaps ink and paper
void draw_art (int x, int y, const char *const *rows, int inverted);
int art_width (const char *const *rows);
int art_height (const char *const *rows);

// draws text with its top at y, returns its width. chars without a glyph
// show up as '?'
int draw_text (int x, int y, const char *text, int colour);
int text_width (const char *text);
// draws as much of text as fits into max_w pixels, cut off with "..."
void draw_text_fit (int x, int y, const char *text, int max_w, int colour);

// shows the frame on the screen, every row twice
void show_frame (void);

// copies the canvas into the frame
void frame_from_canvas (void);

int box_contains (struct box box, int x, int y);

// the text editor of text windows (deskedit.c)
#define EDIT_MAX 16384
#define EDIT_STATUS_H (FONT_HEIGHT + 3) // the status line below the text

struct editor
{
  char text[EDIT_MAX + 1];
  int len;
  int cursor; // offset of the char under the cursor
  int column; // where up and down try to put the cursor
  char message[64]; // shown in the status line
};

// what edit_key wants the window to do
#define EDIT_DONE 0

// takes the file, control chars become spaces
void edit_load (struct editor *ed, const char *data, int len);
// handles a key: a char or a KEY_ code
int edit_key (struct editor *ed, int key);
// puts the cursor on the char at x, y of a view scrolled by scroll pixels
void edit_click (struct editor *ed, struct box view, int scroll, int x,
                 int y);
// draws the text and the status line into view, the cursor if active
void edit_draw (const struct editor *ed, struct box view, int scroll,
                int active);
int edit_line_count (const struct editor *ed);
int edit_cursor_line (const struct editor *ed);
// top of the cursor's line in the unscrolled text
int edit_cursor_y (const struct editor *ed);

#endif // !DESK_H
