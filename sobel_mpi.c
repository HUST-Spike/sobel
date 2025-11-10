#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <mpi.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "pgmio.h"

#define OUTPUT_DIR "output"

// Apply 3x3 mean blur filter on local image chunk with ghost rows
void mean_blur_local(const float *input, float *output, int local_rows, int cols, int has_top, int has_bottom) {
    const float kernel_weight = 1.0f / 9.0f;
    
    // Determine processing range (skip boundaries that need ghost rows)
    int start_row = has_top ? 1 : 1;  // Always skip first row if it's a boundary
    int end_row = has_bottom ? local_rows - 1 : local_rows - 1;  // Always skip last row
    
    // Process interior pixels
    for (int i = start_row; i < end_row; i++) {
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
    
    // Handle borders: copy from input
    // Left and right columns
    for (int i = 0; i < local_rows; i++) {
        output[i * cols] = input[i * cols];
        output[i * cols + (cols - 1)] = input[i * cols + (cols - 1)];
    }
    
    // Top row
    if (!has_top) {
        for (int j = 0; j < cols; j++) {
            output[j] = input[j];
        }
    } else {
        // Copy ghost row
        for (int j = 0; j < cols; j++) {
            output[j] = input[j];
        }
    }
    
    // Bottom row
    if (!has_bottom) {
        for (int j = 0; j < cols; j++) {
            output[(local_rows - 1) * cols + j] = input[(local_rows - 1) * cols + j];
        }
    } else {
        // Copy ghost row
        for (int j = 0; j < cols; j++) {
            output[(local_rows - 1) * cols + j] = input[(local_rows - 1) * cols + j];
        }
    }
}

// Apply Sobel operator on local image chunk with ghost rows
void sobel_filter_local(const float *input, float *output, int local_rows, int cols, int has_top, int has_bottom) {
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
    
    // Determine processing range
    int start_row = has_top ? 1 : 1;
    int end_row = has_bottom ? local_rows - 1 : local_rows - 1;
    
    // Process interior pixels
    for (int i = start_row; i < end_row; i++) {
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
    
    // Handle borders: set to 0
    // Left and right columns
    for (int i = 0; i < local_rows; i++) {
        output[i * cols] = 0.0f;
        output[i * cols + (cols - 1)] = 0.0f;
    }
    
    // Top row
    for (int j = 0; j < cols; j++) {
        output[j] = 0.0f;
    }
    
    // Bottom row
    for (int j = 0; j < cols; j++) {
        output[(local_rows - 1) * cols + j] = 0.0f;
    }
}

int main(int argc, char *argv[]) {
    int rank, num_procs;
    
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);
    
    if (argc != 2) {
        if (rank == 0) {
            fprintf(stderr, "Usage: %s <image_size>\n", argv[0]);
            fprintf(stderr, "Example: %s 256 or %s 4k\n", argv[0], argv[0]);
        }
        MPI_Finalize();
        return 1;
    }
    
    // Parse input argument
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
        if (rank == 0) {
            fprintf(stderr, "Error: Invalid image size\n");
        }
        MPI_Finalize();
        return 1;
    }
    
    // Variables for image data
    float *full_image = NULL;
    float *full_output = NULL;
    int rows = size, cols = size;
    
    // Root process reads the image
    if (rank == 0) {
        // Create output directory (ignore error if it already exists)
#ifdef _WIN32
        mkdir(OUTPUT_DIR);
#else
        mkdir(OUTPUT_DIR, 0755);  // Returns -1 if exists, which is OK
#endif
        
        // Build filename
        char input_filename[256];
        if (size >= 1000 && size % 1000 == 0) {
            int k_value = size / 1000;
            snprintf(input_filename, sizeof(input_filename), "sample_%dk.pgm", k_value);
        } else {
            snprintf(input_filename, sizeof(input_filename), "sample_%d.pgm", size);
        }
        
        if (pgmread(input_filename, &full_image, &rows, &cols) != 0) {
            fprintf(stderr, "Error: Failed to read %s\n", input_filename);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        
        full_output = (float *)calloc(rows * cols, sizeof(float));
    }
    
    // Broadcast image dimensions
    MPI_Bcast(&rows, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&cols, 1, MPI_INT, 0, MPI_COMM_WORLD);
    
    // Calculate rows per process
    int rows_per_proc = rows / num_procs;
    int remainder = rows % num_procs;
    
    // Calculate this process's row range
    int start_row, local_rows_actual;
    if (rank < remainder) {
        local_rows_actual = rows_per_proc + 1;
        start_row = rank * local_rows_actual;
    } else {
        local_rows_actual = rows_per_proc;
        start_row = rank * rows_per_proc + remainder;
    }
    
    // Add ghost rows (except for first and last process boundaries)
    int has_top_ghost = (rank > 0) ? 1 : 0;
    int has_bottom_ghost = (rank < num_procs - 1) ? 1 : 0;
    int local_rows_with_ghost = local_rows_actual + has_top_ghost + has_bottom_ghost;
    
    // Allocate local buffers
    size_t buffer_size = local_rows_with_ghost * cols * sizeof(float);
    float *local_image = (float *)malloc(buffer_size);
    float *local_blurred = (float *)malloc(buffer_size);
    float *local_output = (float *)malloc(buffer_size);
    
    if (!local_image || !local_blurred || !local_output) {
        fprintf(stderr, "Rank %d: Failed to allocate %zu bytes\n", rank, buffer_size);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
    
    // Initialize buffers to zero
    memset(local_image, 0, buffer_size);
    memset(local_blurred, 0, buffer_size);
    memset(local_output, 0, buffer_size);
    
    // Distribute image data
    if (rank == 0) {
        // Root process: copy its own portion (without ghost rows since it's the first)
        int dst_offset = has_top_ghost ? cols : 0;
        size_t copy_size = local_rows_actual * cols * sizeof(float);
        memcpy(local_image + dst_offset, full_image, copy_size);
        
        // Send to other processes
        int current_row = local_rows_actual;
        for (int p = 1; p < num_procs; p++) {
            int p_rows_actual = (p < remainder) ? rows_per_proc + 1 : rows_per_proc;
            int p_has_top = 1;
            int p_has_bottom = (p < num_procs - 1) ? 1 : 0;
            
            // Send starting from one row before (for ghost row)
            int send_start_row = current_row - p_has_top;
            int send_row_count = p_rows_actual + p_has_top + p_has_bottom;
            
            MPI_Send(full_image + send_start_row * cols, 
                    send_row_count * cols, 
                    MPI_FLOAT, p, 0, MPI_COMM_WORLD);
            
            current_row += p_rows_actual;
        }
    } else {
        // Receive data
        MPI_Recv(local_image, local_rows_with_ghost * cols, 
                MPI_FLOAT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
    
    // Synchronize before timing
    MPI_Barrier(MPI_COMM_WORLD);
    double start_time = MPI_Wtime();
    
    // Apply mean blur filter locally
    mean_blur_local(local_image, local_blurred, local_rows_with_ghost, cols, has_top_ghost, has_bottom_ghost);
    
    // Apply Sobel filter locally
    sobel_filter_local(local_blurred, local_output, local_rows_with_ghost, cols, has_top_ghost, has_bottom_ghost);
    
    // Synchronize after computation
    MPI_Barrier(MPI_COMM_WORLD);
    double end_time = MPI_Wtime();
    
    // Gather results back to root
    if (rank == 0) {
        // Copy root's portion (skip ghost rows if any)
        int src_offset = has_top_ghost ? cols : 0;
        memcpy(full_output, local_output + src_offset, local_rows_actual * cols * sizeof(float));
        
        // Receive from other processes
        int current_row = local_rows_actual;
        for (int p = 1; p < num_procs; p++) {
            int p_rows_actual = (p < remainder) ? rows_per_proc + 1 : rows_per_proc;
            
            MPI_Recv(full_output + current_row * cols, 
                    p_rows_actual * cols, 
                    MPI_FLOAT, p, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            
            current_row += p_rows_actual;
        }
    } else {
        // Send result (without ghost rows)
        int src_offset = has_top_ghost ? cols : 0;
        MPI_Send(local_output + src_offset, 
                local_rows_actual * cols, 
                MPI_FLOAT, 0, 1, MPI_COMM_WORLD);
    }
    
    // Root process writes output and prints timing
    if (rank == 0) {
        double elapsed = end_time - start_time;
        
        // Collect node information from all processes
        char *all_names = (char *)malloc(num_procs * MPI_MAX_PROCESSOR_NAME * sizeof(char));
        char processor_name[MPI_MAX_PROCESSOR_NAME];
        int name_len;
        MPI_Get_processor_name(processor_name, &name_len);
        
        MPI_Gather(processor_name, MPI_MAX_PROCESSOR_NAME, MPI_CHAR,
                   all_names, MPI_MAX_PROCESSOR_NAME, MPI_CHAR,
                   0, MPI_COMM_WORLD);
        
        // Count unique nodes
        int num_nodes = 1;
        for (int i = 1; i < num_procs; i++) {
            int is_unique = 1;
            for (int j = 0; j < i; j++) {
                if (strcmp(all_names + i * MPI_MAX_PROCESSOR_NAME, 
                          all_names + j * MPI_MAX_PROCESSOR_NAME) == 0) {
                    is_unique = 0;
                    break;
                }
            }
            if (is_unique) num_nodes++;
        }
        
        printf("Image: %dx%d | Nodes: %d | Processes: %d | Time: %.6f seconds\n", 
               cols, rows, num_nodes, num_procs, elapsed);
        
        free(all_names);
        
        // Build output filename
        char output_filename[256];
        if (size >= 1000 && size % 1000 == 0) {
            int k_value = size / 1000;
            snprintf(output_filename, sizeof(output_filename), "%s/sobel_mpi_%dk.pgm", OUTPUT_DIR, k_value);
        } else {
            snprintf(output_filename, sizeof(output_filename), "%s/sobel_mpi_%d.pgm", OUTPUT_DIR, size);
        }
        
        if (pgmwrite(output_filename, full_output, rows, cols, 1) != 0) {
            fprintf(stderr, "Error: Failed to write %s\n", output_filename);
        }
        
        free(full_image);
        free(full_output);
    } else {
        // Non-root processes send their processor names
        char processor_name[MPI_MAX_PROCESSOR_NAME];
        int name_len;
        MPI_Get_processor_name(processor_name, &name_len);
        
        MPI_Gather(processor_name, MPI_MAX_PROCESSOR_NAME, MPI_CHAR,
                   NULL, 0, MPI_CHAR, 0, MPI_COMM_WORLD);
    }
    
    // Cleanup
    free(local_image);
    free(local_blurred);
    free(local_output);
    
    MPI_Finalize();
    return 0;
}
