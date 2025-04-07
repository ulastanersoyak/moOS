#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>

// char pushed to the input buffer for ctrl+c and ctrl+d
#define KEY_CTRL_C 0x03
#define KEY_CTRL_D 0x04

// navigation keys are pushed as codes above the ascii range
#define KEY_UP 0x80
#define KEY_DOWN 0x81
#define KEY_LEFT 0x82
#define KEY_RIGHT 0x83
#define KEY_HOME 0x84
#define KEY_END 0x85
#define KEY_DELETE 0x86

// called from irq1. translates the scancode and buffers the char
void keyboard_handle_interrupt (void);

// returns next buffered char or -1 if buffer is empty
int32_t keyboard_getchar (void);

// returns 1 and clears the flag if ctrl+c was pressed since last call
uint8_t keyboard_take_ctrl_c (void);

// drops every buffered char
void keyboard_flush (void);

#endif // !KEYBOARD_H
