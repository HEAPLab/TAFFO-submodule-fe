/*
 * rgb_image.cpp
 * 
 * Created on: Sep 9, 2013
 * 			Author: Amir Yazdanbakhsh <a.yazdanbakhsh@gatech.edu>
 */

#include "rgb_image.hpp"
#include <cstdlib>

void Image::printPixel(int x, int y)
{
	std::cout << "# Red: 	" << this->getPixel_r(x, y) << std::endl;
	std::cout << "# Green: 	" << this->getPixel_g(x, y) << std::endl;
	std::cout << "# Blue: 	" << this->getPixel_b(x, y) << std::endl; 
}


const char *readInt(const char *line, int *out)
{
  while (*line != '\0') {
    if ('0' <= *line && *line <= '9') {
      char *end;
      *out = strtol(line, &end, 0);
      return end;
    } else {
      line++;
    }
  }
  return nullptr;
}


void tokenize(std::vector<int>& out, std::string& line)
{
  const char *str = line.c_str();
  int v;
  str = readInt(str, &v);
  while (str) {
    out.push_back(v);
    str = readInt(str, &v);
  }
}


int Image::loadRgbImage(std::string filename)
{
	std::ifstream imageFile ;

	if(DEBUG)
		std::cout << "# Loading " << filename << " ..." << std::endl ;

	imageFile.open(filename.c_str()) ;
	if(! imageFile.is_open())
	{
		std::cerr << "# Error openning the file!" << std::endl ;
		return 0 ;
	}

	// Read first line and split based on the , and any spaces before or after
	std::string line ;
	std::getline(imageFile, line) ;
	std::vector<int> imageInfo ;
	tokenize(imageInfo, line);
	this->width  = imageInfo[0];
	this->height = imageInfo[1];

	if(DEBUG)
	{
		std::cout << "# Width:  " << this->width ;
		std::cout << "# Height: " << this->height  << std::endl ;
	} 

  this->_pixels = (void*)malloc(this->height * (this->width * 3) * sizeof(float));

	// We assume there is a newline after each row
	for (int h = 0 ; h < this->height ; h++)
	{
		std::getline(imageFile, line) ;
		std::vector<int> currRowString;
		tokenize(currRowString, line);

		for(int w = 0 ; w < this->width ; w++)
		{
			int index = w * 3 ;
			float r = currRowString[index++];
			float g = currRowString[index++];
			float b = currRowString[index++];
			// Add pixel to the current row
			putPixel_r(w, h, r);
			putPixel_g(w, h, g);
			putPixel_b(w, h, b);
		}
	}

	std::getline(imageFile, line) ;
	this->meta = line ;
	return 1 ;
}

int Image::saveRgbImage(std::string outFilename, float __attribute((annotate("no_float 12 20"))) scale)
{
	if(DEBUG)
	{
		std::cout << "# Saving into " << outFilename << " ..." << std::endl ;
	}

	std::ofstream outFile ;
	outFile.open(outFilename.c_str()) ;

	outFile << this->width << "," << this->height << std::endl ;

	for(int h = 0 ; h < this->height ; h++)
	{
		for(int w = 0 ; w < (this->width - 1); w++)
		{
			// Write Red
			int red   = (int)(this->getPixel_r(w, h) * scale) ;
			int green = (int)(this->getPixel_g(w, h) * scale) ;
			int blue  = (int)(this->getPixel_b(w, h) * scale) ;

			//if ( red > 255 )
		//		red = 255 ;
		//	if ( green > 255 )
		//		green = 255 ;
		///	if ( blue > 255 )
			//	blue = 255 ;
			outFile << red << "," ;
			// Write Green
			outFile << green << "," ;
			// Write Blue
			outFile << blue << "," ;
			
		}

		int red   = (int)(this->getPixel_r(this->width - 1, h) * scale) ;
		int green = (int)(this->getPixel_g(this->width - 1, h) * scale);
		int blue  = (int)(this->getPixel_b(this->width - 1, h) * scale) ;


		// Write Red
		outFile << red  << "," ;
		// Write Green
		outFile << green << "," ;
		// Write Blue
		outFile << blue << std::endl ;
	}

	// Print the meta information
	outFile << this->meta ;
	outFile.close() ;
	return 1 ;
}

void Image::makeGrayscale()
{

	float __attribute((annotate("no_float 12 20 signed 0 1"))) luminance ;

	float __attribute((annotate("no_float 9 23 signed 1.171875e-3 1.171875e-3"))) rC = 0.30 / 256.0 ;
	float __attribute((annotate("no_float 9 23 signed 2.304688e-3 2.304688e-3"))) gC = 0.59 / 256.0 ;
	float __attribute((annotate("no_float 9 23 signed 4.29687e-4 4.29687e-4"))) bC = 0.11 / 256.0 ;

	for(int h = 0 ; h < this->height ; h++)
	{
		for(int w = 0 ; w < this->width ; w++)
		{
			luminance = ( rC * this->getPixel_b(w, h) ) + 
						( gC * this->getPixel_g(w, h) ) + 
						( bC * this->getPixel_b(w, h) ) ;

			this->putPixel_r(w, h, luminance) ;
			this->putPixel_g(w, h, luminance) ; 
			this->putPixel_b(w, h, luminance) ; 
		}
	}
}
