// the tiny c compiler (external/tinycc) built as one source file, for moos
// and as a cross compiler on the host. its own sources are included here so
// moos can change its defaults without patching them
#define ONE_SOURCE 1
#include "tcc.h"

// tcc -run needs mmap, mprotect and signals
#undef TCC_IS_NATIVE

#include "libtcc.c"

// moos programs are static and linked at USER_CODE_ADDR (see
// src/kernel/config.h and src/user/linker.ld). -Ttext still overrides it
static int
moos_parse_args (TCCState *s, int *argc, char ***argv)
{
  s->static_link = 1;
  s->text_addr = 0x40000000;
  s->has_text_addr = 1;
  return tcc_parse_args (s, argc, argv);
}

// tcc.c has main, it calls moos_parse_args and doesnt include libtcc.c again
#define tcc_parse_args moos_parse_args
#undef ONE_SOURCE
#define ONE_SOURCE 0
#include "tcc.c"
