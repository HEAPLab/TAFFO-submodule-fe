/*
 * distance.c
 * 
 * Created on: Sep 9, 2013
 * 			Author: Amir Yazdanbakhsh <a.yazdanbakhsh@gatech.edu>
 */


#include "distance.h"
#include <math.h>
#include <map>

int count = 0;
#define MAX_COUNT 1200000

float euclideanDistance(float* p, float* c1) {
	float r;

	r = 0;
	double r_tmp;
	
	double dataIn[6];
	dataIn[0] = RGBPIXEL_R(p, 0);
	dataIn[1] = RGBPIXEL_G(p, 0);
	dataIn[2] = RGBPIXEL_B(p, 0);
	dataIn[3] = CENTROID_R(c1, 0);
	dataIn[4] = CENTROID_G(c1, 0);
	dataIn[5] = CENTROID_B(c1, 0);

#pragma parrot(input, "kmeans", [6]dataIn)

	r += (RGBPIXEL_R(p, 0) - CENTROID_R(c1, 0)) * (RGBPIXEL_R(p, 0) - CENTROID_R(c1, 0));
	r += (RGBPIXEL_G(p, 0) - CENTROID_G(c1, 0)) * (RGBPIXEL_G(p, 0) - CENTROID_G(c1, 0));
	r += (RGBPIXEL_B(p, 0) - CENTROID_B(c1, 0)) * (RGBPIXEL_B(p, 0) - CENTROID_B(c1, 0));

	r_tmp = sqrt(r);

#pragma parrot(output, "kmeans", <0.0; 1.0>r_tmp)

	return r_tmp;
}

int pickCluster(float* p, float* c1) {
	float d1;

	d1 = euclideanDistance(p, c1);

	if (RGBPIXEL_DISTANCE(p, 0) <= d1)
		return 0;

	return 1;
}

void assignCluster(float* p, Clusters* clusters) {
	int i = 0;
	int *p2 = (int *)p;
	float *centroids = (float *)clusters->centroids;

	float d;
	d = euclideanDistance(p, &CENTROID(centroids, i));
	RGBPIXEL_DISTANCE(p, 0) = d;

	for(i = 1; i < clusters->k; ++i) {
		d = euclideanDistance(p, &CENTROID(centroids, i));
		if (d < RGBPIXEL_DISTANCE(p, 0)) {
			RGBPIXEL2_CLUSTER(p2, 0) = i;
			RGBPIXEL_DISTANCE(p, 0) = d;
		}
	}
}

