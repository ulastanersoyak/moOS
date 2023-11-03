#include "stdlib.h"
#include "../../kernel/config.h"
#include "../../kernel/kmem/kheap.h"
#include "../../kernel/config.h"

#include <stdint.h>


void *malloc(size_t size){
 return kmalloc(size); 
}
void *calloc(size_t size){
  return kcalloc(size);
}
void free(void *ptr){
  kfree(ptr);
}

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


static unsigned long int next = 1;  // NB: "unsigned long int" is assumed to be 32 bits wide
 
uint32_t rand(void)  // RAND_MAX assumed to be 32767
{
    next = next * 1103515245 + 12345;
    return (unsigned int) (next / 65536) % 32768;
}

uint32_t rand_range(uint32_t lower, uint32_t upper){
  int32_t rnd = rand();
  return rnd * (upper - lower) / RAND_MAX + lower;
}
 
void srand(uint32_t seed)
{
    next = seed;
}
