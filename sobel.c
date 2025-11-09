#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include "pgmio.h"

// Apply Sobel operator to compute edge detection
void sobel_filter(const float *input, float *output, int rows, int cols) {
    // Sobel kernels for gradient computation
    int Gx[3][3] = {
        {-1, 0, 1},
        {-2, 0, 2},
        {-1, 0, 1}
    };
    
    int Gy[3][3] = {
        {-1, -2, -1},
        { 0,  0,  0},
        { 1,  2,  1}
    };
    
    // Process each pixel (excluding 1-pixel border)
    for (int i = 1; i < rows - 1; i++) {
        for (int j = 1; j < cols - 1; j++) {
            float sum_x = 0.0f;
            float sum_y = 0.0f;
            
            // Apply 3x3 convolution
            for (int ki = -1; ki <= 1; ki++) {
                for (int kj = -1; kj <= 1; kj++) {
                    float pixel = input[(i + ki) * cols + (j + kj)];
                    sum_x += pixel * Gx[ki + 1][kj + 1];
                    sum_y += pixel * Gy[ki + 1][kj + 1];
                }
            }
            
            // Compute gradient magnitude
            float magnitude = sqrtf(sum_x * sum_x + sum_y * sum_y);
            output[i * cols + j] = magnitude;
        }
    }
    
    // Handle borders - set to 0
    for (int j = 0; j < cols; j++) {
        output[j] = 0.0f;                          // top row
        output[(rows - 1) * cols + j] = 0.0f;      // bottom row
    }
    for (int i = 0; i < rows; i++) {
        output[i * cols] = 0.0f;                   // left column
        output[i * cols + (cols - 1)] = 0.0f;      // right column
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <image_size>\n", argv[0]);
        fprintf(stderr, "Example: %s 256\n", argv[0]);
        return 1;
    }
    
    int size = atoi(argv[1]);
    if (size <= 0) {
        fprintf(stderr, "Error: Invalid image size\n");
        return 1;
    }
    
    // Build input filename
    char input_filename[256];
    snprintf(input_filename, sizeof(input_filename), "sample_%d.pgm", size);
    
    // Build output filename
    char output_filename[256];
    snprintf(output_filename, sizeof(output_filename), "sobel_%d.pgm", size);
    
    // Read input image
    float *input_image = NULL;
    int rows, cols;
    
    printf("Reading image: %s\n", input_filename);
    if (pgmread(input_filename, &input_image, &rows, &cols) != 0) {
        fprintf(stderr, "Error: Failed to read %s\n", input_filename);
        return 1;
    }
    
    if (rows != size || cols != size) {
        fprintf(stderr, "Warning: Image size is %dx%d, expected %dx%d\n", 
                cols, rows, size, size);
    }
    
    printf("Image loaded: %dx%d\n", cols, rows);
    
    // Allocate output image
    float *output_image = (float *)calloc(rows * cols, sizeof(float));
    if (!output_image) {
        fprintf(stderr, "Error: Failed to allocate output image\n");
        free(input_image);
        return 1;
    }
    
    // Start timing (exclude I/O)
    clock_t start = clock();
    
    // Apply Sobel filter
    sobel_filter(input_image, output_image, rows, cols);
    
    // End timing
    clock_t end = clock();
    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
    
    printf("Sobel filter completed in %.6f seconds\n", elapsed);
    
    // Write output image
    printf("Writing output: %s\n", output_filename);
    if (pgmwrite(output_filename, output_image, rows, cols, 1) != 0) {
        fprintf(stderr, "Error: Failed to write %s\n", output_filename);
        free(input_image);
        free(output_image);
        return 1;
    }
    
    printf("Output saved successfully\n");
    
    // Cleanup
    free(input_image);
    free(output_image);
    
    return 0;
}
