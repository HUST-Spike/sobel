#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "pgmio.h"

#define OUTPUT_DIR "output"

// Apply 3x3 mean blur filter to reduce noise
// This is a bonus feature that improves edge detection quality
void mean_blur(const float *input, float *output, int rows, int cols) {
    // 3x3 mean blur kernel: all weights are 1/9
    const float kernel_weight = 1.0f / 9.0f;
    
    // Process each pixel (excluding 1-pixel border)
    for (int i = 1; i < rows - 1; i++) {
        for (int j = 1; j < cols - 1; j++) {
            float sum = 0.0f;
            
            // Apply 3x3 average filter
            for (int ki = -1; ki <= 1; ki++) {
                for (int kj = -1; kj <= 1; kj++) {
                    sum += input[(i + ki) * cols + (j + kj)];
                }
            }
            
            // Store averaged pixel value
            output[i * cols + j] = sum * kernel_weight;
        }
    }
    
    // Handle borders - copy original values
    // Top and bottom rows
    for (int j = 0; j < cols; j++) {
        output[j] = input[j];                          // top row
        output[(rows - 1) * cols + j] = input[(rows - 1) * cols + j];  // bottom row
    }
    // Left and right columns
    for (int i = 0; i < rows; i++) {
        output[i * cols] = input[i * cols];                   // left column
        output[i * cols + (cols - 1)] = input[i * cols + (cols - 1)];  // right column
    }
}

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
        fprintf(stderr, "Example: %s 256 or %s 4k\n", argv[0], argv[0]);
        return 1;
    }
    
    // Parse input argument: support formats like 256, 1024, 4k, 16k
    char *input_arg = argv[1];
    char size_str[32];
    
    // Check if input ends with 'k' or 'K'
    int len = strlen(input_arg);
    if (len > 1 && (input_arg[len-1] == 'k' || input_arg[len-1] == 'K')) {
        // Extract numeric part and multiply by 1000
        char num_part[32];
        strncpy(num_part, input_arg, len-1);
        num_part[len-1] = '\0';
        int num = atoi(num_part);
        snprintf(size_str, sizeof(size_str), "%d", num * 1000);
    } else {
        // Use the number directly
        strcpy(size_str, input_arg);
    }
    
    int size = atoi(size_str);
    if (size <= 0) {
        fprintf(stderr, "Error: Invalid image size\n");
        return 1;
    }
    
    // Create output directory if not exists
#ifdef _WIN32
    mkdir(OUTPUT_DIR);
#else
    mkdir(OUTPUT_DIR, 0755);
#endif
    
    // Build filenames: prefer 'k' format for multiples of 1000
    char input_filename[256];
    char output_filename[256];
    
    if (size >= 1000 && size % 1000 == 0) {
        int k_value = size / 1000;
        snprintf(input_filename, sizeof(input_filename), "sample_%dk.pgm", k_value);
        snprintf(output_filename, sizeof(output_filename), "%s/sobel_%dk.pgm", OUTPUT_DIR, k_value);
    } else {
        snprintf(input_filename, sizeof(input_filename), "sample_%d.pgm", size);
        snprintf(output_filename, sizeof(output_filename), "%s/sobel_%d.pgm", OUTPUT_DIR, size);
    }
    
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
    
    // Allocate intermediate and output images
    float *blurred_image = (float *)calloc(rows * cols, sizeof(float));
    float *output_image = (float *)calloc(rows * cols, sizeof(float));
    if (!blurred_image || !output_image) {
        fprintf(stderr, "Error: Failed to allocate image buffers\n");
        free(input_image);
        if (blurred_image) free(blurred_image);
        if (output_image) free(output_image);
        return 1;
    }
    
    // Start timing (exclude I/O)
    clock_t start = clock();
    
    // Step 1: Apply mean blur filter to reduce noise (Bonus feature)
    printf("Applying 3x3 mean blur filter...\n");
    mean_blur(input_image, blurred_image, rows, cols);
    
    // Step 2: Apply Sobel filter on blurred image
    printf("Applying Sobel edge detection...\n");
    sobel_filter(blurred_image, output_image, rows, cols);
    
    // End timing
    clock_t end = clock();
    double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
    
    printf("Processing completed in %.6f seconds\n", elapsed);
    
    // Write output image
    printf("Writing output: %s\n", output_filename);
    if (pgmwrite(output_filename, output_image, rows, cols, 1) != 0) {
        fprintf(stderr, "Error: Failed to write %s\n", output_filename);
        free(input_image);
        free(blurred_image);
        free(output_image);
        return 1;
    }
    
    printf("Output saved successfully\n");
    
    // Cleanup
    free(input_image);
    free(blurred_image);
    free(output_image);
    
    return 0;
}
