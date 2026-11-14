# Project 3 Assignment: AlexNet Inference

[English](ASSIGNMENT.md) | [简体中文](ASSIGNMENT.zh-CN.md)

## 1. Overview

Build a complete CUDA implementation of AlexNet batch inference. Start with a
correct naive version, study its timeline in Nsight Systems, add a stream
pipeline, analyze kernels in Nsight Compute, optimize convolution and fully
connected layers, and finally tune batch size and stream count.

The measured task is CPU input preparation → H2D → complete AlexNet inference
→ D2H → completed CPU outputs. The objective is to reduce the time for the
whole task while preserving the predictions of the supplied CPU reference.
See [README.md](README.md) for the model, dependencies, build and run commands.

## 2. Learning objectives

This assignment practices:

- implementing convolution, activation, pooling and fully connected layers;
- managing host/device storage and the lifetime of asynchronous operations;
- reading CUDA API, GPU activity and NVTX tracks in Nsight Systems;
- overlapping transfers and computation across batches with CUDA streams;
- using Nsight Compute evidence to improve convolution and linear kernels;
- measuring end-to-end performance and selecting batch/stream parameters.

## 3. Project scope and supplied code

Implement the entire `AlexNetGpu` class in `include/alexnet_gpu.h` and
`src/alexnet_gpu.cu`, including its GPU kernels and resource management.
Implement `runGpuInference` in `src/main.cpp` to drive a complete pass.
The stream pipeline is part of your implementation work.

The project supplies the CPU model, network descriptions, parameter loader,
image loading/preprocessing, classification metrics, command-line handling
and benchmark/validation harness. Reuse these components. Use the shared
`gpu::Buffer`, `gpu::PinnedBuffer`, `gpu::Stream`, `gpu::Event` and `Timer`
where appropriate.

Preserve the `project3` executable, its command-line options and the
`submit_project3` target. Keep the CPU reference, model, preprocessing and
label mapping consistent throughout the experiments. The supplied pretrained
weights and image/label lists are sufficient for the assignment.

## 4. Required work

### 4.1 Implement a naive GPU baseline

Complete the `AlexNetGpu` class, including:

- allocation and cleanup of input, output, parameter and intermediate storage;
- one-time upload of model weights and biases;
- convolution, ReLU, max pooling and fully connected CUDA kernels;
- the complete forward pass in the order defined by `alexnet_common.h`;
- input upload and output download for the actual image count in each batch.

Use straightforward kernels and synchronous copies with `--streams 1`.
Keep the existing public interface usable; the initial implementation can
complete each batch before processing the next. Allocate reusable storage and
upload the parameters before the benchmark begins.

Complete `runGpuInference` so that it:

1. divides the image list into batches, including a partial final batch;
2. calls `prepareBatch` for each batch inside the measured pass;
3. invokes the GPU implementation to upload, compute and download;
4. places each image's 1000 output scores in the corresponding CPU result slot;
5. returns only when all results for the pass are ready on the CPU.

Pass quick and full validation before measuring the baseline. Use a Release
build and run the benchmark without a profiler:

```sh
./project3 --benchmark --batch-size 8 --streams 1 --repeat 5
```

Save the source revision and output so this baseline remains available after
optimization.

### 4.2 Inspect the naive timeline with Nsight Systems

Profile the baseline before adding concurrency. Mark CPU preparation, H2D,
network layers, D2H and waits with NVTX. Exclude initialization and warmup from
the analysis.

On Windows, you can collect and analyze traces directly in the Nsight Systems
GUI. Select `project3.exe`, set the working directory and use application
arguments `--benchmark --batch-size 8 --streams 1 --repeat 1`.
Enable CUDA and NVTX tracing, start the collection
and inspect the GPU stream tracks after warmup. Save the report under
`nsys_profile/`.

On Linux, use the Nsight Systems command line from the executable directory.
This example uses the benchmark's `cudaProfilerStart()` / `cudaProfilerStop()`
boundaries to capture only the measured pass:

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

See the [Systems guide](https://docs.nvidia.com/nsight-systems/UserGuide/index.html)
for profiling options.

Identify CPU preparation intervals, kernel execution, memory-copy activity,
GPU idle periods and synchronization. Distinguish CPU API durations from GPU
activity durations: a blocking copy API may wait for earlier kernels before
the actual copy executes. Explain which dependencies prevent overlap in the
naive version and which work can run concurrently across different batches.

Use the timeline to form a concrete pipeline optimization hypothesis.

### 4.3 Implement a CUDA stream pipeline

Extend `AlexNetGpu` and `runGpuInference` so that different batches can overlap
CPU preparation, transfers and GPU work. Support the requested `--streams N`.
Design and implement the storage and completion tracking for in-flight batches.

The pipeline must satisfy these lifetime and ordering requirements:

- H2D, dependent kernels and D2H execute in the correct order for each batch.
- In-flight batches have independent input, output and intermediate storage;
  completed model parameters may be shared read-only.
- Input storage remains valid and unchanged until its H2D operation completes.
- CPU code reads downloaded results only after D2H completes. A buffer is
  reused only after its previous users have finished.
- Final CPU result storage remains valid until the corresponding output has
  been collected. Results retain the original image order.
- Each full pass drains pending work and collects all outputs before returning.

Use pinned host buffers, asynchronous copies, streams and completion events
to implement these guarantees. Pass the intended stream to the copies and
kernel launches. Place waits where resources must be reused or a pass must
finish; unnecessary device-wide synchronization can remove overlap.

At the same batch size, compare one stream with two streams. Validate both
and capture a second Systems trace. Check the actual overlap, GPU idle time
and remaining waits, then measure the complete pass without the profiler.
Repeat the capture with `--streams 2` and a different report name.
Confirm overlap on the GPU tracks rather than inferring it from asynchronous
API names or the number of streams.

### 4.4 Analyze kernels with Nsight Compute

After establishing the pipeline, identify expensive convolution and fully
connected kernels in Systems. Collect Compute reports for representative
launches of both, using a fixed batch size and `--streams 1` for kernel
comparisons.

On Windows, you can also collect and analyze kernel profiles directly in the
Nsight Compute GUI. Open **Start Activity**, select **Profile**, and set
`project3.exe`, its working directory and application arguments
`--benchmark --batch-size 8 --streams 1 --repeat 1`.
In **Filter**, select the target kernel and limit collection to one
launch. In **Sections**, choose the metrics to collect. Disable profiling from
application start so the application's profiling API calls can exclude
warmup. Save the results under `ncu_profile/`. See the
[Compute GUI guide](https://docs.nvidia.com/nsight-compute/NsightCompute/index.html).

On Linux, use the Nsight Compute command line. For example, capture one
convolution launch from the measured region:

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

Replace the kernel-name expression to match your implementation, and repeat
for a linear kernel. Collect `--set full` when more detail is needed. The build
enables source correlation with `-lineinfo`; do not use device-debug `-G` for
performance work. Compute may replay or serialize launches, so use separate
unprofiled runs for end-to-end results. See the
[Compute CLI guide](https://docs.nvidia.com/nsight-compute/NsightComputeCli/index.html)
for profiling options.

Analyze memory traffic and access patterns, compute and memory throughput,
warp stalls, register/shared-memory usage and occupancy. Use the observations
to propose a change to each operator. Explain the relationship between the
metrics and your hypothesis; a single occupancy or utilization value is not
a complete diagnosis.

### 4.5 Implement efficient convolution and fully connected layers

Optimize both operator families using your own CUDA kernels. Possible
approaches include reusing input/weight tiles, improving coalescing, shared
memory, register blocking and reducing address-calculation overhead.
Convolution can be organized as a matrix-multiplication problem; a batched
linear layer is a matrix multiplication plus bias. Choose algorithms and
launch shapes based on the workloads in this network.

Retain a way to reproduce the naive baseline. For each operator, use before/after
Compute measurements to evaluate the optimization hypothesis, check correctness
and measure unprofiled end-to-end performance. Keep batch size and stream count fixed
when attributing a performance change to a kernel optimization. If an
optimization introduces extra kernels or intermediate copies, include their
cost in the comparison.

Run quick validation while iterating and full validation after completing
the optimized convolution and fully connected implementations.

### 4.6 Tune batch size and stream count

Use the optimized implementation to compare batch sizes
`1, 2, 4, 8, 16, 32, 64, 128, 256` and stream counts `1, 2, 4`.
Record configurations that exceed available host or device memory and explain
the limit. You may test additional values around a promising configuration.

Keep image count, seed, weights, preprocessing, build settings, warmup and
repeat count fixed. Repeat independent runs and compare both representative
performance and variation. Measure total time for 1024 images, images/s and
host/device buffer usage. More streams or a larger batch are not assumed to
improve performance.

Select the best measured configuration, validate it on the full dataset and
check its Systems timeline. Calculate overall speedup against the saved naive
GPU baseline, with the parameters of both versions stated explicitly.

## 5. Correctness requirements

- Implement the complete model and all 1000 output classes using standard
  C++/CUDA, without PyTorch, cuDNN, cuBLAS, BLAS, CUTLASS or OpenMP.
- Preserve weights, network behavior, preprocessing and canonical class indices.
  Do not skip layers or transfers, cache final predictions, change predictions
  using true labels, or restrict predictions to the dataset's subset classes.
- Use `--validate` with real images. Both CPU/GPU Top-1 agreement and Top-5 set
  agreement must match on every image. Report CPU and GPU Top-1/Top-5 accuracy
  against ground-truth labels separately. Validation passes when CPU/GPU
  predictions agree on every image; classification accuracy need not be 100%.
- Exercise multiple batches, repeated buffer reuse, a partial final batch and
  capacity larger than the image count.
- Keep prediction CSVs and source revisions for the naive baseline and final
  version. Use dataset labels as ground truth and preserve the image order.

## 6. Timing and comparisons

Use the supplied `base` `Timer` around all measured passes and divide the total
by `repeat`. Each pass includes CPU preparation, required staging/copies,
launches, scheduling, synchronization, pipeline fill/drain and completed CPU
outputs. Generation of synthetic source images, model initialization/upload,
allocation and warmup remain outside the measured region.

Use at least one warmup and five measured passes. Benchmark on synthetic
images; validate classification on real images separately. CPU inference is
the correctness reference and is not part of the GPU benchmark.

Compare the naive baseline, stream pipeline, optimized kernels and final tuned
configuration as separate stages. For stream experiments, hold batch size
fixed. For kernel experiments, hold batch size and stream count fixed. State
any parameter changes when reporting the final overall speedup. Final timing
comes from normal runs; profiler replays and sums of overlapping stage times
do not replace the end-to-end measurement.

## 7. Deliverables

Submit:

- completed source under `projects/project3`, including `AlexNetGpu` and
  `runGpuInference`;
- the CMake configuration and shared `base/` headers required to rebuild the
  project;
- a short report at `projects/project3/report/REPORT.md`.

The report must describe:

- the hardware/software environment and benchmark settings;
- the GPU implementation and stream pipeline, including storage ownership and
  synchronization;
- quick and full validation results, including CPU/GPU prediction agreement
  and ground-truth accuracy;
- the bottlenecks observed with Nsight Systems and Nsight Compute, the changes
  made in response, and the measurements supporting those changes;
- end-to-end time and throughput for the naive baseline, stream pipeline,
  optimized kernels and final tuned configuration, with batch size and stream
  count stated for each comparison;
- batch-size/stream-count comparisons, final speedup against the naive GPU
  baseline, and any known limitations.

Place any supporting figures in the same `report/` directory. The submission
target includes the complete directory in `project3.zip`.

Do not submit `assets/`, `images/` or `scripts/`. The supplied model and
validation dataset are loaded separately through `--assets DIR` when running
the extracted project.

Summarize profiling observations and key measurements in the report. Raw
Nsight capture files, separate timeline screenshots, metric exports and
prediction CSVs are not required. Keep raw profiling output in `nsys_profile/`
or `ncu_profile/`, outside the submission.

Do not include build directories, package-manager checkouts, generated Visual
Studio files or machine-specific artifacts in the source submission. From a
configured repository build, use the provided target and submit the generated
`project3.zip`.

On Windows, run the following command from Developer PowerShell for VS in
Windows Terminal:

```powershell
cmake --build --preset windows-vs-release-vcpkg --target submit_project3
```

On Linux:

```sh
cmake --build --preset linux-gcc-release-vcpkg --target submit_project3
```
