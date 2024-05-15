#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "jpeg_loader.h"
#include "jpeg_saver.h"
#include <dirent.h>
#include <time.h>

# define images_count 5

int main() {
    // Make an array to store all the file names
    char* inputFilename[images_count];
    char* outputFilename[images_count];

    // Get all files in the current directory
    DIR *d;
    int fileIDX = 0;
    struct dirent *dir;
    // Open folder ISIC_2020_Test_Input
    char* folderName = "ISIC_2020_Test_Input";
    char* OutputFolderName = "ISIC_2020_Test_Output";
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

    // Load JPEG using the function
    for (int i = 0; i < fileIDX; i++) {
        if (!loadJPEG(inputFilename[i], &imageData[i], &width[i], &height[i], &numChannels[i])) {
            fprintf(stderr, "loadJPEG Failed to load the file!\n");
            return -1;
        }
    }

    // End Time
    end = clock();
    total_time = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("Time taken for image reading: %f\n", total_time);

    // Start Time
    start = clock();

    for (int i = 0; i < fileIDX; i++) {

    // Convert to grayscale without OpenCV functions

        gray_image[i] = (unsigned char*)malloc(width[i] * height[i]);
        for (int y = 0; y < height[i]; y++) {
            for (int x = 0; x < width[i]; x++) {
                // Get the pixel value
                unsigned char* pixel = imageData[i] + (y * width[i] + x) * numChannels[i];
                // Convert to grayscale using the formula
                gray_image[i][y * width[i] + x] = (unsigned char)(0.299 * pixel[0] + 0.587 * pixel[1] + 0.114 * pixel[2]);
            }
        }

    // Save the grayscale image using the JPEG saver function
        if (!saveJPEG(outputFilename[i], gray_image[i], width[i], height[i], 1)) {
            fprintf(stderr, "saveJPEG Failed to save the file!\n");
            return -1;
        }

    // Free allocated memory
        free(imageData[i]);
        free(gray_image[i]);
    }

    // End Time
    end = clock();
    total_time = ((double) (end - start)) / CLOCKS_PER_SEC;
    printf("Time taken: %f\n", total_time);

    return 0;
}
