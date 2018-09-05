
#include "fourier.hpp"
#include <cmath>
#include <fstream>
#include <map>

void calcFftIndices(int K __attribute((annotate("range 0 100 0"))),
		    int* indices __attribute((annotate("0 100 0"))))
{
	int __attribute((annotate("range 0 100 0"))) i, __attribute((annotate("range 0 100 0"))) j;
	int  __attribute((annotate("range 0 100 0"))) N;

	N = (int)log2f(K) ;

	indices[0] = 0 ;
	indices[1 << 0] = 1 << (N - (0 + 1)) ;
	for (i = 1; i < N; ++i)
	{
		for(j = (1 << i) ; j < (1 << (i + 1)); ++j)
		{
			indices[j] = indices[j - (1 << i)] + (1 << (N - (i + 1))) ;
		}
	}
}

void radix2DitCooleyTykeyFft(int K __attribute((annotate("range 0 100 0"))),
			     int* indices __attribute((annotate("range 0 100 0"))),
			     Complex* x __attribute((annotate("range 0 100 0"))),
			     Complex* f __attribute((annotate("range 0 100 0"))))
{

	calcFftIndices(K, indices) ;

	int step ;
	float __attribute((annotate("no_float 1 31 unsigned 0 2048 1e-10"))) arg ;
	int eI ;
	int oI ;

	float __attribute((annotate("no_float 20 12 signed -1.0 1.0 1e-4"))) fftSin;
	float __attribute((annotate("no_float 20 12 signed -1.0 1.0 1e-4"))) fftCos;

	Complex __attribute((annotate("range 0 100 0"))) t;
	int i ;
	int __attribute((annotate("range 1 100 0"))) N ;
	int j ;
	int __attribute((annotate("range 0 100 0"))) k ;

	double __attribute((annotate("range -1.0 1.0"))) dataIn[1];
	double __attribute((annotate("target range -1.0 1.0"))) dataOut[2];

	for(i = 0, N = 1 << (i + 1); N <= K ; i++, N = 1 << (i + 1))
	{
		for(j = 0 ; j < K ; j += N)
		{
			step = N >> 1 ;
			for(k = 0; k < step ; k++)
			{
				arg = (float)k / N ;
				eI = j + k ; 
				oI = j + step + k ;

				dataIn[0] = arg;

#pragma parrot(input, "fft", [1]dataIn)

				fftSinCos(arg, &fftSin, &fftCos);

				dataOut[0] = fftSin;
				dataOut[1] = fftCos;

#pragma parrot(output, "fft", [2]<0.0; 2.0>dataOut)

				fftSin = dataOut[0];
				fftCos = dataOut[1];


				// Non-approximate
				t =  x[indices[eI]] ;
				x[indices[eI]].real = t.real + (x[indices[oI]].real * fftCos - x[indices[oI]].imag * fftSin);
                x[indices[eI]].imag = t.imag + (x[indices[oI]].imag * fftCos + x[indices[oI]].real * fftSin);

                x[indices[oI]].real = t.real - (x[indices[oI]].real * fftCos - x[indices[oI]].imag * fftSin);
                x[indices[oI]].imag = t.imag - (x[indices[oI]].imag * fftCos + x[indices[oI]].real * fftSin);
			}
		}
	}

	for (int i = 0 ; i < K ; i++)
	{
		f[i] = x[indices[i]] ;
	}
}
