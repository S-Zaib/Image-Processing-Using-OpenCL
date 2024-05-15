#ifndef JPEG_LOADER_H
#define JPEG_LOADER_H

#include <stdio.h>
#include <stdlib.h>
#include <jpeglib.h>

int loadJPEG(const char* filename, unsigned char** imageData, int* width, int* height, int* numChannels);
int getJPEGData(const char* filename, unsigned char* imageData, int* width, int* height);
int saveJPEGData(const char* filename, unsigned char* imageData, int width, int height, int numChannels);

int getJPEGSize(const char* filename, int* width, int* height);

#endif  // JPEG_LOADER_H
