#ifndef PAGE_H
#define PAGE_H

#include <stdint.h>

// page mask bits. used in addresses last bytes since addresses are alligned to 4kb and dont
// need to use last bytes
#define CACHE_DISABLED 0b00010000
#define WRITE_THROUGH  0b00001000
#define ACCESS_ALL     0b00000100
#define IS_WRITABLE    0b00000010
#define IS_PRESENT     0b00000001

struct page_dir{
  uint32_t *dir_entry;
};

// holds a virtual addresses directory and page indexes to locate it
struct page_location{
  uint32_t dir_idx;
  uint32_t page_idx;
};

// returns a page directory with 4 gb virtual address space
// with tables and pages that are initialized with given flags
struct page_dir *page_dir_init(uint32_t flags);

// sets given virtual memory addresses dir and page indexes
// returns 0 if successful
int32_t set_page_location(struct page_location *page_loc,void *vir_addr);

// sets a physical address to a virtual address return 0 if sucessful
int32_t virtualize_mem(uint32_t *dir, void* vir_addr, uint32_t phys_addr);

// asm wrapper functions

// tells processor where to find entry point of paging directory
// by storing pointer to paging dir at cr3 register
void switch_page_dir(uint32_t *dir_entry);

// enables cr0 registers 31st bit
void enable_system_paging(void);
#endif //PAGE_H
