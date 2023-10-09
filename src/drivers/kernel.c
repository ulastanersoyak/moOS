#include "screen.h"
void print_char(char c, int col, int row, char attributes) {
  // a pointer to the start of video memory
  unsigned char *vidmem = (unsigned char *)video_address;
  // if attribute is zero, set it to default
  if (!attributes) {
    attributes = monochrome;
  }
}
