#include "complex.hpp"

#include <cmath>

void fftSinCos(float __attribute((annotate("no_float 20 12 signed range 0 167776 0"))) x,
	       float* __attribute((annotate("range -1 1 0"))) s,
	       float* __attribute((annotate("range -1 1 0"))) c) {
    *s = sin(-2 * PI * x);
    *c = cos(-2 * PI * x);
}

#if 0
float abs(const Complex* x) {
	return sqrt(x->real * x->real + x->imag * x->imag);
}

float arg(const Complex* x) {
	if (x->real > 0)
		return atan(x->imag / x->real);

	if (x->real < 0 && x->imag >= 0)
		return atan(x->imag / x->real) + PI;

	if (x->real < 0 && x->imag < 0)
		return atan(x->imag / x->real) - PI;

	if (x->real == 0 && x->imag > 0)
		return PI / 2;

	if (x->real == 0 && x->imag < 0)
		return -PI / 2;

	if (x->real == 0 && x->imag == 0)
		return 0;

	return 0;
}
#endif
