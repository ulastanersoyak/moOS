#ifndef STRING_H
#define STRING_H

#include <stddef.h>
#include <stdint.h>

// searches for the first occurence of the char c in the first n bytes of the string pointed to.
void *memchr(const void *str, uint32_t c, size_t n);

// compares first n bytes of pointed area. if return value < 0 str1 is less than
// str2. if return value > 0 str1 is greater than str2. if return value = 0 str1
// is equal to str2
int32_t memcmp(const void *str1, const void *str2, size_t n);

// copies n chars from src to dest. returns a pointer to the position which is
// dest
void *memcpy(void *dest, const void *src, size_t n);

// another function to copy n chars from str2 to str1
void *memmove(void *dest, void *src, size_t n);

// copies char c to the first n chars of the string pointed to.
void *memset(void *str, int c, size_t n);

// appends src to the end of the dest
char *strcat(char *dest, const char *src);

// appends src t to the end of the dest up to n bytes
char *strncat(char *dest, const char *src, uint32_t n);

// searches for the first occurence of the char c in the string pointed to
char *strchr(const char *str, uint32_t n);

// compares str1 to str2
int32_t strcmp(const char *str1, const char *str2);

// compares str1 to str2 up to n byte
int32_t strncmp(const char *str1, const char *str2, uint32_t n);

// compares str1 to str2. result depends on the LC_COLLATE setting of the
int32_t strcoll(const char *str1, const char *str2);

// copies src to dest
char *strcpy(char *dest, const char *src);

// copies src to dest up to n bytes
char *strncpy(char *dest, const char *src, size_t n);

// calculates the length of the initial segment of str1 which consist entirely
// of chars not in str2
size_t strcspn(const char *str1, const char *str2);

// searches an interna array of error numbers and returns a pointer to an error
// msg string
char *strerror(uint32_t errnum);

// returns length of given string excluding terminating null char
size_t strlen(const char *str);

// finds the first char in str1 that matches any char specified on str2
char *strpbrk(const char *str1, const char *str2);

// searches for the last occurrence of the char c in the string pointed to
char *strchr(const char *str, uint32_t c);

// calculates length of the initial segment of str1 which consist entirety of
// chars in str2
size_t strspn(const char *str1, const char *str2);

// finds the first occurence of the entire str2 (not including null terminator)
// in the str2
char *strstr(const char *str1, const char *str2);

// transforms first n chars of the src into current locale and places them in
// the str
size_t strxfrm(char *dest, const char *src, size_t n);

// TODO REST OF STRING LIBRARY
#endif // !STRING_H
