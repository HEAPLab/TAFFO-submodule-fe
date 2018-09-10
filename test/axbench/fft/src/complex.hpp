#ifndef __COMPLEX_HPP__
#define __COMPLEX_HPP__

#define PI 3.1415926535897931

#define COMPLEX_REAL(v,i) v[i*2] 
#define COMPLEX_IMAG(v,i) v[i*2+1] 

typedef struct {
   float real;
   float imag;
} Complex;

void fftSinCos(float x, float* s, float* c);
float abs(const Complex* x);
float arg(const Complex* x);

#endif
