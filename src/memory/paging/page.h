#ifndef PAGE_H
#define PAGE_H

#include <stdint.h>

#define CACHE_DISABLED 0b00010000
#define WRITE_THROUGH 0b00001000
#define ACCESS_FROM_ALL 0b00000100
#define IS_WRITABLE 0b00000010
#define IS_PRESENT 0b00000001

struct page_dir {
  uint32_t *dir_entry;
};

struct paging_addr {
  int32_t *dir_idx;
  int32_t *table_idx;
};

struct page_dir *page_dir_init(uint8_t flags);
void switch_page_dir(uint32_t *dir);
uint32_t *get_current_dir(struct page_dir *dir);

void enable_paging(void);
int32_t get_paging_index(void *vir_addr, struct paging_addr *addr);
int32_t set_page(uint32_t *page_dir, void *vir_addr, uint32_t val);

#endif // !PAGE_H
