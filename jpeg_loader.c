#include "jpeg_loader.h"
#include <string.h>

int loadJPEG(const char* filename, unsigned char** imageData, int* width, int* height, int* numChannels) {
    FILE* infile;
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    JSAMPARRAY buffer;
    int row_stride;

    // Open the JPEG file
    if ((infile = fopen(filename, "rb")) == NULL) {
        fprintf(stderr, "Whoops, Failed to Open ;/\n");
        return 0;
    }

    // Initialize the JPEG decompression object
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, infile);
    jpeg_read_header(&cinfo, TRUE);
    jpeg_start_decompress(&cinfo);

    *width = cinfo.output_width;
    *height = cinfo.output_height;
    *numChannels = cinfo.output_components;
    row_stride = *width * *numChannels;

    *imageData = (unsigned char*)malloc(row_stride * *height);

    buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr)&cinfo, JPOOL_IMAGE, row_stride, 1);

    // Read the JPEG image data line by line
    while (cinfo.output_scanline < cinfo.output_height) {
        jpeg_read_scanlines(&cinfo, buffer, 1);
        memcpy(*imageData + (cinfo.output_scanline - 1) * row_stride, *buffer, row_stride);
    }

    // Finish decompression and cleanup
    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    fclose(infile);

    return 1;
}

int getJPEGData(const char* filename, unsigned char* imageData, int* width, int* height)
{
    FILE* infile;
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    JSAMPARRAY buffer;
    int row_stride;

    // Open the JPEG file
    if ((infile = fopen(filename, "rb")) == NULL) {
        fprintf(stderr, "Whoops, Failed to Open ;/\n");
        return 0;
    }

    // Print the JPEG file
    // Initialize the JPEG decompression object
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, infile);
    jpeg_read_header(&cinfo, TRUE);
    jpeg_start_decompress(&cinfo);

    *width = cinfo.output_width;
    *height = cinfo.output_height;
    row_stride = *width * 3;

    buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr)&cinfo, JPOOL_IMAGE, row_stride, 1);

    // Read the JPEG image data line by line
    while (cinfo.output_scanline < cinfo.output_height) {
        jpeg_read_scanlines(&cinfo, buffer, 1);
        memcpy(imageData + (cinfo.output_scanline - 1) * row_stride, *buffer, row_stride);
    }



    // Finish decompression and cleanup
    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
    fclose(infile);

    return 1;
}

int saveJPEGData(const char* filename, unsigned char* imageData, int width, int height, int numChannels) {
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    JSAMPROW row_pointer[1];
    int row_stride;

    FILE* outfile = fopen(filename, "wb");
    if (!outfile) {
        fprintf(stderr, "Failed to open output file for writing.\n");
        return 0;
    }

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);
    jpeg_stdio_dest(&cinfo, outfile);

    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = numChannels;
    cinfo.in_color_space = (numChannels == 3) ? JCS_RGB : JCS_GRAYSCALE;

    jpeg_set_defaults(&cinfo);
    jpeg_start_compress(&cinfo, TRUE);

    row_stride = width * numChannels;
    while (cinfo.next_scanline < cinfo.image_height) {
        row_pointer[0] = &imageData[cinfo.next_scanline * row_stride];
        jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }

    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);
    fclose(outfile);

    return 1;
}

int getJPEGSize(const char* filename, int* width, int* height)
{
    FILE* infile;
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;
    JSAMPARRAY buffer;
    int row_stride;

    // Open the JPEG file
    if ((infile = fopen(filename, "rb")) == NULL) {
        fprintf(stderr, "Whoops, Failed to Open ;/\n");
        return 0;
    }

    // Initialize the JPEG decompression object
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, infile);
    jpeg_read_header(&cinfo, TRUE);
    jpeg_start_decompress(&cinfo);

    *width = cinfo.output_width;
    *height = cinfo.output_height;

    // Cleanup
    jpeg_destroy_decompress(&cinfo);
    fclose(infile);
    

    return 1;
}