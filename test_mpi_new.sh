#!/bin/bash

# --- Script to submit MPI tests ---
# This script generates a minimal number of job scripts to avoid submission limits
# Groups configurations to create only 4 job scripts (optimized for submission limits)

echo "=========================================="
echo "MPI Sobel Filter Test Suite"
echo "Optimized for job submission limits"
echo "=========================================="
echo ""

# Create a directory for job scripts if it doesn't exist
mkdir -p mpi_jobs

# Clean up old job scripts
rm -f mpi_jobs/*.job

# ===================================================================
# Job 1: Single Node Tests (1, 2, 4 processes)
# ===================================================================
echo "Generating job script: Single Node Tests (1, 2, 4 processes)"

job_script="mpi_jobs/sobel_mpi_single_node.job"

cat > "$job_script" <<'EOF'
#!/bin/bash
#SBATCH --job-name=mpi_single
#SBATCH --partition=cmsc5702_hpc
#SBATCH --qos=cmsc5702
#SBATCH --nodes=1
#SBATCH --ntasks=4
#SBATCH --ntasks-per-node=4
#SBATCH --time=00:15:00
#SBATCH --output=mpi_single_node_%j.out

mkdir -p output

echo "=========================================="
echo "MPI Single Node Tests"
echo "=========================================="
echo ""

# Test configurations: 1, 2, 4 processes
for nprocs in 1 2 4; do
    echo "=========================================="
    echo "Configuration: 1 node, ${nprocs} processes"
    echo "=========================================="
    echo ""
    
    # Create hostfile
    scontrol show hostnames "$SLURM_NODELIST" | awk -v slots="${nprocs}" '{print $0" slots="slots}' > hostfile_1n_${nprocs}p.txt
    
    echo "Hostfile content:"
    cat hostfile_1n_${nprocs}p.txt
    echo ""
    
    for size in 256 1024 4k 16k; do
        echo "Testing ${size}x${size} image"
        echo "----------------------------"
        
        for run in 1 2 3; do
            echo "Run ${run}:"
            mpiexec.openmpi --hostfile hostfile_1n_${nprocs}p.txt -n ${nprocs} ./sobel_mpi ${size}
            echo ""
        done
        
        echo ""
    done
    
    rm -f hostfile_1n_${nprocs}p.txt
    echo ""
done

echo "=========================================="
echo "Single node tests completed!"
echo "=========================================="
EOF

chmod +x "$job_script"
echo "  Created: $job_script"
echo ""

# ===================================================================
# Job 2: Multi-Node Test with 8 processes (4 nodes, 2 per node)
# ===================================================================
echo "Generating job script: Multi-Node 8 processes"

job_script="mpi_jobs/sobel_mpi_4nodes_8procs.job"

cat > "$job_script" <<'EOF'
#!/bin/bash
#SBATCH --job-name=mpi_4n_8p
#SBATCH --partition=cmsc5702_hpc
#SBATCH --qos=cmsc5702
#SBATCH --nodes=4
#SBATCH --ntasks=8
#SBATCH --ntasks-per-node=2
#SBATCH --time=00:15:00
#SBATCH --output=mpi_4nodes_8procs_%j.out

mkdir -p output

echo "=========================================="
echo "MPI Multi-Node Test: 8 processes"
echo "Configuration: 4 nodes, 2 processes per node"
echo "=========================================="
echo ""

# Create hostfile
scontrol show hostnames "$SLURM_NODELIST" | awk '{print $0" slots=2"}' > hostfile_4n_8p.txt

echo "Hostfile content:"
cat hostfile_4n_8p.txt
echo ""

for size in 256 1024 4k 16k; do
    echo "Testing ${size}x${size} image"
    echo "----------------------------"
    
    for run in 1 2 3; do
        echo "Run ${run}:"
        mpiexec.openmpi --hostfile hostfile_4n_8p.txt -n 8 ./sobel_mpi ${size}
        echo ""
    done
    
    echo ""
done

rm -f hostfile_4n_8p.txt

echo "=========================================="
echo "8-process test completed!"
echo "=========================================="
EOF

chmod +x "$job_script"
echo "  Created: $job_script"
echo ""

# ===================================================================
# Job 3: Multi-Node Test with 16 processes (4 nodes, 4 per node)
# ===================================================================
echo "Generating job script: Multi-Node 16 processes"

job_script="mpi_jobs/sobel_mpi_4nodes_16procs.job"

cat > "$job_script" <<'EOF'
#!/bin/bash
#SBATCH --job-name=mpi_4n_16p
#SBATCH --partition=cmsc5702_hpc
#SBATCH --qos=cmsc5702
#SBATCH --nodes=4
#SBATCH --ntasks=16
#SBATCH --ntasks-per-node=4
#SBATCH --time=00:15:00
#SBATCH --output=mpi_4nodes_16procs_%j.out

mkdir -p output

echo "=========================================="
echo "MPI Multi-Node Test: 16 processes"
echo "Configuration: 4 nodes, 4 processes per node"
echo "=========================================="
echo ""

# Create hostfile
scontrol show hostnames "$SLURM_NODELIST" | awk '{print $0" slots=4"}' > hostfile_4n_16p.txt

echo "Hostfile content:"
cat hostfile_4n_16p.txt
echo ""

for size in 256 1024 4k 16k; do
    echo "Testing ${size}x${size} image"
    echo "----------------------------"
    
    for run in 1 2 3; do
        echo "Run ${run}:"
        mpiexec.openmpi --hostfile hostfile_4n_16p.txt -n 16 ./sobel_mpi ${size}
        echo ""
    done
    
    echo ""
done

rm -f hostfile_4n_16p.txt

echo "=========================================="
echo "16-process test completed!"
echo "=========================================="
EOF

chmod +x "$job_script"
echo "  Created: $job_script"
echo ""

# ===================================================================
# Job 4: Multi-Node Test with 32 processes (4 nodes, 8 per node)
# ===================================================================
echo "Generating job script: Multi-Node 32 processes"

job_script="mpi_jobs/sobel_mpi_4nodes_32procs.job"

cat > "$job_script" <<'EOF'
#!/bin/bash
#SBATCH --job-name=mpi_4n_32p
#SBATCH --partition=cmsc5702_hpc
#SBATCH --qos=cmsc5702
#SBATCH --nodes=4
#SBATCH --ntasks=32
#SBATCH --ntasks-per-node=8
#SBATCH --time=00:15:00
#SBATCH --output=mpi_4nodes_32procs_%j.out

mkdir -p output

echo "=========================================="
echo "MPI Multi-Node Test: 32 processes"
echo "Configuration: 4 nodes, 8 processes per node"
echo "=========================================="
echo ""

# Create hostfile
scontrol show hostnames "$SLURM_NODELIST" | awk '{print $0" slots=8"}' > hostfile_4n_32p.txt

echo "Hostfile content:"
cat hostfile_4n_32p.txt
echo ""

for size in 256 1024 4k 16k; do
    echo "Testing ${size}x${size} image"
    echo "----------------------------"
    
    for run in 1 2 3; do
        echo "Run ${run}:"
        mpiexec.openmpi --hostfile hostfile_4n_32p.txt -n 32 ./sobel_mpi ${size}
        echo ""
    done
    
    echo ""
done

rm -f hostfile_4n_32p.txt

echo "=========================================="
echo "32-process test completed!"
echo "=========================================="
EOF

chmod +x "$job_script"
echo "  Created: $job_script"
echo ""

echo "=========================================="
echo "All job scripts generated in mpi_jobs/"
echo "Total: 4 job scripts (optimized for submission limits)"
echo "=========================================="
echo ""
echo "Job scripts created:"
echo "  1. sobel_mpi_single_node.job      (1, 2, 4 processes)"
echo "  2. sobel_mpi_4nodes_8procs.job    (8 processes)"
echo "  3. sobel_mpi_4nodes_16procs.job   (16 processes)"
echo "  4. sobel_mpi_4nodes_32procs.job   (32 processes)"
echo ""
echo "=========================================="
echo "To submit all jobs sequentially:"
echo "  for job in mpi_jobs/*.job; do sbatch \$job; sleep 2; done"
echo ""
echo "Or submit one at a time:"
echo "  sbatch mpi_jobs/sobel_mpi_single_node.job"
echo "  sbatch mpi_jobs/sobel_mpi_4nodes_8procs.job"
echo "  sbatch mpi_jobs/sobel_mpi_4nodes_16procs.job"
echo "  sbatch mpi_jobs/sobel_mpi_4nodes_32procs.job"
echo ""
echo "To check job status:"
echo "  squeue -u \$USER"
echo ""
echo "To cancel old jobs (75793-75796):"
echo "  scancel 75793 75794 75795 75796"
echo "========================================"
