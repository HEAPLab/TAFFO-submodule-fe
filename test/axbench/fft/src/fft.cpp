#include <cstdio>
#include <iostream>
#include "fourier.hpp"
#include <fstream>
#include <time.h>
#include "benchmark.hpp"

static int* indices;
static float* __attribute((annotate("no_float 20 12"))) x;
static float* __attribute((annotate("no_float 20 12"))) f;

int main(int argc, char* argv[])
{
	int i ;

	int n 						= atoi(argv[1]);
	std::string outputFilename 	= argv[2];

	// prepare the output file for writting the theta values
	std::ofstream outputFileHandler;
	outputFileHandler.open(outputFilename);
	outputFileHandler.precision(5);

	// create the arrays
	x 		= (float*)malloc(n * 2 * sizeof (float));
	f 		= (float*)malloc(n * 2 * sizeof (float));
	indices = (int*)malloc(n * sizeof (int));

	if(x == NULL || f == NULL || indices == NULL)
	{
		std::cout << "cannot allocate memory for the triangle coordinates!" << std::endl;
		return -1 ;
	}

	int K = n;
	
	AxBenchTimer timer;

	for(i = 0;i < K ; i++)
	{
		COMPLEX_REAL(x,i) = i;
		COMPLEX_IMAG(x,i) = 0 ;
	}
	radix2DitCooleyTykeyFft(K, indices, x, f) ;
	
	uint64_t time = timer.nanosecondsSinceInit();
	std::cout << "kernel time = " << ((double)time) / 1000000000.0 << " s\n";
	
	for(i = 0;i < K ; i++)
	{
	  outputFileHandler << COMPLEX_REAL(f,i) << " " << COMPLEX_IMAG(f,i) << " " << indices[i] << std::endl;
	}


	outputFileHandler.close();

	return 0 ;
}
