/*
 * segmentation.h
 * 
 * Created on: Sep 9, 2013
 * 			Author: Amir Yazdanbakhsh <a.yazdanbakhsh@gatech.edu>
 */


#ifndef SEGMENTATION_H_
#define SEGMENTATION_H_

#include "rgbimage.h"

#if 0
typedef struct {
   float r;
   float g;
   float b;
   int n;
} Centroid;
#endif

#define ANNOTATION_CENTROID "no_float 24 8"
#define SIZEOF_CENTROID     (4 * __SIZEOF_FLOAT__)
#define CENTROID(c, i)      (((c)[(i) * 4]))
#define CENTROID_R(c, i)    (((c)[(i) * 4 + 0]))
#define CENTROID_G(c, i)    (((c)[(i) * 4 + 1]))
#define CENTROID_B(c, i)    (((c)[(i) * 4 + 2]))
#define CENTROID2_N(c, i)   (((c)[(i) * 4 + 3]))

typedef struct {
   int k;
   //Centroid* centroids;
   void *centroids;
} Clusters;

int initClusters(Clusters* clusters, int k, float scale);
void segmentImage(RgbImage* image, Clusters* clusters, int n);
void freeClusters(Clusters* clusters);

#endif /* SEGMENTATION_H_ */
