#include "./page.h"
#include "../../libc/string/string.h"
#include "../../kernel/kmem/kheap.h"
#include "../../kernel/config.h"
#include "../../drivers/screen/terminal.h"

#include <stddef.h>

// prototype for asm function that tells processor where to find the entry point
// of the page directory
void load_page_dir(uint32_t *dir);

static uint32_t *current_dir_entry = 0;

struct page_dir *page_dir_init(uint32_t flags){
  uint32_t *dir_entry= kcalloc(sizeof(uint32_t) * TOTAL_ENTRIES_PER_DIRECTORY);
  // hold offset for each table so they dont overwrite eachother while initializing
  uint32_t offset = 0;
  for(size_t i = 0; i < TOTAL_ENTRIES_PER_DIRECTORY; i++){
    // initialize the tables for the page dir
    uint32_t *table_entry = kcalloc(sizeof(uint32_t) * TOTAL_ENTRIES_PER_TABLE); 
    for(size_t j = 0; j < TOTAL_ENTRIES_PER_TABLE; j++){
      // jth entry = offset (aka table number but in bytes) + 
      // j * pagesize ( jth page block). | flags is using the last
      // bytes that is not used by address as a metadata for the virmem block
      table_entry[j] = (offset + (j * PAGE_SIZE)) | flags;
    } 
    // keeps track of the page 
    offset += (TOTAL_ENTRIES_PER_TABLE * PAGE_SIZE);
    // mark table as writable. individual pages might not be writable
    dir_entry[i] = (uint32_t)table_entry | flags | IS_WRITABLE;
  }
  struct page_dir *dir = kcalloc(sizeof(struct page_dir));
  dir->dir_entry = dir_entry;
  return dir;
}

void switch_page_dir(uint32_t *dir_entry){
  load_page_dir(dir_entry);
  current_dir_entry = dir_entry;
}