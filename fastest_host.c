#define CL_USE_DEPRECATED_OPENCL_1_2_APIS
#include <stdio.h>
#include <stdlib.h>
#include <CL/cl.h>
#include "jpeg_loader.h"
#include "jpeg_saver.h"
#include <dirent.h>
#include <string.h>
#include <time.h>

# define images_count 5

int main() {
    cl_platform_id platform_id;
    cl_device_id device_id;
    cl_context context;
    cl_command_queue queue;
    cl_program program;
    cl_kernel kernel;
    cl_mem inp_mem, out_mem;
    cl_int err;

    // Start time
    clock_t start, end;
    double cpu_time_used;
    start = clock();

    // Array to store the input and output filenames
    char* inputFilename[images_count];
    char* outputFilename[images_count];

    // Open the folder and read the file names
    DIR *d;
    int fileIDX = 0;
    struct dirent *dir;
    char* folderName = "ISIC_2020_Test_Input";
    char* OutputFolderName = "ISIC_2020_Test_Output";
    int max_width = 0;
    int max_height = 0;
    // Open folder ISIC_2020_Test_Input
    d = opendir(folderName);
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            // Check if the file is a JPEG file
            if (strstr(dir->d_name, ".jpg") != NULL) {
                // Add the folder name in start and then add file name to the array
                inputFilename[fileIDX] = (char*)malloc(strlen(folderName) + strlen(dir->d_name) + 2);
                strcpy(inputFilename[fileIDX], folderName);
                strcat(inputFilename[fileIDX], "/");
                strcat(inputFilename[fileIDX], dir->d_name);
                // Add the folder name in start
                outputFilename[fileIDX] = (char*)malloc(strlen(OutputFolderName) + strlen(dir->d_name) + 2);
                strcpy(outputFilename[fileIDX], OutputFolderName);
                strcat(outputFilename[fileIDX], "/");
                strcat(outputFilename[fileIDX], dir->d_name);
                // Print the file name
                // printf("%s\n", inputFilename[fileIDX]);
                // printf("%s\n", outputFilename[fileIDX]);

                // Read inputFilename and get width and height to get max width and height using libjpeg
                int width, height;
                if (getJPEGSize(inputFilename[fileIDX], &width, &height) != 1) {
                    fprintf(stderr, "Error reading JPEG file %s\n", inputFilename[fileIDX]);
                    return 1;
                }
                // Free the image

                // Check if the width is more than the maximum
                if (width > max_width) {
                    max_width = width;
                }

                // Check if the height is more than the maximum
                if (height > max_height) {
                    max_height = height;
                }

                fileIDX++;
                // Check if the file count is more than the maximum
                if (fileIDX >= images_count) {
                    break;
                }
            }
        }
        closedir(d);
    }


    // Load all the images data in 1 array and have offsef of max width and height
    unsigned char* imagesData = (unsigned char*)malloc(max_width * max_height * 3 * images_count);
    int* imagesWidth = (int*)malloc(images_count * sizeof(int));
    int* imagesHeight = (int*)malloc(images_count * sizeof(int));
    for (int i = 0; i < images_count; i++) {
        // Read inputFilename and get width and height using libjpeg
        if (getJPEGData(inputFilename[i], imagesData + i * max_width * max_height * 3, &imagesWidth[i], &imagesHeight[i]) != 1) {
            fprintf(stderr, "Error reading JPEG file %s\n", inputFilename[i]);
            return 1;
        }
    }

    // End time
    end = clock();
    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("Time taken to read images and Preprocessing: %f\n", cpu_time_used);

    // Start time
    start = clock();

    // Load the kernel source code into the array source_str
    FILE *fp;
    char *source_str;
    size_t source_size;

    fp = fopen("kernel.cl", "r");
    if (!fp) {
        fprintf(stderr, "Failed to load kernel.\n");
        exit(1);
    }
    source_str = (char*)malloc(10000);
    source_size = fread(source_str, 1, 10000, fp);
    fclose(fp);

    // Get platform and device information
    err = clGetPlatformIDs(1, &platform_id, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Failed to get platform ID.\n");
        exit(1);
    }

    err = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_DEFAULT, 1, &device_id, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Failed to get device ID.\n");
        exit(1);
    }

    // Create an OpenCL context
    context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &err);
    if (!context) {
        fprintf(stderr, "Failed to create context.\n");
        exit(1);
    }

    // Create a command queue
    queue = clCreateCommandQueue(context, device_id, 0, &err);
    if (!queue) {
        fprintf(stderr, "Failed to create command queue.\n");
        exit(1);
    }

    // Create a program from the kernel source code
    program = clCreateProgramWithSource(context, 1, (const char **)&source_str, (const size_t *)&source_size, &err);
    if (!program) {
        fprintf(stderr, "Failed to create program.\n");
        exit(1);
    }

    // Build the program
    err = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);
    if (err != CL_SUCCESS) {
        // Print build error code
        fprintf(stderr, "Error building program: %d\n", err);

        // Get build log
        char build_log[4096];
        clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, sizeof(build_log), build_log, NULL);
        fprintf(stderr, "Build log:\n%s\n", build_log);

        // Cleanup and exit
        clReleaseProgram(program);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        exit(1);
    }
    // Create the OpenCL kernel
    kernel = clCreateKernel(program, "convert_to_grayscale", &err);
    if (!kernel || err != CL_SUCCESS) {
        fprintf(stderr, "Failed to create kernel.\n");
        exit(1);
    }

    /*__kernel void convert_to_grayscale(__global unsigned char* allImages, __global int* widths, __global int* heights, int max_width, int max_height, __global unsigned char* outputImages) {
    // Get the index of the image this work-item should process
    int idx = get_global_id(0);

    // Get the start of the image in the big array
    unsigned char* image = &allImages[idx * max_width * max_height * 3];
    unsigned char* outputImage = &outputImages[idx * max_width * max_height];

    // Get the width and height of the image
    int width = widths[idx];
    int height = heights[idx];
    int channels = 3;

    // Convert the image to grayscale
    for (int i = 0; i < width * height; i++) {
        unsigned char r = image[i * channels];
        unsigned char g = image[i * channels + 1];
        unsigned char b = image[i * channels + 2];

        unsigned char gray = 0.299f * r + 0.587f * g + 0.114f * b;

        outputImage[i] = gray;
    }
}*/

    // Create memory buffers on the device for each vector
    inp_mem = clCreateBuffer(context, CL_MEM_READ_ONLY, max_width * max_height * 3 * images_count, NULL, &err);
    out_mem = clCreateBuffer(context, CL_MEM_WRITE_ONLY, max_width * max_height * images_count, NULL, &err);
    cl_mem imagesWidthBuffer, imagesHeightBuffer;
    imagesWidthBuffer = clCreateBuffer(context, CL_MEM_READ_ONLY, images_count * sizeof(int), NULL, &err);
    imagesHeightBuffer = clCreateBuffer(context, CL_MEM_READ_ONLY, images_count * sizeof(int), NULL, &err);

    // Copy the image data to the device
    err = clEnqueueWriteBuffer(queue, inp_mem, CL_TRUE, 0, max_width * max_height * 3 * images_count, imagesData, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Failed to copy image data to device.\n");
        exit(1);
    }

    // Copy the image width to the device
    err = clEnqueueWriteBuffer(queue, imagesWidthBuffer, CL_TRUE, 0, images_count * sizeof(int), imagesWidth, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Failed to copy image width to device.\n");
        exit(1);
    }

    // Copy the image height to the device
    err = clEnqueueWriteBuffer(queue, imagesHeightBuffer, CL_TRUE, 0, images_count * sizeof(int), imagesHeight, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Failed to copy image height to device.\n");
        exit(1);
    }

        // Set the arguments of the kernel
    err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &inp_mem);
    err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &imagesWidthBuffer);
    err |= clSetKernelArg(kernel, 2, sizeof(cl_mem), &imagesHeightBuffer);
    err |= clSetKernelArg(kernel, 3, sizeof(int), &max_width);
    err |= clSetKernelArg(kernel, 4, sizeof(int), &max_height);
    err |= clSetKernelArg(kernel, 5, sizeof(cl_mem), &out_mem);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Failed to set kernel arguments.\n");
        exit(1);
    }

    // Execute the OpenCL kernel on the list
    size_t global_item_size = images_count;
    err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &global_item_size, NULL, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Failed to execute kernel.\n");
        exit(1);
    }

    // Wait for the command queue to finish
    clFinish(queue);

    // Read the memory buffer on the device to the local variable
    unsigned char* result = (unsigned char*)malloc(max_width * max_height * images_count);
    err = clEnqueueReadBuffer(queue, out_mem, CL_TRUE, 0, max_width * max_height * images_count, result, 0, NULL, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Failed to read output image data from device.\n");
        exit(1);
    }

    // Save the output images
    for (int i = 0; i < images_count; i++) {
//int saveJPEGData(const char* filename, unsigned char* imageData, int width, int height, int numChannels);
        if (saveJPEGData(outputFilename[i], result + i * max_width * max_height, imagesWidth[i], imagesHeight[i], 1) != 1) {
            fprintf(stderr, "Error saving JPEG file %s\n", outputFilename[i]);
            return 1;
        }
    }
    
    // End time
    end = clock();
    cpu_time_used = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("Time taken to convert images to grayscale: %f\n", cpu_time_used);

    // Free dynamically allocated memory
    free(result);

    // Clean up
    clReleaseMemObject(inp_mem);
    clReleaseMemObject(out_mem);
    clReleaseMemObject(imagesWidthBuffer);
    clReleaseMemObject(imagesHeightBuffer);
    clReleaseProgram(program);
    clReleaseKernel(kernel);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);

    return 0;
}
