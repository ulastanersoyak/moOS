void dummy_test_entrypoint() {}

void kernel_start() {
  char *video_memory = (char *)0xb8000;
  *video_memory = 'x';
}
