#ifndef SERIAL_H
#define SERIAL_H

#include <stdint.h>

// initializes com1 at 38400 baud, 8 data bits, no parity, 1 stop bit
void serial_init (void);

// writes a char to com1. \n is sent as \r\n for terminal emulators
void serial_putchar (char c);

#endif // !SERIAL_H
