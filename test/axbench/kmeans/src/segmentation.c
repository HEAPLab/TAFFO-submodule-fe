/*
 * segmentation.c
 * 
 * Created on: Sep 9, 2013
 * 			Author: Amir Yazdanbakhsh <a.yazdanbakhsh@gatech.edu>
 */


#include "segmentation.h"
#include <stdio.h>
#include <stdlib.h>

#include "distance.h"

int initClusters(Clusters* __attribute__((annotate("range " RANGE_CENTROID))) clusters, int k, float __attribute__((annotate("range 1.0 0.0"))) scale /* = 1 */) {
	int i;
	float __attribute__((annotate(ANNOTATION_CENTROID))) x;
	
	float __attribute__((annotate(ANNOTATION_CENTROID))) *centroids = (float *)malloc(k * SIZEOF_CENTROID);
	int *centroids2 = (int *)centroids;

	clusters->centroids = (void *)centroids;

	if (clusters == NULL) {
		printf("Warning: Oops! Cannot allocate memory for the clusters!\n");

		return 0;
	}

	clusters->k = k;
	for (i = 0; i < clusters->k; i ++) {
		x = ((float)rand())/RAND_MAX;
		x *= scale;
		CENTROID_R(centroids, i) = x;

		x = ((float)rand())/RAND_MAX;
		x *= scale;
		CENTROID_G(centroids, i) = x;

		x = ((float)rand())/RAND_MAX;
		x *= scale;
		CENTROID_B(centroids, i) = x;

		CENTROID2_N(centroids2, i) = 0;
	}


	return 1;
}

void freeClusters(Clusters* clusters) {
	if (clusters != NULL)
		free(clusters->centroids);
}

void segmentImage(RgbImage* __attribute__((annotate("range " RANGE_RGBPIXEL))) image, Clusters* __attribute__((annotate("range " RANGE_CENTROID))) clusters, int n) {
	int i;
	int x, y;
	int c;
	float __attribute__((annotate(ANNOTATION_RGBPIXEL))) **pixels = (float **)image->pixels;
	int **pixels2 = (int **)image->pixels;
	float __attribute__((annotate(ANNOTATION_CENTROID))) *centroids = (float *)clusters->centroids;
	int *centroids2 = (int *)clusters->centroids;

	for (i = 0; i < n; ++i) {
		for (y = 0; y < image->h; y++) {
			for (x = 0; x < image->w; x++) {
				assignCluster(&RGBPIXEL(pixels[y], x), clusters);
			}
		}

		/** Recenter */
		for (c  = 0; c < clusters->k; ++c) {
			CENTROID_R(centroids, c) = 0.;
			CENTROID_G(centroids, c) = 0.;
			CENTROID_B(centroids, c) = 0.;
			CENTROID2_N(centroids2, c) = 0;
		}
		for (y = 0; y < image->h; y++) {
			for (x = 0; x < image->w; x++) {
				CENTROID_R(centroids, RGBPIXEL2_CLUSTER(pixels2[y], x)) += RGBPIXEL_R(pixels[y], x);
				CENTROID_G(centroids, RGBPIXEL2_CLUSTER(pixels2[y], x)) += RGBPIXEL_G(pixels[y], x);
				CENTROID_B(centroids, RGBPIXEL2_CLUSTER(pixels2[y], x)) += RGBPIXEL_B(pixels[y], x);
				CENTROID2_N(centroids2, RGBPIXEL2_CLUSTER(pixels2[y], x)) += 1;
			}
		}
		for (c  = 0; c < clusters->k; ++c) {
			if (CENTROID2_N(centroids2, c) != 0) {
				CENTROID_R(centroids, c) = CENTROID_R(centroids, c) / CENTROID2_N(centroids2, c);
				CENTROID_G(centroids, c) = CENTROID_G(centroids, c) / CENTROID2_N(centroids2, c);
				CENTROID_B(centroids, c) = CENTROID_B(centroids, c) / CENTROID2_N(centroids2, c);
			}
		}
	}

	for (y = 0; y < image->h; y++) {
		for (x = 0; x < image->w; x++) {
		  float __attribute__((annotate("target:component " ANNOTATION_RGBPIXEL))) r, g, b;
			RGBPIXEL_R(pixels[y], x) = r = CENTROID_R(centroids, RGBPIXEL2_CLUSTER(pixels2[y], x));
			RGBPIXEL_G(pixels[y], x) = g = CENTROID_G(centroids, RGBPIXEL2_CLUSTER(pixels2[y], x));
			RGBPIXEL_B(pixels[y], x) = b = CENTROID_B(centroids, RGBPIXEL2_CLUSTER(pixels2[y], x));
		}
	}
}

