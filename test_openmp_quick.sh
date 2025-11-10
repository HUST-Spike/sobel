#!/bin/bash
#SBATCH --job-name=sobel_omp_quick
#SBATCH --partition=cmsc5702_hpc
#SBATCH --qos=cmsc5702
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=32
#SBATCH --time=00:20:00
#SBATCH --output=omp_quick_%j.out

# Quick test with smaller images only
mkdir -p output

echo "Quick OpenMP Test (256 and 1024 only)"
echo "======================================"
echo ""

thread_counts=(1 2 4 8 16 32)
image_sizes=(256 1024)

for size in "${image_sizes[@]}"; do
    echo "Testing ${size}x${size}"
    echo "----------------------"
    
    for threads in "${thread_counts[@]}"; do
        export OMP_NUM_THREADS=${threads}
        echo "Threads=${threads}: "
        for run in 1 2 3; do
            ./sobel_omp ${size} | grep "completed"
        done
        echo ""
    done
done

echo "Quick test completed!"
