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

    char* inputFilename[images_count];
    char* outputFilename[images_count];
    DIR *d;
    int fileIDX = 0;
    struct dirent *dir;
    char* folderName = "ISIC_2020_Test_Input";
    char* OutputFolderName = "ISIC_2020_Test_Output";
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
                fileIDX++;


                // Check if the file count is more than the maximum
                if (fileIDX >= images_count) {
                    break;
                }
            }
        }
        closedir(d);
    }
    unsigned char* imageData[images_count];
    int width[images_count];
    int height[images_count];
    int numChannels[images_count];
    unsigned char* gray_image[images_count];

    // Start Time
    clock_t start, end;
    double total_time;
    start = clock();

    for (int i = 0; i < images_count; i++) {
        // Load JPEG using the function
        if (!loadJPEG(inputFilename[i], &imageData[i], &width[i], &height[i], &numChannels[i])) {
            fprintf(stderr, "loadJPEG Failed to load the file!\n");
            return -1;
        }
    }

    // End Time
    end = clock();
    total_time = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("Time taken to load all images: %f\n", total_time);

    // Start Time
    start = clock();

    // Get the first platform and device
    err = clGetPlatformIDs(1, &platform_id, NULL);
    err |= clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_DEFAULT, 1, &device_id, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Error getting platform or device.\n");
        return -1;
    }

    // Create a context
    context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &err);
    if (!context || err != CL_SUCCESS) {
        fprintf(stderr, "Error creating context.\n");
        return -1;
    }

    // Create a command queue
    queue = clCreateCommandQueue(context, device_id, 0, &err);
    if (!queue || err != CL_SUCCESS) {
        fprintf(stderr, "Error creating command queue.\n");
        clReleaseContext(context);
        return -1;
    }

    // Create the program from the kernel source file
    FILE* file = fopen("kernel.cl", "r");
    if (!file) {
        fprintf(stderr, "Error opening kernel file.\n");
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return -1;
    }

    // Get the file size
    fseek(file, 0, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);
    
    char* source = (char*)malloc(fileSize + 1);
    if (!source) {
        fprintf(stderr, "Error allocating memory for kernel source.\n");
        fclose(file);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return -1;
    }

    // Read the file
    fread(source, fileSize, 1, file);
    fclose(file);
    source[fileSize] = '\0';

    // Create the program
    program = clCreateProgramWithSource(context, 1, (const char **)&source, &fileSize, &err);
    if (!program || err != CL_SUCCESS) {
        fprintf(stderr, "Error creating program.\n");
        free(source);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return -1;
    }

    free(source);

    // Build the program
    err = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);
    if (err != CL_SUCCESS) {
        fprintf(stderr, "Error building program.\n");
        clReleaseProgram(program);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return -1;
    }

    // Create the kernel
    kernel = clCreateKernel(program, "old_grayscale", &err);
    if (!kernel || err != CL_SUCCESS) {
        fprintf(stderr, "Error creating kernel.\n");
        clReleaseProgram(program);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return -1;
    }

    // Create memory buffers on the device for each vector
    for (int i = 0; i < images_count; i++) {
        inp_mem = clCreateBuffer(context, CL_MEM_READ_ONLY, width[i] * height[i] * numChannels[i] * sizeof(unsigned char), NULL, &err);
        out_mem = clCreateBuffer(context, CL_MEM_WRITE_ONLY, width[i] * height[i] * sizeof(unsigned char), NULL, &err);

        if (!inp_mem || !out_mem) {
            fprintf(stderr, "Failed to create buffer.\n");
            clReleaseMemObject(inp_mem);
            clReleaseMemObject(out_mem);
            clReleaseKernel(kernel);
            clReleaseProgram(program);
            clReleaseCommandQueue(queue);
            clReleaseContext(context);
            return -1;
        }

        // Copy the image data to the input buffer
        err = clEnqueueWriteBuffer(queue, inp_mem, CL_TRUE, 0, width[i] * height[i] * numChannels[i] * sizeof(unsigned char), imageData[i], 0, NULL, NULL);
        if (err != CL_SUCCESS) {
            fprintf(stderr, "Failed to write buffer.\n");
            return -1;
        }

        // Set the arguments of the kernel
        err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &inp_mem);
        err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &out_mem);
        err |= clSetKernelArg(kernel, 2, sizeof(int), &width[i]);
        err |= clSetKernelArg(kernel, 3, sizeof(int), &height[i]);
        if (err != CL_SUCCESS) {
            fprintf(stderr, "Failed to set kernel arguments.\n");
            return -1;
        }

        // Execute the OpenCL kernel on the list
        size_t global_work_size[2] = { width[i], height[i] };
        err = clEnqueueNDRangeKernel(queue, kernel, 2, NULL, global_work_size, NULL, 0, NULL, NULL);
        if (err != CL_SUCCESS) {
            fprintf(stderr, "Error executing kernel.\n");
            clReleaseMemObject(inp_mem);
            clReleaseMemObject(out_mem);
            clReleaseKernel(kernel);
            clReleaseProgram(program);
            clReleaseCommandQueue(queue);
            clReleaseContext(context);
            return -1;
        }

        // Read the memory buffer on the device to the local variable
        gray_image[i] = (unsigned char*)malloc(width[i] * height[i] * sizeof(unsigned char));
        err = clEnqueueReadBuffer(queue, out_mem, CL_TRUE, 0, width[i] * height[i] * sizeof(unsigned char), gray_image[i], 0, NULL, NULL);
        if (err != CL_SUCCESS) {
            fprintf(stderr, "Failed to read buffer.\n");
            return -1;
        }

        // Save the grayscale image
        if (!saveJPEG(outputFilename[i], gray_image[i], width[i], height[i], 1)) {
            fprintf(stderr, "saveJPEG Failed to save the file!\n");
            return -1;
        }

        // Release memory
        free(imageData[i]);
        free(gray_image[i]);
        free(inputFilename[i]);
        free(outputFilename[i]);
    }

    // End Time
    end = clock();
    total_time = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("Time taken to process all images: %f\n", total_time);

    // Clean up
    clFlush(queue);
    clFinish(queue);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseMemObject(inp_mem);
    clReleaseMemObject(out_mem);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);

    return 0;
}
