/*
 * convolution.hpp
 * 
 * Created on: Sep 9, 2013
 *          Author: Amir Yazdanbakhsh <a.yazdanbakhsh@gatech.edu>
 */

 #ifndef __CONVOLUTION_HPP__
 #define __CONVOLUTION_HPP__

#include "rgb_image.hpp"
#include <iostream>

 float sobel(float w[][3]) ;

 #define WINDOW(imagePtr, x, y, window) {                   \
    window[0][0] = imagePtr->getPixel_r( x - 1 ,  y - 1 ) ;  \
    window[0][1] = imagePtr->getPixel_r( x ,  y - 1 ) ;      \
    window[0][2] = imagePtr->getPixel_r( x + 1 ,  y - 1 ) ;  \
\
    window[1][0] = imagePtr->getPixel_r( x - 1 ,  y ) ;      \
    window[1][1] = imagePtr->getPixel_r( x ,  y ) ;          \
    window[1][2] = imagePtr->getPixel_r( x + 1 ,  y ) ;      \
\
    window[2][0] = imagePtr->getPixel_r( x - 1 ,  y + 1 ) ;  \
    window[2][1] = imagePtr->getPixel_r( x ,  y + 1 ) ;      \
    window[2][2] = imagePtr->getPixel_r( x + 1 ,  y + 1 ) ;  \
}

#define HALF_WINDOW(imagePtr, x, y, window) {                                                                           \
    window[0][0] = (x == 0 || y == 0                                        ) ? 0 : imagePtr->getPixel_r(x - 1, y - 1);  \
    window[0][1] = (y == 0                                                  ) ? 0 : imagePtr->getPixel_r(x, y - 1);      \
    window[0][2] = (x == imagePtr->width -1 || y == 0                       ) ? 0 : imagePtr->getPixel_r(x + 1, y - 1);  \
\
    window[1][0] = (x == 0                                                  ) ? 0 : imagePtr->getPixel_r(x - 1, y);      \
    window[1][1] =                                                                  imagePtr->getPixel_r(x, y);          \
    window[1][2] = (x == imagePtr->width -1                                 ) ? 0 : imagePtr->getPixel_r(x + 1, y);      \
\
    window[2][0] = (x == 0 || y == imagePtr->height - 1                     ) ? 0 : imagePtr->getPixel_r(x - 1, y + 1);  \
    window[2][1] = (y == imagePtr->height - 1                               ) ? 0 : imagePtr->getPixel_r(x, y + 1);      \
    window[2][2] = (x == imagePtr->width -1 || y == imagePtr->height - 1    ) ? 0 : imagePtr->getPixel_r(x + 1, y + 1);  \
}

 #endif