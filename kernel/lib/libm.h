// minimal math library for doom
// but why????
// doom uses fixed-point math internallly for most thingys, but the renderer setup and angle tables need some trigonometry, sqrt, fabs, floor, ceil, fmod, etc
// and here we are implementing them using lookup tables and int approximations. precision is sufficient for doom, it was designed for 486s lol

#ifndef LIBM_H
#define LIBM_H

double sin (double x);
double cos (double x);
double tan (double x);
double atan(double x);
double atan2(double y, double x);
double sqrt (double x);
double fabs (double x);
double floor (double x);
double ceil (double x);
double fmod (double x, double y);
double pow (double base, double exp);
double log (double x);

// integer square root
unsigned int isqrt(unsigned int n);

#endif // !LIBM_H
