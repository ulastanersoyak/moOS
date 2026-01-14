#ifndef PAGE_H
#define PAGE_H

#include "../../config.h"
#include <stdint.h>

// page mask bits. used in addresses last bytes since addresses are aligned to
// 4kb and dont need to use last bytes
#define CACHE_DISABLED 0b00010000
#define WRITE_THROUGH 0b00001000
#define ACCESS_ALL 0b00000100
#define IS_WRITABLE 0b00000010
#define IS_PRESENT 0b00000001

struct page_dir
{
  uint32_t *dir_entry;
  // tables of the user address space, owned by a user page directory. 0
  // until a page in them is mapped, always 0 for the kernel directory
  uint32_t *user_tables[USER_SPACE_TABLES];
  uint32_t user_start; // first user address
};

// holds a virtual addresses directory and page indexes to locate it
struct page_location
{
  uint32_t dir_idx;
  uint32_t page_idx;
};

// returns a page directory with 4 gb virtual address space
// with tables and pages that are initialized with given flags
struct page_dir *page_dir_init (uint32_t flags);

// returns a page directory sharing every kernel table, with an empty user
// address space of USER_SPACE_SIZE bytes starting at user_addr (aligned to
// 4mb)
struct page_dir *page_dir_new_user (struct page_dir *kernel_dir,
                                    void *user_addr);

// returns the page table entry of the user page at vaddr, or 0 if vaddr is
// outside the user address space. with create a missing page table is
// allocated, else 0 is returned for it too
uint32_t *page_dir_user_entry (struct page_dir *dir, uint32_t vaddr,
                               uint8_t create);

// frees a directory made by page_dir_new_user and its page tables. the pages
// mapped in them are not freed
void page_dir_free_user (struct page_dir *dir);

// address that caused the last page fault (cr2)
uint32_t page_fault_address (void);

// sets given virtual memory addresses dir and page indexes
// returns 0 if successful
int32_t set_page_location (struct page_location *page_loc, void *vir_addr);

// maps a virtual address to a physical address with given page flags.
// returns 0 if successful
int32_t virtualize_mem (uint32_t *dir, void *vir_addr, uint32_t phys_addr,
                        uint32_t flags);

// asm wrapper functions

// tells processor where to find entry point of paging directory
// by storing pointer to paging dir at cr3 register
void switch_page_dir (uint32_t *dir_entry);

// enables cr0 registers 31st bit
void enable_system_paging (uint8_t verbose);
#endif // PAGE_H
