/*
 * rgbimage.h
 * 
 * Created on: Sep 9, 2013
 * 			Author: Amir Yazdanbakhsh <a.yazdanbakhsh@gatech.edu>
 */


#ifndef RGB_IMAGE_H_
#define RGB_IMAGE_H_

#if __SIZEOF_INT__ != __SIZEOF_FLOAT__
#error houston we have a problem, the struct flattening hack would fail!!
#endif

#if 0
typedef struct {
   float r;
   float g;
   float b;
   int cluster;
   float distance;
} RgbPixel;
#endif


#define SIZEOF_RGBPIXEL             (5*__SIZEOF_FLOAT__)
#define RGBPIXEL(pix, i)            (((pix)[((i)) * 5 + 0]))
#define RGBPIXEL_R(pix, i)          (((pix)[((i)) * 5 + 0]))
#define RGBPIXEL_G(pix, i)          (((pix)[((i)) * 5 + 1]))
#define RGBPIXEL_B(pix, i)          (((pix)[((i)) * 5 + 2]))
#define RGBPIXEL2_CLUSTER(pix2, i)  (((pix2)[((i)) * 5 + 3]))
#define RGBPIXEL_DISTANCE(pix, i)   (((pix)[((i)) * 5 + 4]))


typedef struct {
   int w;
   int h;
   void *pixels;
   char* meta;
} RgbImage;

void initRgbImage(RgbImage* image);
int loadRgbImage(const char* fileName, RgbImage* image, float scale);
int saveRgbImage(RgbImage* image, const char* fileName, float scale);
void freeRgbImage(RgbImage* image);

void makeGrayscale(RgbImage* rgbImage);

#endif /* RGB_IMAGE_H_ */
