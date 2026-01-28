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
// only reported as key events, they dont produce chars
#define KEY_CTRL 0x87
#define KEY_SHIFT 0x88
#define KEY_ALT 0x89
#define KEY_F1 0x8A // up to KEY_F1 + 11 for f12
#define KEY_PAGE_UP 0x96
#define KEY_PAGE_DOWN 0x97
#define KEY_INSERT 0x98

// key events: a key code (the unshifted char, or one of the KEY_ codes)
// with KEY_RELEASED set when the key goes up
#define KEY_RELEASED 0x100

// called from irq1. translates the scancode and buffers the char
void keyboard_handle_interrupt (void);

// returns next buffered char or -1 if buffer is empty
int32_t keyboard_getchar (void);

// returns 1 and clears the flag if ctrl+c was pressed since last call
uint8_t keyboard_take_ctrl_c (void);

// drops every buffered char
void keyboard_flush (void);

// returns the next key event or -1 if there is none. events are queued
// next to the chars, the oldest ones get dropped when nobody reads them
int32_t keyboard_get_event (void);

// drops every queued key event
void keyboard_flush_events (void);

#endif // !KEYBOARD_H
