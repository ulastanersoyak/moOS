void dummy_test_entrypoint() {}

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
}
