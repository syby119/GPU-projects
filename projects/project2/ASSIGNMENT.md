# Project 2 Assignment: OptiX Ray Tracer

[English](ASSIGNMENT.md) | [简体中文](ASSIGNMENT.zh-CN.md)

## 1. Overview

Build an OptiX ray tracer. The finished `project2` program must render the
provided scene consisting of three Lucy models, a ground sphere, a field of
smaller spheres, and a skybox. Offline mode must render a chosen number of
samples per pixel to a PNG. Builds with OpenGL also provide an interactive
viewer that progressively refines the image.

## 2. Learning objectives

This assignment is intended to practice:

- building triangle and built-in sphere acceleration structures with OptiX;
- instancing shared geometry and organizing an IAS;
- creating OptiX modules, program groups, a pipeline, and shader binding table
  (SBT) records;
- tracing multiple material bounces and accumulating Monte Carlo samples;
- using a cubemap as environment lighting with correct color-space conversion.

## 3. Project scope and shared code

Keep scene-specific code and assets under `projects/project2`. The shared
`base/` directory may be extended for reusable OptiX, CUDA, and graphics
wrappers, provided that other projects continue to build. Preserve the
repository's CMake presets, `project2` executable target, and
`submit_project2` source-package target.

The supplied `assets/models/lucy.obj` and `assets/skyboxrt/` images are part of
the scene. Default asset lookup must be independent of the process's working
directory. Support `--assets DIR` for an external runtime asset directory;
explicit relative paths are resolved from the current working directory.

## 4. Required work

### 4.1 Build the scene acceleration structures

Load the Lucy OBJ as triangles and build one triangle geometry acceleration
structure (GAS). Reuse that GAS for three Lucy instances with separate static
transforms. Build another GAS for the ground and smaller spheres using OptiX's
built-in sphere primitive. Combine the three Lucy instances and the sphere
instance in an instance acceleration structure (IAS).

Use separate hit programs and SBT records for triangles and spheres. Associate
surface normals and materials with the correct geometry. Position the camera
and models so that the three Lucy instances, ground, sphere field, and skybox
are visible. Assign Lambertian, metal, and dielectric materials across the
scene. Use a fixed random seed for the sphere layout so output can be compared
across runs.

Complete the `__closesthit__triangle` and `__closesthit__sphere` TODOs in
`src/optix_programs.cu`. Read the hit record through the SBT, interpolate
triangle normals from barycentric coordinates, and calculate sphere normals
from the hit position and center. Store each hit's normal, material, and
surface identity in the ray payload.

### 4.2 Trace and shade paths

Compile the device programs to OptiX IR and construct a pipeline with raygen,
miss, and closest-hit program groups. The raygen program must trace paths for
up to 16 bounces without recursive host-side tracing. Implement Lambertian,
fuzzy metal, and dielectric scattering. Keep radiance and path throughput
separate so each bounce contributes the right amount of light.

Sample the six skybox images through a cubemap for missed rays. Convert their
sRGB JPEG values to linear light before using them for illumination, and
convert final RGB values to sRGB for display and PNG output. Use floating-point
sampling and keep the accumulated image in floating point.

Complete the marked raygen TODOs in `src/optix_programs.cu`: sample the skybox
when a path escapes the scene, and choose a scattered direction according to
the hit material. Apply path throughput to the sky contribution.

### 4.3 Accumulate samples

Use separate OptiX launches for successive samples, then maintain a running
average of raw linear radiance. The offline renderer must honor its requested
sample count. When a viewer is built, it must add samples over time and clear
the average when the camera moves or the framebuffer size changes.

Complete the OptiX launch TODO in `OptixRenderer::render`: pass the camera and
sample state through launch parameters, launch the requested samples, and
advance sample and frame indices correctly. Complete the `OptixRenderer::resize`
TODO by allocating per-pixel rendering resources for the requested dimensions.

### 4.4 Complete the viewer for OpenGL builds

When OpenGL support is enabled, present the OptiX result in a window through a
CUDA-registered OpenGL buffer. CUDA must own the buffer only while it is mapped;
OpenGL may read it after unmapping. Handle framebuffer resize by recreating the
output storage, resizing the renderer resources, and resetting accumulation.

Use a camera with position and quaternion rotation, controlled separately from
the camera state. Support WASD and Q/E movement, mouse-wheel movement, and
Escape to close the viewer. Camera motion must reset progressive accumulation.

### 4.5 Complete offline rendering

Support `--offline` without creating an OpenGL context. Accept image width and
height, positive sample count, and PNG output path from the command line. The
defaults are 1920x1080 and 64 samples per pixel. If no output path is supplied,
generate a filename containing the resolution and sample count. Print the
offline render time.

The resulting PNG must have the requested dimensions and show the provided
scene. When a viewer is built, it must show the same scene. Keep the output path
relative to the current working directory unless an absolute path is supplied.

### 4.6 Build and stage runtime assets

Build the OptiX IR from `src/optix_programs.cu` as part of the CMake build.
Place it under `assets/optix/` beside the executable together with the Lucy
OBJ and skybox images. Use the first visible GPU's compute capability for the
IR target when it can be queried, falling back to `compute_75` when it cannot.
When source assets are absent, skip copying the model and images while still
building and staging the IR. Allow `PROJECT2_OPTIX_IR` to select the generated
IR output file, defaulting to `assets/optix/project2.optixir` under the project's
build directory. `--assets` selects the root for the model, skybox and IR.

Support `PROJECT2_OFFLINE_ONLY=ON` without GLAD, GLFW, or a window-system
development stack. The `submit_project2` target packages the project source and
shared `base/` files, excluding the provided scene assets.

## 5. Bonus: Denoising

Optionally, add an OptiX denoiser that consumes the accumulated image and
surface guides. In OpenGL builds, temporal denoising can use motion data and
history when successive frames can use them safely. Keep denoised output
separate from the raw sample average. This corresponds to the denoising TODO in
`OptixRenderer::render`; resize the guide and history buffers if you implement
it.

If implemented, support `--denoiser on|off|1|0` in offline mode and let the
viewer's Space key toggle denoising when a viewer is built. The output filename
may distinguish `raw` from `denoised` when no path is supplied. Denoising and
its controls are not required for the base assignment.

## 6. Functional requirements

A correct implementation must satisfy all of the following:

1. Debug and Release builds complete with the documented Windows CMake presets.
2. In an OpenGL build, the viewer shows three Lucy models, the sphere field,
   ground, and skybox; camera movement and resizing reset the sample average.
3. Offline mode writes a valid PNG at the requested resolution and path.
4. Increasing the offline sample count reduces Monte Carlo noise in raw output.
5. The executable loads assets and OptiX IR from its adjacent `assets/`
   directory when launched by path, including from another working directory.
   `--assets DIR` overrides the root directory for the model, skybox and IR
   in both modes.
6. An offline-only build renders without initializing OpenGL or GLFW.
7. `submit_project2` creates a source archive without `assets/`.

## 7. Suggested validation

Start with small offline renders to verify path and asset loading:

```text
project2 --offline --width 320 --height 180 --samples 1 --output sample-1.png
project2 --offline --width 320 --height 180 --samples 64 --output sample-64.png
```

Check that the same scene appears in both images and compare them for noise
reduction. In OpenGL builds, move and resize the viewer to confirm that
accumulation resets. If you implement the denoising bonus and enable it by
default, use `--denoiser off` for the sample comparison. For bonus validation,
compare raw and denoised output at the same sample count. In OpenGL builds,
also check the viewer toggle and temporal stability.

## 8. Deliverables

Submit:

- completed source under `projects/project2` and any reusable shared-code
  changes required by it;
- a short `report/REPORT.md` under `projects/project2/report` describing the
  GAS/IAS and SBT layout, material sampling, accumulation, validation commands,
  one offline screenshot, and any known limitations.

If you implement the denoising bonus, describe it and its validation in the
report.

Do not submit `assets/`. The provided Lucy model and skybox images are supplied
separately and can be selected with `--assets DIR` when running the project.

Place supporting screenshots in the same `report/` directory. The submission
target includes that directory when present.

Do not include build directories, generated OptiX IR, package-manager
checkouts, or machine-specific artifacts in the source submission. From a
configured repository build, use the provided target.

On Windows, run the following command from Developer PowerShell for VS in
Windows Terminal:

```powershell
cmake --build --preset windows-vs-release-vcpkg --target submit_project2
```

On Linux:

```sh
cmake --build --preset linux-gcc-release-vcpkg --target submit_project2
```
