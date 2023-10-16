#include "string.h"

#include <stddef.h>
#include <stdint.h>

int32_t memcmp(const void *str1, const void *str2, size_t n) {
  // check if both point to the same memory location
  if (str1 == str2) {
    return 0;
  }
  const uint8_t *ptr1 = (const uint8_t *)str1;
  const uint8_t *ptr2 = (const uint8_t *)str2;
  for (size_t i = 0; i < n; i++, ptr1++, ptr2++) {
    if (*ptr1 != *ptr2) {
      return (*ptr1 > *ptr2) ? 1 : -1;
    }
  }
  return 0;
}

void *memcpy(void *dest, const void *src, size_t n) {
  uint8_t *destin = (uint8_t *)dest;
  uint8_t *source = (uint8_t *)src;
  for (size_t i = 0; i < n; i++) {
    destin[i] = source[i];
  }
  return dest;
}

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
