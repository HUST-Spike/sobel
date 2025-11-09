#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <omp.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "pgmio.h"

#define OUTPUT_DIR "output"

// Apply 3x3 mean blur filter using OpenMP parallelization
void mean_blur(const float *input, float *output, int rows, int cols) {
    const float kernel_weight = 1.0f / 9.0f;
    
    // Parallelize the main computation loop
    // Each thread processes different rows independently
    #pragma omp parallel for schedule(static)
    for (int i = 1; i < rows - 1; i++) {
        for (int j = 1; j < cols - 1; j++) {
            float sum = 0.0f;
            
            // Apply 3x3 convolution
            for (int ki = -1; ki <= 1; ki++) {
                for (int kj = -1; kj <= 1; kj++) {
                    sum += input[(i + ki) * cols + (j + kj)];
                }
            }
            
            output[i * cols + j] = sum * kernel_weight;
        }
    }
    
    // Handle borders in parallel
    #pragma omp parallel sections
    {
        #pragma omp section
        {
            // Top and bottom rows
            for (int j = 0; j < cols; j++) {
                output[j] = input[j];
                output[(rows - 1) * cols + j] = input[(rows - 1) * cols + j];
            }
        }
        
        #pragma omp section
        {
            // Left and right columns
            for (int i = 0; i < rows; i++) {
                output[i * cols] = input[i * cols];
                output[i * cols + (cols - 1)] = input[i * cols + (cols - 1)];
            }
        }
    }
}

// Apply Sobel operator using OpenMP parallelization
void sobel_filter(const float *input, float *output, int rows, int cols) {
    // Sobel kernels
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
    
    // Parallelize the main computation
    // static schedule: divide rows evenly among threads
    #pragma omp parallel for schedule(static)
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
    
    // Handle borders in parallel
    #pragma omp parallel sections
    {
        #pragma omp section
        {
            // Top and bottom rows
            for (int j = 0; j < cols; j++) {
                output[j] = 0.0f;
                output[(rows - 1) * cols + j] = 0.0f;
            }
        }
        
        #pragma omp section
        {
            // Left and right columns
            for (int i = 0; i < rows; i++) {
                output[i * cols] = 0.0f;
                output[i * cols + (cols - 1)] = 0.0f;
            }
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <image_size>\n", argv[0]);
        fprintf(stderr, "Example: %s 256 or %s 4k\n", argv[0], argv[0]);
        return 1;
    }
    
    // Parse input argument: support 256, 1024, 4k, 16k formats
    char *input_arg = argv[1];
    char size_str[32];
    
    int len = strlen(input_arg);
    if (len > 1 && (input_arg[len-1] == 'k' || input_arg[len-1] == 'K')) {
        char num_part[32];
        strncpy(num_part, input_arg, len-1);
        num_part[len-1] = '\0';
        int num = atoi(num_part);
        snprintf(size_str, sizeof(size_str), "%d", num * 1000);
    } else {
        strcpy(size_str, input_arg);
    }
    
    int size = atoi(size_str);
    if (size <= 0) {
        fprintf(stderr, "Error: Invalid image size\n");
        return 1;
    }
    
    // Create output directory
#ifdef _WIN32
    mkdir(OUTPUT_DIR);
#else
    mkdir(OUTPUT_DIR, 0755);
#endif
    
    // Build filenames
    char input_filename[256];
    char output_filename[256];
    
    if (size >= 1000 && size % 1000 == 0) {
        int k_value = size / 1000;
        snprintf(input_filename, sizeof(input_filename), "sample_%dk.pgm", k_value);
        snprintf(output_filename, sizeof(output_filename), "%s/sobel_omp_%dk.pgm", OUTPUT_DIR, k_value);
    } else {
        snprintf(input_filename, sizeof(input_filename), "sample_%d.pgm", size);
        snprintf(output_filename, sizeof(output_filename), "%s/sobel_omp_%d.pgm", OUTPUT_DIR, size);
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
    
    // Get number of threads
    int num_threads;
    #pragma omp parallel
    {
        #pragma omp master
        num_threads = omp_get_num_threads();
    }
    
    printf("Image loaded: %dx%d\n", cols, rows);
    printf("OpenMP threads: %d\n", num_threads);
    
    // Allocate buffers
    float *blurred_image = (float *)calloc(rows * cols, sizeof(float));
    float *output_image = (float *)calloc(rows * cols, sizeof(float));
    if (!blurred_image || !output_image) {
        fprintf(stderr, "Error: Failed to allocate buffers\n");
        free(input_image);
        if (blurred_image) free(blurred_image);
        if (output_image) free(output_image);
        return 1;
    }
    
    // Start timing (exclude I/O)
    double start = omp_get_wtime();
    
    // Step 1: Apply mean blur filter
    mean_blur(input_image, blurred_image, rows, cols);
    
    // Step 2: Apply Sobel filter
    sobel_filter(blurred_image, output_image, rows, cols);
    
    // End timing
    double end = omp_get_wtime();
    double elapsed = end - start;
    
    printf("Processing completed in %.6f seconds\n", elapsed);
    
    // Write output
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
