#include "./page.h"
#include "../../../drivers/screen/terminal.h"
#include "../../../libc/stdio/stdio.h"
#include "../../../libc/stdlib/stdlib.h"
#include "../../../libc/string/string.h"
#include "../../config.h"

#include <stddef.h>

// prototype for asm function that tells processor where to find the entry
// point of the page directory
void load_page_dir (uint32_t *dir);

// prototype for the function that enables paging using 31th bit of cr0
// register DO NOT CALL BEFORE INITIALIZING PAGE DIRECTORY!!!
void enable_paging (void);

// invalidates the tlb entry of the given virtual address
void flush_tlb_entry (void *vir_addr);

// returns the address that caused the last page fault
uint32_t read_cr2 (void);

static uint32_t *current_dir_entry = 0;

struct page_dir *
page_dir_init (uint32_t flags)
{
  uint32_t *dir_entry
      = calloc (sizeof (uint32_t) * TOTAL_ENTRIES_PER_DIRECTORY);
  // hold offset for each table so they dont overwrite eachother while
  // initializing
  uint32_t offset = 0;
  for (size_t i = 0; i < TOTAL_ENTRIES_PER_DIRECTORY; i++)
    {
      // initialize the tables for the page dir
      uint32_t *table_entry
          = calloc (sizeof (uint32_t) * TOTAL_ENTRIES_PER_TABLE);
      for (size_t j = 0; j < TOTAL_ENTRIES_PER_TABLE; j++)
        {
          // jth entry = offset (aka table number but in bytes) +
          // j * pagesize ( jth page block). | flags is using the last
          // bytes that is not used by address as a metadata for the virmem
          // block
          table_entry[j] = (offset + (j * PAGE_SIZE)) | flags;
        }
      // keeps track of the page
      offset += (TOTAL_ENTRIES_PER_TABLE * PAGE_SIZE);
      // mark table as writable. individual pages might not be writable
      dir_entry[i] = (uint32_t)table_entry | flags | IS_WRITABLE;
    }
  struct page_dir *dir = calloc (sizeof (struct page_dir));
  dir->dir_entry = dir_entry;
  return dir;
}

#define TABLE_SPAN (TOTAL_ENTRIES_PER_TABLE * PAGE_SIZE)

struct page_dir *
page_dir_new_user (struct page_dir *kernel_dir, void *user_addr)
{
  struct page_dir *dir = calloc (sizeof (struct page_dir));
  uint32_t *dir_entry
      = calloc (sizeof (uint32_t) * TOTAL_ENTRIES_PER_DIRECTORY);
  if (!dir || !dir_entry)
    {
      free (dir);
      free (dir_entry);
      return 0;
    }
  // kernel tables are shared. they are supervisor only so ring 3 code
  // can't touch them, but the kernel stays mapped during syscalls
  memcpy (dir_entry, kernel_dir->dir_entry,
          sizeof (uint32_t) * TOTAL_ENTRIES_PER_DIRECTORY);
  // the user part starts empty, its tables come with the first page mapped
  // in them
  dir->user_start = (uint32_t)user_addr & ~(TABLE_SPAN - 1);
  for (uint32_t i = 0; i < USER_SPACE_TABLES; i++)
    {
      dir_entry[dir->user_start / TABLE_SPAN + i] = 0;
    }
  dir->dir_entry = dir_entry;
  return dir;
}

uint32_t *
page_dir_user_entry (struct page_dir *dir, uint32_t vaddr, uint8_t create)
{
  if (vaddr < dir->user_start
      || vaddr - dir->user_start >= USER_SPACE_TABLES * TABLE_SPAN)
    {
      return 0;
    }
  uint32_t idx = (vaddr - dir->user_start) / TABLE_SPAN;
  uint32_t *table = dir->user_tables[idx];
  if (!table)
    {
      if (!create)
        {
          return 0;
        }
      table = calloc (sizeof (uint32_t) * TOTAL_ENTRIES_PER_TABLE);
      if (!table)
        {
          return 0;
        }
      dir->user_tables[idx] = table;
      // not present entries arent cached by the tlb, no flush needed
      dir->dir_entry[dir->user_start / TABLE_SPAN + idx]
          = (uint32_t)table | IS_PRESENT | IS_WRITABLE | ACCESS_ALL;
    }
  return &table[(vaddr / PAGE_SIZE) % TOTAL_ENTRIES_PER_TABLE];
}

void
page_dir_free_user (struct page_dir *dir)
{
  for (uint32_t i = 0; i < USER_SPACE_TABLES; i++)
    {
      free (dir->user_tables[i]);
    }
  free (dir->dir_entry);
  free (dir);
}

uint32_t
page_fault_address (void)
{
  return read_cr2 ();
}

void
switch_page_dir (uint32_t *dir_entry)
{
  load_page_dir (dir_entry);
  current_dir_entry = dir_entry;
}

void
enable_system_paging (uint8_t verbose)
{
  enable_paging ();
  if (verbose)
    {
      printf ("system paging");
      init_OK ();
    }
}

static int32_t
check_memory_alignment (void *addr)
{
  return ((uint32_t)addr % PAGE_SIZE == 0) ? 0 : INVALID_ARG_ERROR;
}

int32_t
set_page_location (struct page_location *page_loc, void *vir_addr)
{
  // return error if memory is not aligned
  if (check_memory_alignment (vir_addr))
    {
      return -INVALID_ARG_ERROR;
    }
  // each directory entry covers 4mb (1024 pages of 4kb) so dividing the
  // vir addr by 4mb results with the directory idx.
  page_loc->dir_idx
      = ((uint32_t)vir_addr / (TOTAL_ENTRIES_PER_DIRECTORY * PAGE_SIZE));
  // result of modulo operator is the address within the page directory.
  // dividing it by page size will result index within the page table
  page_loc->page_idx
      = (((uint32_t)vir_addr % (TOTAL_ENTRIES_PER_TABLE * PAGE_SIZE))
         / PAGE_SIZE);
  return OK;
}

int32_t
virtualize_mem (uint32_t *dir, void *vir_addr, uint32_t phys_addr,
                uint32_t flags)
{
  // return error if memory is not aligned
  if (check_memory_alignment (vir_addr))
    {
      return -INVALID_ARG_ERROR;
    }
  struct page_location page_loc = { .page_idx = 0, .dir_idx = 0 };
  int32_t rs = set_page_location (&page_loc, vir_addr);
  if (rs < 0)
    {
      return rs;
    }
  // map the page table from the page directory.
  uint32_t dir_loc = dir[page_loc.dir_idx];
  // & with 0xfffff000 to get highest 20 bits aka address, zeroing flags
  uint32_t *table_loc = (uint32_t *)(dir_loc & 0xfffff000);
  // map the table's page index to given physical address
  table_loc[page_loc.page_idx] = (phys_addr & 0xfffff000) | flags;
  // cpu may still have the old translation cached
  flush_tlb_entry (vir_addr);
  return OK;
}
