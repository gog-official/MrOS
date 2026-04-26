// implemented without the FPU library btw

#include "libm.h"

// constant
#define PI	3.14159265358979323846
#define TWO_PI  6.28318530717958647692 // idk but 2*PI didnt worked for me, i had to manually add it
#define HALF_PI 1.57079632679489661923

double fabs(double x) {
	return x < 0.0 ? -x : x;
}

double floor(double x) {
	int i = (int)x;
	return (double)(x < 0.0 && (double)i != x) ? (double)(i-1) : (double)i;
}

double ceil(double x) {
	int i = (int)x;
	return (double)(x > 0.0 && (double)i != x) ? (double)(i+1) : (double)i;
}

double fmod(double x, double y) {
	if (y == 0.0) return 0.0;
	int q = (int)(x / y);
	return x - (double)q * y;
}

// we squirting in the Newton-Raphson way
double sqrt(double x) {
	if (x < 0.0) return 0.0;
	if (x == 0.0) return 0.0; // btw this way is recommended rather than x <=0
	double guess = x > 1.0 ?  x / 2.0 : x;
	for (int i = 0; i < 32; i++) {
		double next = 0.5 * (guess + x / guess);
		if (fabs(next - guess) < 1e-10) return next;
		guess = next;
	}
	return guess;
}

// squirting integers
unsigned int isqrt(unsigned int n) { // trust me its integer sqrt, not i squirt :(
	unsigned int x = n;
	unsigned int r = 0;
	unsigned int b = 1u << 30;
	while (b > x) b >>= 2;
	while (b != 0) {
		if (x >= r + b) {
			x-=r+b;
			r=(r >> 1)+b;
		} else {
			r >>= 1;
		}
		b >>= 2;
	}
	return r;

}

// we doing sin via taylor(not swift) series
// taylor(not swift): sin(x) = x - x^3/6 + x^5/120 - x^7/5040 + ... you got it
// first normalize x to [-pi, pi].
// 8 terms gives < 1e-10 error for |x| < pi.

static double normalize_angle(double x) {
	x = fmod(x, TWO_PI); // reduce to [0, 2*PI]
	if (x < 0.0) x += TWO_PI; 
	if (x > PI)
		x -= TWO_PI; // reduce to [-pi, pi]
	return x;
}

double sin(double x) {
	x = normalize_angle(x);
	double term = x;
	double sum = x;
	double x2 = x * x;
	for (int n = 1; n <= 8; n++) {
		term *= -x2 / (double)((2*n) * (2*n+1));
		sum += term;
	}
	return sum;
}

double cos(double x) {
	return sin(x + HALF_PI);
}

double tan(double x) {
	double c = cos(x);
	if (fabs(c) < 1e-15) return 1e-15;
	return sin(x) / c;
}

// this cooked me, atan via chebyshev approximation
// atan(x) for |x| <= 1; polynomial approximation.
// for |x| > 1: atan(x) = PI/2 - atan(1/x).

double atan(double x) {
	int neg = 0, inv = 0;
	if (x < 0.0) { neg = 1; x = -x; }
	if (x > 1.0) { inv = 1; x = 1.0 / x; }

	// chebyshev coefficients for atan on [0,1]
	double x2 = x*x;
	double r = x * (1.0 - x2 * (1.0/3.0 - x2 * (1.0/5.0 - x2 * (1.0/7.0 - x2 * (1.0/9.0 - x2 * (1.0/ 11.0)))))); // typical physics classes, noice
	if (inv) r = HALF_PI - r;
	if (neg) r = -r;
	return r; // my nvim crashed due to that chebyshev :(
}

double atan2(double y, double x) {
	if (x == 0.0) {
		if (y > 0.0) return HALF_PI;
		if (y < 0.0) return -HALF_PI;
		return 0.0;
	}
	double a = atan(y/x);
	if (x < 0.0){
		if (y >= 0.0) return a + PI;
		return a-PI;
	}
	return a; // this was simpler
}

// is power or exponent a good term for this?
double pow(double base, double exp) {
	// integer exponent fast path
	int e = (int)exp;
	if ((double)e == exp) {
		double r = 1.0;
		int neg = e < 0;
		if (neg) e = -e;
		while (e--) r *= base;
		return neg ? 1.0 / r : r;
	}
	// general case, omit for now
	return 0.0;
}

double log(double x) {
	// ln(x) via identity ln(x) = 2*atanh((x-1)/(x+1))
	if (x <= 0.0) return -1e15;
	double y = (x - 1.0) / (x + 1.0);
	double y2 = y*y;
	double r = y;
	double term = y;
	for (int n = 1; n <= 16; n++) {
		term *= y2;
		r += term / (double)(2*n + 1);
	}
	return 2.0 * r;
}
