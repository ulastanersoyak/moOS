#include "ctype.h"

#include "../stdio/stdio.h"

int32_t isalnum(int32_t c){
          /* decimal numbers */  /* uppercase chars */    /* lowercase chars */
  return ((c >= 48 && c <= 57) || (c >= 65 && c<= 90) || (c >= 97 && c <= 122)) ? 0 : 1;
}

int32_t isalpha(int32_t c){
          /* uppercase chars */    /* lowercase chars */
  return ((c >= 65 && c<= 90) || (c >= 97 && c <= 122)) ? 0 : 1;
}

int32_t iscntrl(int32_t c){
          // ascii control chars
  return ((c >= 0 && c <= 31) || c == 127) ? 0 : 1;   
}

int32_t isdigit(int32_t c){
  return (c >= 48 && c <= 57) ? 0 : 1;
}

int32_t isgraph(int32_t c){
  return (c >= 33 && c <= 126) ? 0 : 1;  
}

int32_t islower(int32_t c){
  return (c >= 97 && c <= 122) ? 0 : 1;
}

int32_t isprint(int32_t c){
  return (c >= 33 && c <= 126) ? 0 : 1;  
}

int32_t ispunct(int32_t c){
  // TODO: might be a better way of doing this
  return ((c >= 33 && c <= 47) || (c >= 58 && c <= 64) || (c >= 91 && c <= 96) || (c >= 123 && c <= 126)) ? 0 : 1;
}

int32_t isspace(int32_t c){
  return c == 32 ? 0 : 1;
}

int32_t isupper(int32_t c){
  return (c >= 65 && c<= 90) ? 0 : 1; 
}

int32_t isxdigit(int32_t c){
  return ((c >= 48 && c <= 57) || (c >= 97 && c <= 102)) ? 0 : 1;
}

int32_t tolower(int32_t c){
  if(isupper(c)){
    return c - 32;
  }
  return c;
}

int32_t toupper(int32_t c){
  if(islower(c)){
    return c + 32;
  }
  return c;
}
