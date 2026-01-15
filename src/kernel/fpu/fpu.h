#ifndef FPU_H
#define FPU_H

#include <stdint.h>

// size of the state fnsave stores
#define FPU_STATE_SIZE 108

// state of the x87 fpu of a task, swapped on every task switch
struct fpu_state
{
  uint8_t data[FPU_STATE_SIZE];
};

// turns the fpu on and keeps its reset state for new tasks
void fpu_init (uint8_t verbose);

// copies the state of a freshly reset fpu into state
void fpu_init_state (struct fpu_state *state);

// asm functions
void fpu_save (struct fpu_state *state);
void fpu_restore (const struct fpu_state *state);

#endif // !FPU_H
