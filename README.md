# GPU-projects

[English](README.md) | [简体中文](README.zh-CN.md)

GPU computing assignments and projects for graduate students. The repository
uses CMake to build CUDA examples, standalone assignments, and a reusable set
of CPU, GPU, and OpenGL support modules.

## Repository structure

```text
.
├── base/       Shared CPU, GPU, and graphics modules
├── examples/   Standalone CUDA examples
├── projects/   Course assignments and larger projects
└── cmake/      Submission-package helpers
```

The `base/` directory is intentionally not a standalone CMake target. Each
project lists the shared files it uses in its own `CMakeLists.txt`, so
dependencies remain explicit and submission packages can include only the
required files.

## Requirements

- CMake 3.25 or newer
- A C++20 compiler
- CUDA Toolkit with `nvcc` and the CUDA runtime development files
- A CUDA-capable GPU and a compatible driver for running CUDA targets
- vcpkg, or equivalent pre-installed packages, when building targets that use
  third-party libraries

## Dependency setup with vcpkg

The recommended setup is to clone vcpkg into the repository root:

```sh
git clone https://github.com/microsoft/vcpkg.git vcpkg
```

Bootstrap vcpkg once:

```sh
# Windows PowerShell
.\vcpkg\bootstrap-vcpkg.bat

# Linux/macOS
./vcpkg/bootstrap-vcpkg.sh
```

Then edit `CMakeUserPresets.json.example`. Replace each
`$env{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake` with
`${sourceDir}/vcpkg/scripts/buildsystems/vcpkg.cmake`, and copy the result to
`CMakeUserPresets.json`:

```sh
cp CMakeUserPresets.json.example CMakeUserPresets.json
```

Choose a `*-vcpkg` configure/build preset afterwards. CMake uses the
repository's `vcpkg.json` in manifest mode and installs the declared packages
automatically for the selected triplet.

## Configure and build

The committed `CMakePresets.json` contains provider-neutral presets for Visual
Studio, GCC, and Clang. After following the vcpkg setup above, use one of the
`*-vcpkg` presets.

On Windows, the recommended workflow is to open the repository directory
directly in Visual Studio. In the CMake configuration selector, choose either
`windows-vs-debug-vcpkg` or `windows-vs-release-vcpkg`, then build the desired
target from Visual Studio.

From Developer PowerShell for VS in Windows Terminal, run the equivalent
Windows commands:

```sh
cmake --preset windows-vs-release-vcpkg
cmake --build --preset windows-vs-release-vcpkg
```

On Linux, use the corresponding preset directly from a terminal:

```sh
# GCC release
cmake --preset linux-gcc-release-vcpkg
cmake --build --preset linux-gcc-release-vcpkg
```

For debug builds, replace `release` with `debug` in the preset name.

If dependencies are already installed and visible to CMake, use a
provider-neutral preset such as `linux-gcc-release` instead:

```sh
cmake --preset linux-gcc-release
cmake --build --preset linux-gcc-release
```

## Submission

Every project under `projects/` provides a `submit_<project-name>` target. For
example, to package `project1`:

On Windows, run the following command from Developer PowerShell for VS in
Windows Terminal:

```sh
cmake --build --preset windows-vs-release-vcpkg --target submit_project1
```

On Linux:

```sh
cmake --build --preset linux-gcc-release-vcpkg --target submit_project1
```

The resulting archive is written below the selected preset's binary directory:
`out/build/<preset>/projects/<project-name>/submission/<project-name>.zip`.

## Project 0: CUDA device query

`projects/project0` uses the CUDA Runtime and Driver APIs, along with NVML,
to report properties and capabilities for every detected CUDA device. CUDA
Core counts use NVML when available and otherwise fall back to an architecture
table. The program also checks peer access between devices when multiple GPUs
are available.

## Project 1: Mandelbrot renderer

`projects/project1` is a Mandelbrot renderer with interactive OpenGL and
one-shot offline PNG modes. It supports high-precision center/zoom values,
configurable resolution, and 1x1 or 2x2 sampling. The
`PROJECT1_OFFLINE_ONLY` CMake option builds it without the online OpenGL
dependencies. See the [project1 README](projects/project1/README.md) for
usage, command-line options, controls, and offline-build details.

## Project 2: OptiX ray tracer

`projects/project2` renders three Lucy models, a ground sphere, a randomized
sphere field, and a skybox with OptiX. It provides a progressive OpenGL viewer
and offline PNG rendering. Offline mode has configurable sample count and
denoising; the viewer has a denoiser toggle. The `PROJECT2_OFFLINE_ONLY` CMake
option removes the viewer's OpenGL and GLFW dependencies. Project 2 requires a
compatible NVIDIA GPU, CUDA, and OptiX 9.1. See the
[project2 README](projects/project2/README.md) for
usage, command-line options, controls, and offline-build details.

## Project 3: AlexNet inference

`projects/project3` performs AlexNet batch inference with CUDA. It provides
benchmarking on synthetic images and CPU/GPU prediction validation on real
images, with configurable batch size and stream count. Students use Nsight
Systems and Nsight Compute to analyze the pipeline, overlap transfers and
computation with CUDA streams, and optimize convolution and fully connected
kernels. Pretrained weights and validation images are included; retrieving
the weights requires Git LFS. See the
[project3 README](projects/project3/README.md) for dependencies, build
instructions and command-line options.
