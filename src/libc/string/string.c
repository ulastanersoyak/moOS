#include "string.h"

#include <stddef.h>
#include <stdint.h>

// int32_t memcmp(const void *str1, const void *str2, size_t n) {
//   for (size_t i = 0; i < n; i++) {
//     if (str1[i] != str2[i]) {
//      return -1;
//    }
// }
// }

void *memset(void *str, int c, size_t n) {
  char *c_ptr = (char *)str;
  for (size_t i = 0; i < n; i++) {
    c_ptr[i] = (char)c;
  }
  return str;
}

size_t strlen(const char *str) {
  size_t size = 0;
  while (str[size]) {
    size++;
  }
  return size;
}
