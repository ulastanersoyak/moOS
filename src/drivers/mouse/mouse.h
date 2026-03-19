#ifndef MOUSE_H
#define MOUSE_H

#include <stdint.h>

// buttons of a mouse event
#define MOUSE_LEFT 0x1
#define MOUSE_RIGHT 0x2
#define MOUSE_MIDDLE 0x4

// movement since the last event, positive dy goes down the screen. must
// match struct mouse_event in src/user/lib/moos.h
struct mouse_event
{
  int16_t dx;
  int16_t dy;
  uint8_t buttons; // MOUSE_ bits held down
  uint8_t reserved[3];
};

// turns on the ps/2 mouse port of the keyboard controller and lets the
// mouse send its movement through irq12. call with interrupts off
void mouse_init (uint8_t verbose);

// called from irq12, collects the 3 byte packets of the mouse
void mouse_handle_interrupt (void);

// takes the next event, returns 0 or -1 if there is none. events are dropped
// when nobody reads them, the oldest first
int32_t mouse_get_event (struct mouse_event *out);

// drops every queued event
void mouse_flush_events (void);

#endif // !MOUSE_H
