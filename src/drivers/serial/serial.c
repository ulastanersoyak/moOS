#include "serial.h"
#include "../../kernel/io/io.h"

#define COM1 0x3F8

static uint8_t serial_ready = 0;

void
serial_init (void)
{
  outb (COM1 + 1, 0x00); // disable serial interrupts
  outb (COM1 + 3, 0x80); // enable divisor latch to set baud rate
  outb (COM1 + 0, 0x03); // divisor 3 (lo byte) -> 115200 / 3 = 38400 baud
  outb (COM1 + 1, 0x00); //            (hi byte)
  outb (COM1 + 3, 0x03); // 8 bits, no parity, one stop bit
  outb (COM1 + 2, 0xC7); // enable and clear fifo, 14 byte threshold
  outb (COM1 + 4, 0x0B); // data terminal ready, request to send
  serial_ready = 1;
}

static void
serial_write (uint8_t c)
{
  // wait until transmit holding register is empty
  while (!(insb (COM1 + 5) & 0x20))
    {
    }
  outb (COM1, c);
}

void
serial_putchar (char c)
{
  if (!serial_ready)
    {
      return;
    }
  if (c == '\n')
    {
      serial_write ('\r');
    }
  serial_write (c);
}
