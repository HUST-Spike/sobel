#!/bin/bash
#SBATCH --job-name=sobel_omp
#SBATCH --partition=cmsc5702_hpc
#SBATCH --qos=cmsc5702
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=32
#SBATCH --time=00:25:00
#SBATCH --output=omp_test_%j.out

# Create output directory
mkdir -p output

echo "=========================================="
echo "OpenMP Sobel Filter Performance Test"
echo "=========================================="
echo ""

# Test thread counts: 1, 2, 4, 8, 16, 32
thread_counts=(1 2 4 8 16 32)

# Test image sizes: 256, 1024, 4k, 16k
image_sizes=(256 1024 4k 16k)

# Run tests
for size in "${image_sizes[@]}"; do
    echo "=========================================="
    echo "Testing ${size}x${size} image"
    echo "=========================================="
    echo ""
    
    for threads in "${thread_counts[@]}"; do
        echo "Thread count: ${threads}"
        echo "----------------------------"
        
        export OMP_NUM_THREADS=${threads}
        
        for run in 1 2 3; do
            echo "Run ${run}:"
            ./sobel_omp ${size}
            echo ""
        done
        
        echo ""
    done
    
    echo ""
done

echo "=========================================="
echo "All OpenMP tests completed!"
echo "=========================================="
