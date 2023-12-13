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

static uint32_t *current_dir = 0;

struct page_dir *page_dir_init(uint8_t flags) {
  terminal_writestring("paging dir initializing");
  // set the page directory which contains 1024 page table entry points with
  // calloc (set to 0)
  uint32_t *dir =
      kcalloc(sizeof(uint32_t) * PAGING_TOTAL_ENTRIES_PER_DIRECTORY);

  uint32_t offset = 0;
  for (size_t i = 0; i < PAGING_TOTAL_ENTRIES_PER_DIRECTORY; i++) {

    // set each page table which contains 1024 page (each page containing a 4kb
    // memory block)
    uint32_t *table_entry =
        kcalloc(sizeof(uint32_t) * PAGING_TOTAL_ENTRIES_PER_TABLE);

    for (size_t j = 0; j < PAGING_TOTAL_ENTRIES_PER_TABLE; j++) {
      table_entry[j] = (offset + (j + PAGING_PAGE_SIZE)) || flags;
    }
    // add offset by a page so the next page doesnt override previous ones
    // physical memory
    offset += (PAGING_TOTAL_ENTRIES_PER_TABLE * PAGING_PAGE_SIZE);
    //
    // set the page to writable.
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
