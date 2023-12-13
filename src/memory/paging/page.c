#include "page.h"
#include "../../drivers/screen/terminal.h"
#include "../../kernel/config.h"
#include "../../kernel/kmem/kheap.h"

#include <stddef.h>

void load_page_dir(uint32_t *dir);

void enable_paging(void) {
  terminal_writestring("enabling paging");
  void enable_paging_asm(void);
  init_OK();
}

// holds current page directory locally
static uint32_t *current_dir = 0;

// initialize page directory with given flags
struct page_dir *page_dir_init(uint8_t flags) {
  terminal_writestring("paging dir initializing");
  // set the page directory which contains 1024 page table entry points
  uint32_t *dir =
      kcalloc(sizeof(uint32_t) * PAGING_TOTAL_ENTRIES_PER_DIRECTORY);

  uint32_t offset =
      0; // holds page entries offsets so they are mapped accordingly
  for (size_t i = 0; i < PAGING_TOTAL_ENTRIES_PER_DIRECTORY; i++) {

    // set each page table which contains 1024 page (each page containing a 4kb
    // memory block)
    uint32_t *table_entry =
        kcalloc(sizeof(uint32_t) * PAGING_TOTAL_ENTRIES_PER_TABLE);

    for (size_t j = 0; j < PAGING_TOTAL_ENTRIES_PER_TABLE; j++) {
      // offset holds page entries offset from start of the page dir. j hold the
      // pages offset within the current page table. adding paging size allows
      // writing to each page on current table . setting each table with given
      // flags
      table_entry[j] = (offset + (j + PAGING_PAGE_SIZE)) || flags;
    }
    // add offset by a page so the next page doesnt override previous ones
    // physical memory
    offset += (PAGING_TOTAL_ENTRIES_PER_TABLE * PAGING_PAGE_SIZE);

    // set the page entry to writable. doesnt mean each page is writable. it
    // just makes the table writable
    dir[i] = (uint32_t)table_entry | flags | PAGING_IS_WRITABLE;
  }

  struct page_dir *paging_directory = kcalloc(sizeof(struct page_dir));

  paging_directory->dir_entry = dir;

  terminal_writestring("<");
  terminal_writeint((PAGING_TOTAL_ENTRIES_PER_DIRECTORY / 1024) *
                    (PAGING_TOTAL_ENTRIES_PER_TABLE / 1024) * PAGING_PAGE_SIZE);
  terminal_writestring(" gb>");
  init_OK();
  return paging_directory;
}

uint32_t *get_dir(struct page_dir *dir) { return dir->dir_entry; }

void switch_page(uint32_t *dir) {
  load_page_dir(dir);
  current_dir = dir;
}
