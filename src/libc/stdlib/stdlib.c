#include "stdlib.h"
#include "../../kernel/config.h"
#include "../../memory/heap/heap.h"
#include <stdint.h>

void *malloc(size_t size);
void *calloc(size_t size);
void free(void *ptr);

uint32_t atoi(const char *buffer){
  int32_t rs = 0;
  for(size_t i = 0; buffer[i] != '\0'; i++){
      int32_t temp = buffer[i] - 48;
      if(temp >= 0 && temp <= 9){
        rs = rs * 10 + temp;
    } 
  }
  return rs;
}

