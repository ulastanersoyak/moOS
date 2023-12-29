#ifndef PAGE_H
#define PAGE_H

#include <stdint.h>

#define CACHE_DISABLED 0b00010000
#define WRITE_THROUGH  0b00001000
#define ACCESS_ALL     0b00000100
#define IS_WRITABLE    0b00000010
#define IS_PRESENT     0b00000001

struct page_dir{
  uint32_t *dir_entry;
};

// returns a page directory with 4 gb virtual address space
// with tables that contain given flags
struct page_dir *page_dir_init(uint32_t flags);

// switches paging directories using an asm subroutine that tells
// processor where to find entry point of pagind directory
void switch_page_dir(uint32_t *dir_entry);

void enable_system_paging(void);
#endif //PAGE_H