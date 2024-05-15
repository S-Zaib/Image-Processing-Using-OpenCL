#ifndef JPEG_SAVER_H
#define JPEG_SAVER_H

#include <stdio.h>
#include <stdlib.h>
#include <jpeglib.h>

int saveJPEG(const char* filename, unsigned char* imageData, int width, int height, int channels);

#endif  // JPEG_SAVER_H
