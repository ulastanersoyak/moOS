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
  uint32_t *dir = kcalloc(sizeof(uint32_t) * TOTAL_ENTRIES_PER_DIRECTORY);

  uint32_t offset = 0; // holds page entries offsets

  for (size_t i = 0; i < TOTAL_ENTRIES_PER_DIRECTORY; i++) {

    // set each page table which contains 1024 page (each page containing a 4kb
    // memory block)
    uint32_t *table_entry = kcalloc(sizeof(uint32_t) * TOTAL_ENTRIES_PER_TABLE);

    for (size_t j = 0; j < TOTAL_ENTRIES_PER_TABLE; j++) {
      // offset holds page entries offset from start of the page dir. j hold the
      // pages offset within the current page table. adding paging size allows
      // writing to each page on current table . setting each table with given
      // flags
      table_entry[j] = (offset + (j + PAGE_SIZE)) | flags;
    }
    // add offset by a page so the next page doesnt override previous ones
    // physical memory
    offset += (TOTAL_ENTRIES_PER_TABLE * PAGE_SIZE);

    // set the page entry to writable. doesnt mean each page is writable. it
    // just makes the table writable
    dir[i] = (uint32_t)table_entry | flags | IS_WRITABLE;
  }

  struct page_dir *paging_directory = kcalloc(sizeof(struct page_dir));

  paging_directory->dir_entry = dir;

  terminal_writestring("<");
  terminal_writeint((TOTAL_ENTRIES_PER_DIRECTORY / 1024) *
                    (TOTAL_ENTRIES_PER_TABLE / 1024) * PAGE_SIZE);
  terminal_writestring(" gb>");
  init_OK();
  return paging_directory;
}

uint32_t *get_current_dir(struct page_dir *dir) { return dir->dir_entry; }

void switch_page_dir(uint32_t *dir) {
  load_page_dir(dir);
  current_dir = dir;
}

int32_t get_paging_index(void *vir_addr, struct paging_addr *addr) {
  // check if given address aligns
  if (!(((uint32_t)vir_addr % PAGE_SIZE) == 0)) {
    return -INVALID_ARG_ERROR;
  }
  *(addr->dir_idx) =
      ((uint32_t)vir_addr / (TOTAL_ENTRIES_PER_DIRECTORY * PAGE_SIZE));
  *(addr->table_idx) =
      (((uint32_t)vir_addr % (TOTAL_ENTRIES_PER_TABLE * PAGE_SIZE)) /
       PAGE_SIZE);

  return OK;
}

int32_t set_page(uint32_t *page_dir, void *vir_addr, uint32_t val) {
  if (!(((uint32_t)vir_addr % PAGE_SIZE) == 0)) {
    return -INVALID_ARG_ERROR;
  }
  struct paging_addr page_addr = {.table_idx = 0, .dir_idx = 0};
  int32_t rs = get_paging_index(vir_addr, &page_addr);

  if (rs < 0) {
    return rs;
  }
  uint32_t dir_entry = page_dir[*(page_addr.dir_idx)];
  uint32_t *table_entry = (uint32_t *)(dir_entry & 0xfffff000);
  table_entry[*(page_addr.table_idx)] = val;
  return OK;
}
