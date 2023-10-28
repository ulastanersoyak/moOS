
#include "string.h"

void *memchr(const void *str, uint32_t c, size_t n) {
  // TODO
}

int32_t memcmp(const void *str1, const void *str2, size_t n) {
  for (size_t i = 0; i < n; i++) {
    if (str1[i] != str2[i]) {
      return -1;
    }
  }
}
size_t strlen(const char *str) {
  size_t size = 0;
  while (str[size]) {
    size++;
  }
  return size;
}
