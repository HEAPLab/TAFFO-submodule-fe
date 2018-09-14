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
	
	float getPixel_r(int x, int y) {
	  return ((float*)_pixels)[y * (width * 3) + x * 3 + 0];
	}
	float getPixel_g(int x, int y) {
	  return ((float*)_pixels)[y * (width * 3) + x * 3 + 1];
	}
	float getPixel_b(int x, int y) {
	  return ((float*)_pixels)[y * (width * 3) + x * 3 + 2];
	}
	
	void putPixel_r(int x, int y, float v) {
	  ((float*)_pixels)[y * (width * 3) + x * 3 + 0] = v;
	}
	void putPixel_g(int x, int y, float v) {
	  ((float*)_pixels)[y * (width * 3) + x * 3 + 1] = v;
	}
	void putPixel_b(int x, int y, float v) {
	  ((float*)_pixels)[y * (width * 3) + x * 3 + 2] = v;
	}
} ;

#endif