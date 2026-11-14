# Project 3 作业：AlexNet 推理

[English](ASSIGNMENT.md) | [简体中文](ASSIGNMENT.zh-CN.md)

## 1. 概述

完整实现 AlexNet 的 CUDA 批量推理。先完成正确的朴素版本，再用 Nsight Systems
观察时间线、实现 stream 流水线，然后用 Nsight Compute 分析并优化卷积和全连接
内核，最后调整 batch size 与 stream 数量。

测量任务为 CPU 输入准备 → H2D → 完整 AlexNet 推理 → D2H → CPU 输出全部就绪。
目标是在保持与提供的 CPU 参考预测一致的前提下，缩短这一完整任务的耗时。
模型、依赖、构建与运行命令见 [README.zh-CN.md](README.zh-CN.md)。

## 2. 学习目标

本作业练习以下内容：

- 实现卷积、激活、池化和全连接层。
- 管理主机与显存资源，以及异步操作涉及的内存生命周期。
- 阅读 Nsight Systems 中的 CUDA API、GPU 活动和 NVTX 轨道。
- 使用 CUDA stream 让不同批次的传输和计算重叠。
- 根据 Nsight Compute 的证据优化卷积和全连接内核。
- 测量端到端性能，并选择合适的 batch/stream 参数。

## 3. 实现范围与已提供代码

完成 `include/alexnet_gpu.h` 和 `src/alexnet_gpu.cu` 中的整个 `AlexNetGpu` 类，
包括 GPU 内核和资源管理。完成 `src/main.cpp` 中驱动整轮推理的 `runGpuInference`。
stream 流水线也属于你需要实现的内容。

项目提供 CPU 模型、网络描述、参数加载、图像读取与预处理、分类指标、命令行处理，
以及 benchmark/validation 框架，直接复用这些部分。
按需使用共享的 `gpu::Buffer`、`gpu::PinnedBuffer`、`gpu::Stream`、`gpu::Event`
和 `Timer`。

保留 `project3` 可执行目标、命令行选项和 `submit_project3` 目标。
整个实验过程中保持 CPU 参考、模型、预处理和标签映射一致。
提供的预训练权重及图像、标签清单足够完成作业。

## 4. 必做内容

### 4.1 实现朴素 GPU 基线

完成 `AlexNetGpu` 类，包括：

- 输入、输出、模型参数及中间结果的分配和释放。
- 模型权重与 bias 的一次性上传。
- 卷积、ReLU、最大池化和全连接 CUDA 内核。
- 按 `alexnet_common.h` 中的顺序执行完整前向计算。
- 按每批实际图像数量上传输入和下载输出。

先使用直观的内核、同步拷贝和 `--streams 1`。
保留现有公共接口的可用性，初始实现可以处理完一批再开始下一批。
可复用资源分配与参数上传应在 benchmark 开始前完成。

完成 `runGpuInference`，使其能够：

1. 将图像清单划分为多个 batch，并处理不足一批的最后一组图像。
2. 在正式推理轮次中为每批调用 `prepareBatch`。
3. 调用 GPU 实现完成上传、计算和下载。
4. 将每张图像的 1000 个得分写入对应的 CPU 结果位置。
5. 等待本轮全部结果在 CPU 上就绪后返回。

先通过快速和完整验证，再记录基线性能。使用 Release 构建，在关闭 profiler 的情况下
运行 benchmark：

```sh
./project3 --benchmark --batch-size 8 --streams 1 --repeat 5
```

保存源码版本和输出，保证后续优化后仍能复现朴素基线。

### 4.2 用 Nsight Systems 观察朴素版本的时间线

在引入并发之前采集基线。为 CPU 准备、H2D、各层、D2H 和等待添加 NVTX 区间。
分析时排除初始化和预热。

Windows 可以直接使用 Nsight Systems GUI 完成采集与分析。
选择 `project3.exe`，设置工作目录，程序参数填写
`--benchmark --batch-size 8 --streams 1 --repeat 1`。
启用 CUDA 和 NVTX 采集，启动后查看预热之后的 GPU stream 轨道，
将报告保存到 `nsys_profile/`。

Linux 可以在可执行文件所在目录使用 Nsight Systems 命令行。下面的示例通过 benchmark 现有的
`cudaProfilerStart()` / `cudaProfilerStop()` 边界，仅采集正式推理：

```sh
cmake -E make_directory nsys_profile

nsys profile \
    --trace=cuda,nvtx \
    --sample=none \
    --capture-range=cudaProfilerApi \
    --capture-range-end=stop \
    -o nsys_profile/naive \
    ./project3 --benchmark \
    --batch-size 8 --streams 1 \
    --repeat 1

nsys stats nsys_profile/naive.nsys-rep
```

更多参数见 [Systems 使用指南](https://docs.nvidia.com/nsight-systems/UserGuide/index.html)。

识别 CPU 准备区间、内核执行、拷贝活动、GPU 空闲及同步。
区分 CPU API 耗时和 GPU 活动耗时：阻塞拷贝 API 可能先等待前序内核，
再真正执行拷贝。解释朴素版本中的哪些依赖阻止了重叠，
以及不同批次之间哪些工作可以并发执行。

根据时间线提出具体的流水线优化假设。

### 4.3 实现 CUDA stream 流水线

扩展 `AlexNetGpu` 和 `runGpuInference`，让不同批次的 CPU 准备、传输与 GPU 工作
能够重叠。支持 `--streams N` 指定的 stream 数量。
自行设计并实现各在途批次的存储和完成状态管理。

流水线需要满足以下生命周期与顺序要求：

- 每批的 H2D、依赖它的内核及 D2H 按正确顺序执行。
- 各在途批次独立持有输入、输出和中间结果存储；完成上传的模型参数可只读共享。
- H2D 完成之前，输入存储保持有效且内容不变。
- D2H 完成之后，CPU 才能读取下载结果；缓冲区的前一轮使用结束后才能复用。
- 最终 CPU 结果存储保持有效，直到对应输出已被收集，且结果与原图像顺序一致。
- 每轮推理返回前排空在途工作并收集全部输出。

使用 pinned 主机缓冲区、异步拷贝、stream 和完成事件实现这些保证。
给拷贝和 kernel launch 传入正确的 stream，在需要复用资源或结束整轮推理时等待。
不必要的 device-wide synchronization 会消除重叠。

固定 batch size，对比单 stream 和双 stream。两者都要通过验证，并采集优化后的
Systems 时间线。检查实际重叠、GPU 空闲和剩余等待，再在关闭 profiler 后测量完整耗时。
将采集配置中的程序参数改为 `--streams 2`，并使用新的报告名称。
根据 GPU 轨道确认实际重叠，不能只根据异步 API 的名称或 stream 数量判断。

### 4.4 用 Nsight Compute 分析内核

完成流水线后，从 Systems 中找出耗时的卷积和全连接内核。
两类内核都要选取有代表性的 launch 采集 Compute 报告。
比较内核时固定 batch size，并使用 `--streams 1`。

Windows 也可以直接使用 Nsight Compute GUI 完成内核采集与分析。
打开 **Start Activity**，选择 **Profile**，设置 `project3.exe`、工作目录和程序参数
`--benchmark --batch-size 8 --streams 1 --repeat 1`。
在 **Filter** 中选取目标内核并将采集数量限制为一次 launch，
在 **Sections** 中选择要收集的指标。关闭从程序启动时就开始采集的选项，
让程序中的 profiling API 调用排除预热。将结果保存到 `ncu_profile/`。
详见 [Compute GUI 指南](https://docs.nvidia.com/nsight-compute/NsightCompute/index.html)。

Linux 可以使用 Nsight Compute 命令行，例如从正式测量区间采集一次卷积内核 launch：

```sh
cmake -E make_directory ncu_profile

ncu \
    --profile-from-start off \
    --set basic \
    --kernel-name "regex:conv2d.*" \
    --launch-count 1 \
    -o ncu_profile/conv \
    ./project3 --benchmark \
    --batch-size 8 --streams 1 \
    --repeat 1
```

按实际实现替换内核名称表达式，对全连接内核重复同样的采集。
需要更多细节时使用 `--set full`。构建已启用 `-lineinfo`，性能分析不要使用
设备调试选项 `-G`。Compute 可能重放或串行化 launch，端到端结果应另行在无 profiler
的普通运行中测量。更多参数见
[Compute CLI 指南](https://docs.nvidia.com/nsight-compute/NsightComputeCli/index.html)。

分析访存流量与访问模式、计算和访存吞吐、warp stalls、寄存器/shared memory 用量
以及 occupancy。根据观察结果分别提出优化方案，并解释指标与假设的关系。
不能只凭单个 occupancy 或 utilization 数值判断瓶颈。

### 4.5 实现高效卷积和全连接层

使用自己编写的 CUDA 内核优化这两类算子。
可考虑复用输入或权重分块、改善合并访存、使用 shared memory、寄存器分块，
以及减少地址计算开销。卷积可以组织成矩阵乘法问题，批量 linear 层则是矩阵乘法加 bias。
根据本网络的实际工作量选择算法和 launch 配置。

保留可复现朴素基线的方式。对于每类算子，使用优化前后的 Compute 指标评估优化假设，
检查正确性，并在普通运行中测量端到端性能。
归因于 kernel 的性能变化需要固定 batch size 和 stream 数量。
如果优化引入额外内核或中间拷贝，对比时需要计入这些开销。

迭代期间运行快速验证，完成卷积和全连接优化后运行完整验证。

### 4.6 调整 batch size 和 stream 数量

对优化版本测试 batch size `1、2、4、8、16、32、64、128、256`
及 stream 数量 `1、2、4`。记录超出主机或显存容量的配置并解释原因，
也可以在较优配置附近增加测试点。

固定图像数量、随机种子、权重、预处理、构建配置、warmup 和 repeat。
进行多次独立运行，比较代表性性能及波动。
测量 1024 张图像的总耗时、images/s 和主机/显存缓冲区占用。
不预设更多 stream 或更大 batch 一定更快。

选择实测最优配置，在完整数据集上验证，并检查它的 Systems 时间线。
计算相对已保存朴素 GPU 基线的总体加速比，明确两个版本各自的参数。

## 5. 正确性要求

- 使用标准 C++/CUDA 实现完整网络和全部 1000 个输出类别，
  不调用 PyTorch、cuDNN、cuBLAS、BLAS、CUTLASS 或 OpenMP。
- 保持权重、网络行为、预处理和原始类别索引不变。
  不跳过层或传输，不缓存最终预测，不利用真实标签修改预测，
  不将预测范围缩减为数据子集包含的类别。
- 使用 `--validate` 和真实图像。CPU/GPU 的 Top-1 预测和 Top-5 集合
  都必须在每张图像上保持一致。另行报告 CPU 和 GPU 相对真实标签的 Top-1/Top-5 准确率。
  验证通过以 CPU/GPU 在每张图像上的预测一致为准，不要求分类准确率达到 100%。
- 检查多批次、缓冲区反复复用、尾 batch，以及批容量大于图像数的情况。
- 保存朴素基线和最终版本的预测 CSV 与源码版本。
  ground truth 使用数据集原始标签，保持图像顺序一致。

## 6. 计时与对照

使用提供的 `base` `Timer` 测量全部正式轮次的总耗时，再除以 `repeat`。
每轮包含 CPU 准备、必要暂存和拷贝、launch、调度、同步、流水线启动和排空，
以及 CPU 输出全部就绪。合成源图像生成、模型初始化与上传、资源分配及预热
保留在计时区间之外。

至少预热一次、正式测量五次。合成图像用于 benchmark，真实图像单独用于分类验证。
CPU 推理作为正确性参考，不计入 GPU benchmark。

分别比较朴素基线、stream 流水线、内核优化和最终参数调优四个阶段。
对比 stream 时固定 batch size；对比 kernel 时同时固定 batch size 和 stream 数量。
总体加速比需要明确说明最终参数发生的变化。
正式计时来自普通运行，不能使用 profiler 重放时间或重叠阶段耗时之和代替端到端时间。

## 7. 提交内容

需要提交：

- `projects/project3` 下补全后的源代码，包括 `AlexNetGpu` 和 `runGpuInference`；
- 重建项目所需的 CMake 配置和共享 `base/` 头文件；
- 位于 `projects/project3/report/REPORT.md` 的简短报告。

报告需要说明：

- 软硬件环境与 benchmark 设置；
- GPU 实现与 stream 流水线，包括存储资源的归属和同步方式；
- 快速与完整验证的结果，包括 CPU/GPU 预测一致率及相对真实标签的准确率；
- 使用 Nsight Systems 和 Nsight Compute 观察到的瓶颈、据此进行的修改，
  以及支持这些修改的测量数据；
- 朴素基线、stream 流水线、内核优化和最终参数调优各阶段的端到端耗时与吞吐量，
  并注明各次比较使用的 batch size 和 stream 数量；
- batch size / stream 数量的对比、相对朴素 GPU 基线的最终加速比，以及已知限制。

报告所需的配图放在同一个 `report/` 目录下。提交目标会将整个目录打入 `project3.zip`。

不要提交 `assets/`、`images/` 和 `scripts/`。运行解压后的项目时，
通过 `--assets DIR` 单独加载提供的模型与验证数据集。

在报告中总结 profiling 观察和关键测量数据。无需另外提交 Nsight 原始采集文件、
时间线截图、指标导出或预测 CSV。原始 profiling 输出保留在 `nsys_profile/`
或 `ncu_profile/`，不放入提交包。

源码提交中不要包含构建目录、包管理器的检出目录、自动生成的 Visual Studio 文件或
与本机环境有关的产物。完成仓库构建配置后，使用提供的提交目标，提交生成的 `project3.zip`。

在 Windows 上，从 Windows Terminal 的 Developer PowerShell for VS 中执行：

```powershell
cmake --build --preset windows-vs-release-vcpkg --target submit_project3
```

在 Linux 上：

```sh
cmake --build --preset linux-gcc-release-vcpkg --target submit_project3
```
