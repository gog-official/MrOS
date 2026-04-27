#ifndef _MATH_H
#define _MATH_H

double sin (double x);
double cos (double x);
double tan (double x);
double atan (double x);
double atan2 (double y, double x);
double sqrt (double x); // squirt ;)
double fabs (double x);
double floor(double x);
double ceil (double x);
double fmod (double x, double y);
double pow (double base, double exp);
double log (double x);

#define M_PI	3.14159265358979323846
#define M_PI_2	1.57079632679489661923
#define HUGE_VAL __builtin_huge_val()


#endif // !_MATH_H
