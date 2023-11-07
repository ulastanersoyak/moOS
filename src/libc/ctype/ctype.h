#ifndef CTYPE_H
#define CTYPE_H

#include <stdint.h>

// checks whether the passed character is alphanumeric
int32_t isalnum(int32_t c);

// checks whether the passed character is alphabetic
int32_t isalpha(int32_t c);

// checks whether the passed character is control character
int32_t iscntrl(int32_t c);

// checks whether the passed character is decimal digit
int32_t isdigit(int32_t c);

// checks whether the passed character has graphical representation 
int32_t isgraph(int32_t c);

// checks whether the passed character is lowercase letter
int32_t islower(int32_t c);

// checks whether the passed character is printable
int32_t isprint(int32_t c);

// checks whether the passed character is a punctuation character
int32_t ispunct(int32_t c);

// checks whether the passed character is white-space
int32_t isspace(int32_t c);

// checks whether the passed character is an uppercase letter
int32_t isupper(int32_t c);

// checks whether the passed character is a hexadecimal digit
int32_t isxdigit(int32_t c);

// converts uppercase char to lowercase 
int32_t tolower(int32_t c);

// converts lowercase char to uppercase
int32_t toupper(int32_t c);
#endif // !CTYPE_H
