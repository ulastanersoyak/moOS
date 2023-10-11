#include "screen.h"
#include "ports.h"

int get_cursor_offset();
void set_cursor_offset(int offset);
int print_char(char c, int col, int row, char attr);
int get_offset(int col, int row);
int get_offset_row(int offset);
int get_offset_col(int offset);

// print the msg to a specifiet location. if col or row is negative it will use
// current offset
void kprint_at(char *msg, int col, int row) {
  int offset;
  if (col >= 0 && row >= 0) {
    offset = get_offset(col, row);
  } else {
    offset = get_cursor_offset();
    row = get_offset_row(offset);
    col = get_offset_col(offset);
  }

  int i = 0;
  while (msg[i] != 0) {
    offset = print_char(msg[i++], col, row, b_w);
    row = get_offset_row(offset);
    col = get_offset_col(offset);
  }
}

void kprint(char *msg) { kprint_at(msg, -1, -1); }

// kernels printing function. has direct video memory access.
// if row or col is negative, will use current offset. if attr is zero it will
// use b_w as default returns the offset of next char and sets video cursor to
// returned offset
int print_char(char c, int col, int row, char attr) {
  unsigned char *video_mem = (unsigned char *)video_address;
  if (!attr) {
    attr = b_w;
  }

  // check if col and row is in bounds. else print a red E
  if (col >= max_cols || row >= max_rows) {
    video_mem[2 * (max_cols) * (max_rows)-2] = 'E';
    video_mem[2 * (max_cols) * (max_rows)-1] = r_w;
    return get_offset(col, row);
  }

  int offset = 0;
  if (col >= 0 && row >= 0) {
    offset = get_offset(col, row);
  } else {
    offset = get_cursor_offset();
  }

  // if \n is processed, set col to 0 and row to row+1
  if (c == '\n') {
    row = get_offset_row(offset);
    offset = get_offset(0, row + 1);
  } else {
    video_mem[offset] = c;
    video_mem[offset + 1] = attr;
    offset += 2;
  }
  set_cursor_offset(offset);
  return offset;
}

int get_cursor_offset() {
  port_byte_out(reg_screen_ctrl, 14);
  int offset = port_byte_in(reg_screen_data) << 8; // set high 4 byte
  port_byte_out(reg_screen_ctrl, 15);
  offset += port_byte_in(reg_screen_data);
  return offset * 2;
}

void set_cursor_offset(int offset) {
  offset /= 2;
  port_byte_out(reg_screen_ctrl, 14);
  port_byte_out(reg_screen_data, (unsigned char)(offset >> 8));
  port_byte_out(reg_screen_ctrl, 15);
  port_byte_out(reg_screen_data, (unsigned char)(offset & 0xff));
}

void clear_screen() {
  int screen_size = max_cols * max_rows;
  char *screen = (char *)video_address;
  int i = 0;
  for (; i < screen_size; i++) {
    screen[i * 2] = ' ';
    screen[(i * 2) + 1] = b_w;
  }
  set_cursor_offset(get_offset(0, 0));
}

int get_offset(int col, int row) { return 2 * ((row * max_cols) + col); }

int get_offset_row(int offset) { return offset / (2 * max_cols); }

int get_offset_col(int offset) {
  return (offset - (get_offset_row(offset) * 2 * max_cols)) / 2;
}
