#include <math.h>
#include <stdint.h>

// most of these are single x87 instructions

double
fabs (double x)
{
  __asm__ ("fabs" : "+t"(x));
  return x;
}

float
fabsf (float x)
{
  return fabs (x);
}

double
sqrt (double x)
{
  __asm__ ("fsqrt" : "+t"(x));
  return x;
}

float
sqrtf (float x)
{
  return sqrt (x);
}

double
sin (double x)
{
  __asm__ ("fsin" : "+t"(x));
  return x;
}

double
cos (double x)
{
  __asm__ ("fcos" : "+t"(x));
  return x;
}

double
tan (double x)
{
  // fptan pushes 1.0 after the result
  double one;
  __asm__ ("fptan" : "+t"(x), "=u"(one));
  return x;
}

double
atan2 (double y, double x)
{
  double res;
  __asm__ ("fpatan" : "=t"(res) : "0"(x), "u"(y) : "st(1)");
  return res;
}

double
atan (double x)
{
  return atan2 (x, 1.0);
}

// rounds with the rounding mode rc (bits 10-11 of the control word)
static double
round_with (double x, uint16_t rc)
{
  uint16_t old;
  __asm__ volatile ("fnstcw %0" : "=m"(old));
  uint16_t cw = (old & ~0x0C00) | rc;
  __asm__ volatile ("fldcw %0" : : "m"(cw));
  __asm__ ("frndint" : "+t"(x));
  __asm__ volatile ("fldcw %0" : : "m"(old));
  return x;
}

double
floor (double x)
{
  return round_with (x, 0x0400);
}

double
ceil (double x)
{
  return round_with (x, 0x0800);
}

double
trunc (double x)
{
  return round_with (x, 0x0C00);
}

double
round (double x)
{
  return x < 0 ? -floor (-x + 0.5) : floor (x + 0.5);
}

double
fmod (double x, double y)
{
  // fprem gives a partial remainder, repeat until it is complete
  double res;
  __asm__ ("1: fprem\n\t"
           "fnstsw %%ax\n\t"
           "sahf\n\t"
           "jp 1b"
           : "=t"(res)
           : "0"(x), "u"(y)
           : "ax", "cc");
  return res;
}

double
log2 (double x)
{
  // fyl2x computes y * log2 (x)
  double res;
  __asm__ ("fld1\n\t"
           "fxch\n\t"
           "fyl2x"
           : "=t"(res)
           : "0"(x));
  return res;
}

double
log (double x)
{
  return log2 (x) * 0.69314718055994530942;
}

double
log10 (double x)
{
  return log2 (x) * 0.30102999566398119521;
}

double
exp (double x)
{
  // 2^(x * log2 (e)), split into an integer power (fscale) and a fraction
  // between -1 and 1 (f2xm1)
  double y = x * 1.44269504088896340736;
  double whole = round_with (y, 0x0C00);
  double frac = y - whole;
  double res;
  __asm__ ("f2xm1" : "+t"(frac));
  frac += 1.0;
  __asm__ ("fscale" : "=t"(res) : "0"(frac), "u"(whole));
  return res;
}

double
pow (double base, double exp_)
{
  if (exp_ == 0)
    {
      return 1;
    }
  if (base == 0)
    {
      return 0;
    }
  // whole exponents work with negative bases too
  if (exp_ == (int32_t)exp_ && (exp_ < 0 ? -exp_ : exp_) < 64)
    {
      double res = 1;
      int32_t n = exp_ < 0 ? -exp_ : exp_;
      for (int32_t i = 0; i < n; i++)
        {
          res *= base;
        }
      return exp_ < 0 ? 1 / res : res;
    }
  return exp (exp_ * log (base));
}
