#include "../drivers/ports.h"

void kernel_start() {
  char *video_memory = (char *)0xb8000;
  int row = 15 * 160;
  video_memory[row] = 'h';
  video_memory[row + 2] = 'e';
  video_memory[row + 4] = 'l';
  video_memory[row + 6] = 'l';
  video_memory[row + 8] = 'o';
  video_memory[row + 10] = ' ';
  video_memory[row + 12] = 'k';
  video_memory[row + 14] = 'e';
  video_memory[row + 16] = 'r';
  video_memory[row + 18] = 'n';
  video_memory[row + 20] = 'e';
  video_memory[row + 22] = 'l';

  port_byte_out(0x3d4, 14);
  int position = port_byte_in(0x3d5);
  position = position << 8;
  port_byte_out(0x3d4, 15);
  position += port_byte_in(0x3d5);
  int offset_from_vga = position * 2;
  char *vga = (char *)0xb8000;
  vga[offset_from_vga] = 'x';
  vga[offset_from_vga + 1] = 0x0f;
}
