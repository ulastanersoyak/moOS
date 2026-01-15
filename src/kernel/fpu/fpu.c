#include "fpu.h"
#include "../../drivers/screen/terminal.h"
#include "../../libc/stdio/stdio.h"
#include "../../libc/string/string.h"

void fpu_enable (void);

static struct fpu_state reset_state;

void
fpu_init (uint8_t verbose)
{
  fpu_enable ();
  // fnsave resets the fpu after saving, the saved state is a clean one
  fpu_save (&reset_state);
  if (verbose)
    {
      printf ("x87 fpu init");
      init_OK ();
    }
}

void
fpu_init_state (struct fpu_state *state)
{
  memcpy (state, &reset_state, sizeof (*state));
}
