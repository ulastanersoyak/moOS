#include "pit.h"
#include "../../kernel/config.h"
#include "../../kernel/io/io.h"
#include "../../libc/stdio/stdio.h"
#include "../screen/terminal.h"

#define PIT_CHANNEL0 0x40
#define PIT_COMMAND 0x43
#define PIT_BASE_FREQUENCY 1193182

static volatile uint32_t ticks = 0;

void
pit_init (uint8_t verbose)
{
  uint32_t divisor = PIT_BASE_FREQUENCY / PIT_FREQUENCY_HZ;
  // channel 0, lo/hi byte access, square wave generator
  outb (PIT_COMMAND, 0x36);
  outb (PIT_CHANNEL0, divisor & 0xFF);
  outb (PIT_CHANNEL0, (divisor >> 8) & 0xFF);
  if (verbose)
    {
      printf ("pit timer init <%d hz>", PIT_FREQUENCY_HZ);
      init_OK ();
    }
}

void
pit_tick (void)
{
  ticks++;
}

uint32_t
pit_ticks (void)
{
  return ticks;
}

uint32_t
pit_uptime_ms (void)
{
  return ticks * (1000 / PIT_FREQUENCY_HZ);
}

uint32_t
pit_ms_to_ticks (uint32_t ms)
{
  return (ms * PIT_FREQUENCY_HZ + 999) / 1000;
}
