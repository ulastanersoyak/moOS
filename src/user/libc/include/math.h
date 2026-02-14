#ifndef _MATH_H
#define _MATH_H

// double precision maths on the x87 fpu

#define M_PI 3.14159265358979323846
#define HUGE_VAL __builtin_huge_val ()
#define INFINITY __builtin_inff ()
#define NAN __builtin_nanf ("")

double fabs (double x);
double floor (double x);
double ceil (double x);
double round (double x);
double trunc (double x);
double fmod (double x, double y);
double sqrt (double x);
double sin (double x);
double cos (double x);
double tan (double x);
double atan (double x);
double atan2 (double y, double x);
double exp (double x);
double log (double x);
double log2 (double x);
double log10 (double x);
double pow (double base, double exp);
float fabsf (float x);
float sqrtf (float x);

#endif
