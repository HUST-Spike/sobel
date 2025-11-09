#!/bin/bash
#SBATCH --job-name=sobel_seq
#SBATCH --partition=cmsc5702_hpc
#SBATCH --qos=cmsc5702
#SBATCH --nodes=1
#SBATCH --ntasks=1
#SBATCH --cpus-per-task=1
#SBATCH --time=00:30:00
#SBATCH --output=seq_test_%j.out

# Create output directory
mkdir -p output

echo "=========================================="
echo "Sequential Sobel Filter Performance Test"
echo "=========================================="
echo ""

# Test each image size 3 times
for size in 256 1024 4k 16k; do
    echo "Testing ${size}x${size} image:"
    echo "----------------------------"
    
    for run in 1 2 3; do
        echo "Run ${run}:"
        ./sobel ${size}
        echo ""
    done
    
    echo ""
done

echo "All tests completed!"
echo "Results saved in output/"
