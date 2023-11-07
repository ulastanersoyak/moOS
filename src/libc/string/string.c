#include "string.h"

#include <stddef.h>
#include <stdint.h>

void *memchr(const void *str, uint32_t c, size_t n) {
  unsigned char *p = (unsigned char *)str;
  unsigned char *chr = NULL;
  while ((str != NULL) && (n--)) {
    if (*p != (unsigned char)c) {
      p++;
    } else {
      chr = p;
      break;
    }
  }
  return chr;
}

int32_t memcmp(const void *str1, const void *str2, size_t n) {
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

void *memset(void *ptr, int c, size_t n) {
  char *c_ptr = (char *)ptr;
  for (size_t i = 0; i < n; i++) {
    c_ptr[i] = (char)c;
  }
  return ptr;
}

char *strcat(char *dest,const char *src){
  size_t size = strlen(dest);
  size_t i;
  for(i = 0; src[i] != '\0'; i++){
    dest[size + i] = src[i];
  }
  dest[size + i] = '\0';
  return dest;
}

char *strncat(char *dest, const char *src, uint32_t n){
  // TODO: there might be a better way of doing this
  size_t size = strlen(dest);
  size_t i;
  for(i = 0; i < n; i++){
    dest[size + i] = src[i];
  }
  dest[size + i] = '\0';
  return dest;
}

char *strchr(const char *str, uint32_t n){
  for(size_t i = 0; str[i] != '\0'; i++){
    if(str[i] == n){
      // add offset i to base pointer to return the string FROM offset to end.
      return (char*)str + i;
    }   
  }
  return 0;
}

int32_t strcmp(const char *str1, const char *str2){
  if(!(strlen(str1) == strlen(str2))){
    return -1;
  }
  for(size_t i = 0; i <strlen(str1); i++){
    if(str1[i] != str2[i]){
      return -1;
    }
  }
  return 0;
}

int32_t strncmp(const char *str1, const char *str2, uint32_t n){
  if(n > strlen(str1) || n> strlen(str2)){
    return -1;
  }
  for(size_t i = 0; i < n; i++){
    if(str1[i] != str2[i]){
      return str1[i] - str2[i];
    }
  }
  return 0;
}

char *strcpy(char *dest, const char *src){
  while(*src != 0){
    *dest = *src;
    dest++;
    src++;
  }
  *dest = '\0';
  return dest;
}


size_t strcspn(const char *str1, const char *str2){
  size_t rs = 0;

  while(*str1){
    if(strchr(str2,(uint32_t)*str1)){
      return rs;
    }
    str1++;
    rs++;
  }
  return rs;
}

size_t strlen(const char *str) {
  size_t size = 0;
  while (str[size]) {
    size++;
  }
  return size;
}
