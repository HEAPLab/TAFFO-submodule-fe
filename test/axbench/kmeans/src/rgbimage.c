/*
 * rgbimage.c
 * 
 * Created on: Sep 9, 2013
 * 			Author: Amir Yazdanbakhsh <a.yazdanbakhsh@gatech.edu>
 */


#include "rgbimage.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void initRgbImage(RgbImage* __attribute__((annotate("range " RANGE_RGBPIXEL))) image) {
	image->w = 0;
	image->h = 0;
	image->pixels = NULL;
	image->meta = NULL;
}

int readCell(FILE *fp, char* w) {
	int c;
	int i;

	w[0] = 0;
	for (c = fgetc(fp), i = 0; c != EOF; c = fgetc(fp)) {
		if (c == ' ' || c == '\t') {
			if (w[0] != '\"')
				continue;
		}

		if (c == ',' || c == '\n') {
			if (w[0] != '\"')
				break;
			else if (c == '\"') {
				w[i] = c;
				i++;
				break;
			}
		}

		w[i] = c;
		i++;
	}
	w[i] = 0;

	return c;
}

int loadRgbImage(const char* fileName, RgbImage* __attribute__((annotate("range " RANGE_RGBPIXEL))) image, float __attribute__((annotate("range 1.0 0.0")))scale) {
	int c;
	int i;
	int j;
	char w[256];
	//RgbPixel** pixels;
	float** __attribute__((annotate(ANNOTATION_RGBPIXEL))) pixels;
	int** pixels2;
	FILE *fp;

	//printf("Loading %s ...\n", fileName);

	fp = fopen(fileName, "r");
	if (!fp) {
		printf("Warning: Oops! Cannot open %s!\n", fileName);
		return 0;
	}

	c = readCell(fp, w);
	image->w = atoi(w);
	c = readCell(fp, w);
	image->h = atoi(w);

	//printf("%d x %d\n", image->w, image->h);

	pixels = (float**)malloc(image->h * SIZEOF_RGBPIXEL);
	pixels2 = (int **)pixels;

	if (pixels == NULL) {
		printf("Warning: Oops! Cannot allocate memory for the pixels!\n");

		fclose(fp);

		return 0;
	}

	c = 0;
	for(i = 0; i < image->h; i++) {
		pixels[i] = (float*)malloc(image->w * SIZEOF_RGBPIXEL);
		if (pixels[i] == NULL) {
			c = 1;
			break;
		}
	}

	if (c == 1) {
		printf("Warning: Oops! Cannot allocate memory for the pixels!\n");

		for (i--; i >= 0; i--)
			free(pixels[i]);
		free(pixels);

		fclose(fp);

		return 0;
	}

	for(i = 0; i < image->h; i++) {
		for(j = 0; j < image->w; j++) {
			c = readCell(fp, w);
			RGBPIXEL_R(pixels[i], j) = atoi(w) / scale;

			c = readCell(fp, w);
			RGBPIXEL_G(pixels[i], j) = atoi(w) / scale;

			c = readCell(fp, w);
			RGBPIXEL_B(pixels[i], j) = atoi(w) / scale;

			RGBPIXEL2_CLUSTER(pixels2[i], j) = 0;
			RGBPIXEL_DISTANCE(pixels[i], j) = 0.;
		}
	}
	image->pixels = pixels;

	c = readCell(fp, w);
	image->meta = (char*)malloc(strlen(w) * sizeof(char));
	if(image->meta == NULL) {
		printf("Warning: Oops! Cannot allocate memory for the pixels!\n");

		for (i = 0; i < image->h; i++)
			free(pixels[i]);
		free(pixels);

		fclose(fp);

		return 0;

	}
	strcpy(image->meta, w);

	//printf("%s\n", image->meta);

	//printf("w=%d x h=%d\n", image->w, image->h);

	return 1;
}

int saveRgbImage(RgbImage* __attribute__((annotate("range " RANGE_RGBPIXEL))) image, const char* fileName, float __attribute__((annotate("no_float 24 8"))) scale) {
	int i;
	int j;
	FILE *fp;
	float __attribute__((annotate(ANNOTATION_RGBPIXEL))) **pixels = (float **)image->pixels;
	//int **pixels2 = (int **)image->pixels;

	//printf("Saving %s ...\n", fileName);

	fp = fopen(fileName, "w");
	if (!fp) {
		printf("Warning: Oops! Cannot open %s!\n", fileName);
		return 0;
	}

	fprintf(fp, "%d,%d\n", image->w, image->h);
	//printf("%d,%d\n", image->w, image->h);

	for(i = 0; i < image->h; i++) {
		for(j = 0; j < (image->w - 1); j++) {
			fprintf(fp, "%d,%d,%d,", int(RGBPIXEL_R(pixels[i], j) * scale), int(RGBPIXEL_G(pixels[i], j) * scale), int(RGBPIXEL_B(pixels[i], j) * scale));
		}
		fprintf(fp, "%d,%d,%d\n", int(RGBPIXEL_R(pixels[i], j) * scale), int(RGBPIXEL_G(pixels[i], j) * scale), int(RGBPIXEL_B(pixels[i], j) * scale));
	}

	fprintf(fp, "%s", image->meta);
	//printf("%s\n", image->meta);

	fclose(fp);

	return 1;
}

void freeRgbImage(RgbImage* __attribute__((annotate("range " RANGE_RGBPIXEL))) image) {
	int i;

	if (image->meta != NULL)
		free(image->meta);

	if (image->pixels == NULL)
		return;

	for (i = 0; i < image->h; i++)
		if (image->pixels != NULL)
			free(((void**)image->pixels)[i]);

	free(image->pixels);
}

void makeGrayscale(RgbImage* __attribute__((annotate("range " RANGE_RGBPIXEL))) image) {
	int i;
	int j;
	float __attribute__((annotate(ANNOTATION_RGBPIXEL))) luminance;
	float __attribute__((annotate(ANNOTATION_RGBPIXEL))) **pixels = (float **)image->pixels;

	float __attribute__((annotate(ANNOTATION_RGBPIXEL))) rC = 0.30;
	float __attribute__((annotate(ANNOTATION_RGBPIXEL))) gC = 0.59;
	float __attribute__((annotate(ANNOTATION_RGBPIXEL))) bC = 0.11;

	for(i = 0; i < image->h; i++) {
		for(j = 0; j < image->w; j++) {
			luminance =
				rC * RGBPIXEL_R(pixels[i], j) +
				gC * RGBPIXEL_G(pixels[i], j) +
				bC * RGBPIXEL_B(pixels[i], j);

			RGBPIXEL_R(pixels[i], j) = (unsigned char)luminance;
			RGBPIXEL_G(pixels[i], j) = (unsigned char)luminance;
			RGBPIXEL_B(pixels[i], j) = (unsigned char)luminance;
		}
	}
}
