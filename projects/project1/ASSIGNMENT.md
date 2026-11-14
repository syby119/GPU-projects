# Project 1 Assignment: CUDA Mandelbrot Renderer

[English](ASSIGNMENT.md) | [简体中文](ASSIGNMENT.zh-CN.md)

## 1. Overview

Complete the starter implementation of `project1`, a CUDA Mandelbrot-set
renderer. The program must support both of the modes already provided by the
starter project:

- **Online mode:** render continuously in an OpenGL window.
- **Offline mode:** render one image with CUDA and save it as an RGBA PNG.

The starter repository intentionally leaves several implementation points
empty. Your task is to complete those points while preserving the existing
program structure and command-line interface.

The online and offline paths must use the same CUDA rendering entry point and
the same pixel-generation kernel. Online rendering exchanges pixels through a
CUDA-registered OpenGL shader-storage buffer object (SSBO); do not replace this
with an OpenGL texture or a CPU-side pixel loop.

## 2. Learning objectives

This assignment is intended to practice:

- mapping a two-dimensional image to CUDA thread blocks and grids;
- implementing complex-number iteration on the GPU;
- smooth escape-time coloring and supersampling anti-aliasing;
- passing a trivially copyable parameter block from C++ to CUDA;
- coordinating CUDA with an OpenGL SSBO through CUDA/OpenGL interoperation;
- keeping high-precision camera state on the CPU while supplying suitable
  double-precision values to the GPU;
- organizing one renderer so that it can be used by both an interactive and an
  offline application.

## 3. Starter code and allowed changes

The files under `projects/project1` are the assignment area. You may add
private helper functions or additional files under this directory, and you may
extend the renderer function signature when implementing the optional
perturbation/reference-data bonus. Keep the public behavior of the existing
command-line options and build targets unchanged.

The shared `base/` directory is support code and should not be modified for
this assignment. `palette.cu` and the palette interface are provided; use the
provided palette rather than replacing it with a CPU color-generation path.

## 4. Required work

### 4.1 Complete the render parameter block

Complete `render_paramters.h` with every value required by the host and device
renderer. The block must contain enough information to describe:

- image width and height;
- maximum iteration count and pixel scale;
- 1x1 versus 2x2 sampling;
- palette parameters;
- the direct-render center;
- any additional camera-delta, reference-orbit, or series-approximation data
  used by the optional bonus.

The parameter block is copied to device code by value. It must therefore remain
trivially copyable, contain no owning pointers or C++ library objects with
non-trivial state, and have the same interpretation in host and device code.

### 4.2 Implement the CUDA Mandelbrot sampling

Complete the missing sampling code in `src/mandelbrot.cu`.

For each sample, map the pixel coordinate to the complex plane using the image
center, image dimensions, pixel scale, and the existing sub-pixel offsets. The
Mandelbrot recurrence is

$$
z_0 = 0, \qquad z_{n+1} = z_n^2 + c
$$

Use the supplied escape radius ($|z|^2 > 256$) and stop after the configured
maximum number of iterations. Escaped samples must use the supplied smooth
iteration helper; samples that do not escape must be represented as interior
samples and rendered black by the existing shading path.

The kernel must support both sampling modes already exposed by the starter
code:

- **1x1:** one sample at the pixel center;
- **2x2:** four samples at the existing quarter-pixel offsets, averaged before
  writing the output.

Write one `float4` per pixel, with RGB produced by `Palette::sample()` and an
alpha value of `1.0`.

### 4.3 Implement and launch the CUDA kernel

Complete the host-side `renderMandelbrotSet` function.

Requirements:

- launch a two-dimensional grid that covers the complete image;
- perform a bounds check in the kernel so non-multiple image dimensions are
  safe.

If you implement the optional perturbation or series bonus, pass its data to
the kernel without copying a full image through the CPU.

The renderer must remain asynchronous with respect to the caller unless a
synchronization is required by the surrounding resource-ownership contract.

### 4.4 Complete the offline renderer

Complete the TODOs in `src/mandelbrot_offline_render.cpp`.

Build a valid `RenderParameters` object from `OfflineRenderConfig`, including
the requested center, zoom, dimensions, anti-aliasing mode, pixel scale, and
palette. Call the same `renderMandelbrotSet` entry point used by online mode.
Keep the existing device-to-host copy, vertical image flip, and PNG-writing
behavior intact.

The following must work without creating an OpenGL context:

```text
./project1 --offline
./project1 --offline --width 640 --height 360 --aa 1 --zoom 1 --output result.png
```

### 4.5 Complete the online compute path

Complete the missing parts of `MandelbrotCompute` and its implementation.

The online renderer stores camera coordinates as `HighPrecisionComplex` and
`HighPrecisionFloat` on the CPU. Use that state to compute the current camera
position and pixel scale for each frame. Do not reduce the complete camera
animation to ordinary `double` values before the camera calculation.

The implementation must also:

- react correctly to framebuffer-resize events;
- fill `RenderParameters` for the current frame and invoke the shared CUDA
  renderer;
- return meaningful frame information for the existing window title.

#### Optional bonus: perturbation and reference data

At ordinary zoom levels, the required direct iteration path is sufficient.
For bonus credit, implement perturbation or another numerically stable method
for the deep zoom used by the default animation. A bonus implementation may
maintain a reference orbit and use the perturbation recurrence instead of
converting the complete high-precision center to a rounded `double` value.

The series-approximation acceleration exposed by the starter interfaces is also
optional. If you implement it, validate its pointer, order, count, base,
radius, and iteration range before using it, and fall back safely when the
approximation is unavailable or outside its valid range.

### 4.6 Preserve CUDA/OpenGL SSBO interoperation

The online viewer maps the color SSBO, obtains a CUDA pointer, renders into
that pointer, unmaps the resource, and then draws the SSBO in OpenGL. Preserve
this ownership sequence. In particular:

- never access the mapped SSBO from OpenGL while CUDA owns it;
- never access the CUDA pointer after unmapping;
- recreate and re-register the buffer when the framebuffer size changes;
- do not substitute a texture-based CUDA interop implementation.

## 5. Functional requirements

A correct submission must satisfy all of the following:

1. The project builds with the repository's documented CMake presets.
2. Offline-only builds work with `PROJECT1_OFFLINE_ONLY=ON` and do not require
   OpenGL, GLFW, or GLM.
3. Online mode opens a window and continuously updates the Mandelbrot image.
4. Offline mode writes a valid PNG at the requested resolution and output path.
5. Changing center, zoom, resolution, and anti-aliasing changes the rendered
   result as expected.
6. Interior points are black, while escaped points receive smooth palette
   colors.
7. The implementation does not introduce a CPU loop over every output pixel.

## 6. Suggestions

Use offline mode first, because it makes experiments repeatable. Render a
`640x360` image while increasing the zoom manually, for example:

```text
./project1 --offline --width 640 --height 360 --aa 0 --zoom 1000
./project1 --offline --width 640 --height 360 --aa 0 --zoom 10000
./project1 --offline --width 640 --height 360 --aa 0 --zoom 100000
```

Continue with larger zoom values and observe when ordinary double-precision
iteration begins to lose detail or produce an unstable image. After the
offline renderer is correct, run the windowed program and watch the camera
zoom continuously toward the default target center and zoom. Compare the
interactive result with the offline images and investigate any visible loss of
precision, discontinuity, or incorrect coloring. These experiments are also a
useful way to evaluate the optional perturbation and series bonus.

## 7. Deliverables

Submit:

- the completed source under `projects/project1`;
- a short `report/REPORT.md` under `projects/project1/report` containing:
  - a brief description of the CUDA kernel and pixel mapping;
  - how 1x1/2x2 sampling is implemented;
  - how online SSBO interoperation is synchronized;
  - how the high-precision camera is handled;
  - if implemented, how perturbation, reference-orbit, or series data are
    handled as part of the optional bonus;
  - one offline screenshot and the command used to produce it;
  - a short note about performance or any known limitations.

Place any report screenshots or other supporting files in the same `report/`
directory. The submission target includes the complete `report/` directory in
the generated archive.

Do not submit build directories, package-manager checkouts, generated Visual
Studio files, or other machine-specific artifacts. From a configured
repository build, use the provided target.

On Windows, run the following command from Developer PowerShell for VS in
Windows Terminal:

```powershell
cmake --build --preset windows-vs-release-vcpkg --target submit_project1
```

On Linux:

```sh
cmake --build --preset linux-gcc-release-vcpkg --target submit_project1
```
