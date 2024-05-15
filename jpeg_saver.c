#include "jpeg_saver.h"

int saveJPEG(const char* filename, unsigned char* imageData, int width, int height, int channels) {
    FILE* outfile = fopen(filename, "wb");
    if (!outfile) {
        fprintf(stderr, "Failed to open output file for writing.\n");
        return 0;
    }

    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    JSAMPROW row_pointer[1];
    int row_stride;

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);

    jpeg_stdio_dest(&cinfo, outfile);

    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = channels; // 3 for RGB, 1 for grayscale
    cinfo.in_color_space = JCS_GRAYSCALE;

    jpeg_set_defaults(&cinfo);
    jpeg_start_compress(&cinfo, TRUE);

    row_stride = width * cinfo.input_components;
    while (cinfo.next_scanline < cinfo.image_height) {
        row_pointer[0] = &imageData[cinfo.next_scanline * row_stride];
        jpeg_write_scanlines(&cinfo, row_pointer, 1);
    }

    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);

    fclose(outfile);

    return 1;
}
