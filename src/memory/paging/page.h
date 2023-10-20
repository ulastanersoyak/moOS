#ifndef PAGE_H
#define PAGE_H

#include <stdint.h>

#define PAGING_CACHE_DISABLED 0b00010000
#define PAGING_WRITE_THROUGH 0b00001000
#define PAGING_ACCESS_FROM_ALL 0b00000100
#define PAGING_IS_WRITABLE 0b00000010
#define PAGING_IS_PRESENT 0b00000001

struct page_dir {
  uint32_t *dir_entry;
};

struct page_dir *page_dir_init(uint8_t flags);
void switch_page(uint32_t *dir);
uint32_t *get_dir(struct page_dir *dir);

void enable_paging(void);

#endif // !PAGE_H
