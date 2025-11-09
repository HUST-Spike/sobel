好的，这是一份为你（或你的编程助手）准备的详细 Markdown 指南，汇总了 `Assignment 1.pdf` 和 `HPC Usage Notes.pdf` 中所有关于实现、测试和提交的关键要求。

-----

# CMSC5702 Assignment 1: 并行 Sobel 滤波器 Copilot 指南

## 1\. 🎯 总体目标

[cite\_start]核心任务是实现 Sobel 边缘检测算子，并开发、测量和比较三种实现方式的性能：串行、共享内存 (OpenMP) 和分布式内存 (MPI) [cite: 661]。

## 2\. 📐 核心算法：Sobel 算子

1.  [cite\_start]**输入**: 灰度图像 $I$ [cite: 665]。
2.  **卷积核**:
      * [cite\_start]水平梯度 $G_x = \begin{bmatrix} -1 & 0 & +1 \\ -2 & 0 & +2 \\ -1 & 0 & +1 \end{bmatrix} * I$ [cite: 664] [cite\_start](注意: `Assignment 1.pdf` 和 `HPC Usage.pdf` 中 $G_x$ 的符号相反 [cite: 580, 664][cite\_start]，请以 `Assignment 1.pdf` [cite: 664] 为准，或确认 $G_x$ 和 $G_y$ 的一致性)。
      * [cite\_start]垂直梯度 $G_y = \begin{bmatrix} -1 & -2 & -1 \\ 0 & 0 & 0 \\ +1 & +2 & +1 \end{bmatrix} * I$ [cite: 664, 581]。
3.  [cite\_start]**输出**: 梯度幅值 $G = \sqrt{G_x^2 + G_y^2}$ [cite: 666, 582]。
4.  **可选模糊 (加分项)**:
      * [cite\_start]在 Sobel 之前应用一个步骤来降噪 [cite: 662, 676, 688]。
      * [cite\_start]推荐使用 $3 \times 3$ 均值滤波器 (Average Filter) [cite: 670, 677]。
      * [cite\_start]卷积核 $K = \frac{1}{9} \begin{bmatrix} 1 & 1 & 1 \\ 1 & 1 & 1 \\ 1 & 1 & 1 \end{bmatrix}$ [cite: 678]。

## 3\. 💻 实现要求

### Part 1: 串行实现 (`sobel.c` / `.cpp`)

  * [cite\_start]**占比**: 10% [cite: 668]。
  * **要求**:
      * [cite\_start]实现一个标准的、串行的 C 程序来应用 Sobel 算子 [cite: 669, 598]。
      * [cite\_start]代码必须能接收一个**样本大小 (sample size)** 作为参数 [cite: 671]。
  * **计时**:
      * [cite\_start]C: 使用 `time.h` 中的 `clock()` [cite: 640]。
      * [cite\_start]C++: 使用 `<chrono>` 中的 `std::chrono::steady_clock::now()` [cite: 641]。

### Part 2: OpenMP 实现 (`sobel_omp.c` / `.cpp`)

  * [cite\_start]**占比**: 40% [cite: 672]。
  * **要求**:
      * [cite\_start]使用 OpenMP 将 Sobel 算子并行化 [cite: 673, 599]。
      * [cite\_start]代码必须能接收一个**样本大小 (sample size)** 作为参数 [cite: 679]。
      * [cite\_start]*提示*: 处理大图像时（如 4K, 16K），可以考虑分块（tiles）处理以减少内存带宽问题 [cite: 675]。
  * [cite\_start]**计时**: 使用 `omp_get_wtime()` [cite: 644]。

### Part 3: MPI 实现 (`sobel_mpi.c` / `.cpp`)

  * [cite\_start]**占比**: 50% [cite: 683]。
  * **要求**:
      * [cite\_start]使用 MPI 实现 Sobel 算子 [cite: 684, 601]。
      * [cite\_start]代码必须能接收一个**样本大小 (sample size)** 作为参数 [cite: 691]。
      * **[关键] 域分解**:
          * [cite\_start]必须选择一种域分解策略（如行划分、列划分等） [cite: 685, 605]。
          * [cite\_start]推荐的例子是**按行分解 (row-wise decomposition)** [cite: 611]。
          * [cite\_start]设计和通信模式会影响分数 [cite: 686]。
      * **[关键] 边界处理**:
          * [cite\_start]由于 $3 \times 3$ 卷积，你必须处理子域的边界像素 [cite: 687]。
          * [cite\_start]这**必须**通过\*\*“幽灵行/列” (Ghost rows/columns)\*\* 来实现 [cite: 606]。
          * [cite\_start]“幽灵缓冲区”是本地数组中的额外空间，用于存储从邻居进程通过消息传递交换来的边界数据 [cite: 607, 608, 621, 622]。
  * [cite\_start]**计时**: 使用 `MPI_Wtime()` [cite: 645]。

## 4\. ⚙️ 开发、编译与测试工作流

这是一个**两阶段**过程。**不遵守这个流程会导致编译或运行失败。**

### 阶段 1: 开发与调试 (在 `hpc1-hpc8` 上)

1.  [cite\_start]**登录**: `ssh` 到 `hpc1` - `hpc8` 中的任意一台 [cite: 139, 156]。
2.  [cite\_start]**编码**: 编写和**充分调试**你的三个程序 [cite: 157, 159, 160, 161]。
3.  **OpenMP 编译 (Dev)**:
      * [cite\_start]`gcc -fopenmp sobel_omp.c -o sobel_omp` [cite: 186]。
      * **\!\! [cite\_start]关键 \!\!**: **不要**使用 `-O3` 等优化选项，以保证基准测试的一致性 [cite: 194, 195]。
4.  **OpenMP 运行 (Dev)**:
      * [cite\_start]通过环境变量设置线程数 [cite: 197]。
      * [cite\_start]`export OMP_NUM_THREADS=8` (bash) 或 `setenv OMP_NUM_THREADS 8` (tcsh) [cite: 190, 191]。
      * [cite\_start]`./sobel_omp` [cite: 192]。
5.  **MPI 库选择**:
      * **\!\! [cite\_start]关键 \!\!**: **必须使用 Open MPI**。MPICH 在基准测试节点上有问题 [cite: 338, 340, 342]。
6.  **MPI 编译 (Dev)**:
      * [cite\_start]**手动**设置 Open MPI 环境变量（每次登录或在 `.cshrc` / `.bashrc` 中） [cite: 301, 302]。
      * [cite\_start]`export OPENMPI=/usr/local/openmpi` [cite: 293]。
      * [cite\_start]`export PATH=$OPENMPI/bin:$PATH` [cite: 293]。
      * [cite\_start]`export LD_LIBRARY_PATH=$OPENMPI/lib:$LD_LIBRARY_PATH` [cite: 294]。
      * [cite\_start]`mpicc sobel_mpi.c -o sobel_mpi` [cite: 314]。
7.  **MPI 运行 (Dev)**:
      * **\!\! 关键 \!\!**: 在开发节点上跨节点运行 Open MPI 时，**必须使用绝对路径**。
      * [cite\_start]`/usr/local/openmpi/bin/mpiexec --host hpc1:4,hpc2:4 -n 8 ./sobel_mpi` [cite: 328, 329]。

-----

### 阶段 2: 基准测试 (在 `hpc11-hpc14` 上通过 Slurm)

这是你获取报告所需性能数据的**唯一**地方。

1.  [cite\_start]**登录**: `ssh` 到 `linux5` - `linux16` 登录节点 [cite: 422]。
2.  **设置 Slurm**:
      * [cite\_start]`export SLURM_CONF=/opt1/slurm/hpc-slurm.conf` (bash) [cite: 424]。
      * [cite\_start]`setenv SLURM_CONF /opt1/slurm/hpc-slurm.conf` (tcsh) [cite: 424]。
3.  **\!\! 关键: 重新编译 \!\!**:
      * [cite\_start]开发节点和测试节点的库路径不同 [cite: 377, 379, 381]。
      * [cite\_start]在 `hpc1` 上编译的程序**无法**在 `hpc11` 上运行 [cite: 382, 430]。
      * **必须**使用 `srun` 在测试节点上重新编译：
      * **Re-compile Seq**:
        [cite\_start]`srun -p cmsc5702_hpc -q cmsc5702 gcc sobel.c -o sobel` [cite: 437]。
      * **Re-compile OMP**:
        [cite\_start]`srun -p cmsc5702_hpc -q cmsc5702 gcc -fopenmp sobel_omp.c -o sobel_omp` [cite: 438]。
      * **Re-compile MPI**:
        [cite\_start]`srun -p cmsc5702_hpc -q cmsc5702 /usr/bin/mpicc.openmpi sobel_mpi.c -o sobel_mpi -lm` [cite: 432]。
        [cite\_start]*(注意命令是 `mpicc.openmpi` [cite: 380])*。

## 5\. 📊 性能测量 (Benchmarking)

  * [cite\_start]**计时规则**: 测量时间**必须排除**文件 I/O（如加载图像） [cite: 636][cite\_start]。在计时前后使用适当的屏障 (Barriers) [cite: 638]。
  * [cite\_start]**重复次数**: 每次运行**必须重复 3 次** [cite: 681, 692]。
  * [cite\_start]**图像尺寸**: 256x256, 1024x1024, 4000x4000, 16000x16000 [cite: 680, 708]。
  * [cite\_start]**工具**: `pgmio.h` 和 `test_pgmio.c` 提供了读写 PGM 图像的示例 [cite: 709][cite\_start]。`download_samples.py` 用于下载大样本 [cite: 708]。

### 运行串行 (Slurm)

  * [cite\_start]**命令**: `srun -p cmsc5702_hpc -q cmsc5702 -c 1 ./sobel` [cite: 448]。

### 运行 OpenMP (Slurm)

  * [cite\_start]**测试范围**: 1, 2, 4, 8, 16, 32 线程 [cite: 680, 711]。
  * **命令**: 使用 `srun` 的 `-c` 选项来控制核心/线程数。
  * [cite\_start]**1 线程**: `srun -p cmsc5702_hpc -q cmsc5702 -c 1 --export=OMP_NUM_THREADS=1 ./sobel_omp` [cite: 451]。
  * [cite\_start]**32 线程**: `srun -p cmsc5702_hpc -q cmsc5702 -c 32 ./sobel_omp` [cite: 453]。

### 运行 MPI (Slurm)

  * **测试范围**:
      * [cite\_start]1 节点: 1, 2, 4 进程 [cite: 690, 712]。
      * [cite\_start]4 节点: 8, 16, 32 进程 [cite: 690, 712]。
  * **\!\! 关键: 如何运行 \!\!**:
      * [cite\_start]**绝对不要**使用 `srun ... mpirun ...`。这会错误地启动 $N \times N$ 个进程 [cite: 458, 464, 465]。
      * [cite\_start]**必须**使用 `sbatch` 提交一个作业脚本 (job script) [cite: 468]。
  * **方法 1: 作业脚本 (推荐)**
      * [cite\_start]使用 PPT P31 提供的 `sobel_mpi.job` 模板 [cite: 476]。
      * 关键行:
        ```bash
        [cite_start]#SBATCH --nodes=4         [cite: 492]
        [cite_start]#SBATCH --ntasks=32       [cite: 493]
        [cite_start]#SBATCH --ntasks-per-node=8 [cite: 494]
        # [cite_start]... 动态创建 hostfile ... [cite: 500]
        [cite_start]mpiexec.openmpi --hostfile hostfile.txt -n 32 ./sobel_mpi 1024 [cite: 502]
        ```
      * [cite\_start]提交: `sbatch -p cmsc5702_hpc -q cmsc5702 sobel_mpi.job` [cite: 473]。
  * **方法 2: 辅助脚本**
      * [cite\_start]使用 PPT P32-34 提供的 `sbatch_sobel_mpi.sh` 脚本 [cite: 513]。
      * [cite\_start]用法: `./sbatch_sobel_mpi.sh <num_nodes> <tasks_per_node> <problem_size>` [cite: 519]。
      * [cite\_start]**示例 (4 节点, 共 32 进程, 1024 图像)**: `./sbatch_sobel_mpi.sh 4 8 1024` [cite: 563]。

## 6\. ✅ 验证与报告

### 验证

  * [cite\_start]**串行**: 在小的 $5 \times 5$ 或 $10 \times 10$ 模式上验证梯度结果 [cite: 702, 689]。
  * [cite\_start]**OpenMP**: 结果必须与串行程序输出一致 [cite: 703]。
  * [cite\_start]**MPI**: 结果必须与串行程序输出一致 [cite: 704]。

### 报告与提交

  * [cite\_start]**Deliverable 1: 源代码** [cite: 716]。
      * [cite\_start]必须有良好注释 [cite: 716]。
      * **\!\! 严格命名 \!\!**:
          * [cite\_start]`sobel.c` 或 `sobel.cpp` [cite: 717, 650, 653]。
          * [cite\_start]`sobel_omp.c` 或 `sobel_omp.cpp` [cite: 718, 650, 653]。
          * [cite\_start]`sobel_mpi.c` 或 `sobel_mpi.cpp` [cite: 718, 650, 653]。
  * [cite\_start]**Deliverable 2: Jupyter Notebook 报告** [cite: 719]。
      * [cite\_start]必须填写提供的模板 [cite: 694]。
      * [cite\_start]必须包含图表 [cite: 720]。
      * [cite\_start]必须包含对你的代码和优化方法的简短描述 [cite: 699]。
      * [cite\_start]（可选）包含原始和过滤后的图像以供验证 [cite: 700, 706, 721]。
  * **Deliverable 3: CSV 数据文件**
      * [cite\_start]报告模板会从这三个 CSV 文件读取数据 [cite: 695]。
      * [cite\_start]`openmp_times.csv`: 记录 OpenMP 的执行时间 [cite: 681, 696]。
      * [cite\_start]`mpi_times.csv`: 记录 MPI 的执行时间 [cite: 692, 697]。
      * [cite\_start]`verification.csv`: 记录验证结果 [cite: 698]。