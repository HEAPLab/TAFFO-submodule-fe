/*
 * rgb_image.hpp
 * 
 * Created on: Sep 9, 2013
 * 			Author: Amir Yazdanbakhsh <a.yazdanbakhsh@gatech.edu>
 */

#ifndef __RGB_IMAGE_HPP__
#define __RGB_IMAGE_HPP__

#include <vector>
#include <fstream>
#include <stdlib.h>
#include <iostream>


#define DEBUG 0

/*
class Pixel {
public:
	Pixel (float r, float g, float b)
	{
		this->r = r ;
		this->g = g ;
		this->b = b ;
	}
	float r ;
	float g ;
	float b ;
} ;
*/

#define ANNOTATION_PIXEL "no_float 8 24 unsigned"

class Image {
public:
	int 			width ;
	int 			height ;
	void     *_pixels;
	//std::vector 	<std::vector<boost::shared_ptr<Pixel> > > pixels ;
	std::string 	meta ;

	// Constructor
	Image()
	{
		this->width  = 0 ;
		this->height = 0 ;
	}

	int loadRgbImage (std::string filename) ;
	int saveRgbImage (std::string outFilename, float scale) ;
	void makeGrayscale() ;
	void printPixel(int x, int y) ;
	
	float getPixel_r(int x, int y) __attribute((always_inline)) {
	  float *pixels __attribute((annotate(ANNOTATION_PIXEL))) = ((float*)_pixels);
	  return pixels[y * (width * 3) + x * 3 + 0];
	}
	inline float getPixel_g(int x, int y)  __attribute((always_inline)) {
	  float *pixels __attribute((annotate(ANNOTATION_PIXEL))) = ((float*)_pixels);
	  return pixels[y * (width * 3) + x * 3 + 1];
	}
	inline float getPixel_b(int x, int y)  __attribute((always_inline)) {
	  float *pixels __attribute((annotate(ANNOTATION_PIXEL))) = ((float*)_pixels);
	  return pixels[y * (width * 3) + x * 3 + 2];
	}
	
	inline void putPixel_r(int x, int y, float __attribute((annotate(ANNOTATION_PIXEL))) v)  __attribute((always_inline)) {
	  float *pixels __attribute((annotate(ANNOTATION_PIXEL))) = ((float*)_pixels);
	  pixels[y * (width * 3) + x * 3 + 0] = v;
	}
	inline void putPixel_g(int x, int y, float __attribute((annotate(ANNOTATION_PIXEL))) v)  __attribute((always_inline)) {
	  float *pixels __attribute((annotate(ANNOTATION_PIXEL))) = ((float*)_pixels);
	  pixels[y * (width * 3) + x * 3 + 1] = v;
	}
	inline void putPixel_b(int x, int y, float __attribute((annotate(ANNOTATION_PIXEL))) v)  __attribute((always_inline)) {
	  float *pixels __attribute((annotate(ANNOTATION_PIXEL))) = ((float*)_pixels);
	  pixels[y * (width * 3) + x * 3 + 2] = v;
	}
} ;

#endif

