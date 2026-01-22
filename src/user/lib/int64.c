#include <stdint.h>

// 64 bit division on a 32 bit cpu. the compiler turns / and % of 64 bit
// numbers into calls to these, they normally come from libgcc or
// compiler-rt. they must not use 64 bit / or % themselves

static uint64_t
udivmod (uint64_t num, uint64_t den, uint64_t *rem)
{
  if (den == 0)
    {
      // like a 32 bit division by zero, the task gets killed
      volatile uint32_t zero = 0;
      return 1 / zero;
    }
  uint64_t quot = 0;
  uint64_t r = 0;
  // shift and subtract, one bit of the quotient per round
  for (int bit = 63; bit >= 0; bit--)
    {
      r = (r << 1) | ((num >> bit) & 1);
      if (r >= den)
        {
          r -= den;
          quot |= (uint64_t)1 << bit;
        }
    }
  if (rem)
    {
      *rem = r;
    }
  return quot;
}

uint64_t
__udivdi3 (uint64_t num, uint64_t den)
{
  return udivmod (num, den, 0);
}

uint64_t
__umoddi3 (uint64_t num, uint64_t den)
{
  uint64_t rem;
  udivmod (num, den, &rem);
  return rem;
}

// signed versions round towards zero, the remainder has the sign of num
int64_t
__divdi3 (int64_t num, int64_t den)
{
  uint64_t q = udivmod (num < 0 ? -(uint64_t)num : (uint64_t)num,
                        den < 0 ? -(uint64_t)den : (uint64_t)den, 0);
  return (num < 0) != (den < 0) ? -(int64_t)q : (int64_t)q;
}

int64_t
__moddi3 (int64_t num, int64_t den)
{
  uint64_t rem;
  udivmod (num < 0 ? -(uint64_t)num : (uint64_t)num,
           den < 0 ? -(uint64_t)den : (uint64_t)den, &rem);
  return num < 0 ? -(int64_t)rem : (int64_t)rem;
}
