__kernel void convert_to_grayscale(__global unsigned char* allImages, __global int* widths, __global int* heights, int max_width, int max_height, __global unsigned char* outputImages) {
    int idx = get_global_id(0);

    __global unsigned char* image = &allImages[idx * max_width * max_height * 3];
    __global unsigned char* outputImage = &outputImages[idx * max_width * max_height];

    int width = widths[idx];
    int height = heights[idx];
    int channels = 3;

    for (int i = 0; i < width * height; i++) {
        unsigned char r = image[i * channels];
        unsigned char g = image[i * channels + 1];
        unsigned char b = image[i * channels + 2];

        unsigned char gray = 0.299f * r + 0.587f * g + 0.114f * b;

        outputImage[i] = gray;
    }
}

__kernel void old_grayscale(__global const uchar* inputImage, __global uchar* outputImage, const int width, const int height) {
    int x = get_global_id(0);
    int y = get_global_id(1);


    if (x < width && y < height) {
        int index = y * width + x;
        uchar3 pixel = *((__global uchar3*)&inputImage[index * 3]);
        outputImage[index] = 0.299f * pixel.x + 0.587f * pixel.y + 0.114f * pixel.z;
    }
}
