#ifndef PIT_H
#define PIT_H

#include <stdint.h>

// programs pit channel 0 to fire irq0 PIT_FREQUENCY_HZ times a second
void pit_init (uint8_t verbose);

// called from irq0
void pit_tick (void);

// ticks since pit_init
uint32_t pit_ticks (void);

// milliseconds since pit_init
uint32_t pit_uptime_ms (void);

// number of ticks covering at least ms milliseconds
uint32_t pit_ms_to_ticks (uint32_t ms);

#endif // !PIT_H
