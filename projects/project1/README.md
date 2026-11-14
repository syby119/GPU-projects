# Project 1: Mandelbrot renderer

[English](README.md) | [简体中文](README.zh-CN.md)

`project1` renders the Mandelbrot set with CUDA. It supports an interactive
OpenGL mode and a one-shot offline mode that writes an RGBA PNG to disk. See
the [repository README](../../README.md) for project-wide dependencies and
build instructions. The assignment specification is available in
[ASSIGNMENT.md](ASSIGNMENT.md).

![Mandelbrot set](screenshots/mandelbrot_set.png)

## Dependencies

`project1` requires the CUDA Toolkit and the following vcpkg manifest
dependencies:

- Boost.Multiprecision for high-precision camera coordinates
- STB for writing offline PNG images
- GLAD, GLFW, and GLM for online OpenGL rendering

The offline-only build does not compile or link the online OpenGL renderer, so
it does not require GLAD, GLFW, GLM, or a window-system development stack. It
still requires CUDA, Boost.Multiprecision, and STB.

On Ubuntu or Debian, online OpenGL rendering additionally requires the system
development packages used by GLFW's X11 backend:

```sh
sudo apt-get update
sudo apt-get install -y \
    pkg-config \
    libxinerama-dev \
    libxcursor-dev \
    xorg-dev \
    libglu1-mesa-dev
```

The CMake/vcpkg manifest supplies the C++ libraries; these packages provide
the X11/OpenGL development files needed while GLFW is built.

## Build

When building the whole repository, configure and build `project1` from the
repository root with a vcpkg preset.

On Linux:

```sh
cmake --preset linux-gcc-release-vcpkg
cmake --build --preset linux-gcc-release-vcpkg --target project1
```

On Windows, the recommended workflow is to open the repository root directly
in Visual Studio and select the `windows-vs-release-vcpkg` CMake preset. The
same configuration can be built from Developer PowerShell for VS in Windows
Terminal:

```powershell
cmake --preset windows-vs-release-vcpkg
cmake --build --preset windows-vs-release-vcpkg --target project1
```

Use `windows-vs-debug-vcpkg` for a Debug build.

## Online mode

The default mode creates an OpenGL window and continuously renders the
Mandelbrot set. Running without arguments uses a 640x360 window and 1x1
sampling. The application starts from the default center and zoom used by the
renderer; the camera can then be controlled interactively.

```sh
./project1
```

Useful controls include Space (play/pause), Home/R (restart), End (jump to the
end), Left/Right (seek), Up/Down (change playback speed), A (toggle 1x1/2x2
sampling), and `[`/`]` (change palette shift). Escape exits.

## Offline mode

`--offline` renders one image to disk without creating an OpenGL window. Its
defaults are 1920x1080 and 2x2 sampling. If `--output` is omitted, the output
filename is generated from the requested center and zoom.

```sh
./project1 --offline
```

Command-line options:

| Option | Meaning |
| --- | --- |
| `--offline` | Select one-shot offline rendering; online is the default. |
| `--re <decimal>` / `--center-re <decimal>` | Real part of the image center. |
| `--im <decimal>` / `--center-im <decimal>` | Imaginary part of the image center. |
| `--zoom <decimal>` | Positive zoom value. |
| `--width <pixels>` | Positive image/window width. |
| `--height <pixels>` | Positive image/window height. |
| `--aa <value>` | `0`, `false`, or `1x1` for 1x1; `1`, `true`, or `2x2` for 2x2 sampling. |
| `--output <path>` | Offline output image path. |
| `--help` / `-h` | Print usage information. |

Online mode defaults to 640x360 with 1x1 sampling. Offline mode defaults to
1920x1080 with 2x2 sampling. The center and zoom defaults are shared by both
modes and can be overridden independently.

## Building without OpenGL

For machines or course submissions that do not have OpenGL development
libraries, configure `project1` with `PROJECT1_OFFLINE_ONLY=ON`.

### Top-level build

From the repository root, pass the option while configuring the top-level
project with a vcpkg preset.

On Linux:

```sh
cmake --preset linux-gcc-release-vcpkg -DPROJECT1_OFFLINE_ONLY=ON
cmake --build --preset linux-gcc-release-vcpkg --target project1
```

On Windows, run the following commands from Developer PowerShell for VS in
Windows Terminal:

```powershell
cmake --preset windows-vs-release-vcpkg -DPROJECT1_OFFLINE_ONLY=ON
cmake --build --preset windows-vs-release-vcpkg --target project1
```

### Standalone build

To configure only `project1`, run the commands below from the repository root.
This uses the local `vcpkg/` checkout and the repository-root `vcpkg.json`
described in the repository README.

On Linux:

```sh
cmake -S projects/project1 -B out/build/project1-offline \
    -DPROJECT1_OFFLINE_ONLY=ON \
    -DCMAKE_TOOLCHAIN_FILE="$PWD/vcpkg/scripts/buildsystems/vcpkg.cmake" \
    -DVCPKG_MANIFEST_DIR="$PWD" \
    -DVCPKG_TARGET_TRIPLET=x64-linux \
    -DCMAKE_BUILD_TYPE=Release
cmake --build out/build/project1-offline --target project1 --parallel
```

On Windows, use `x64-windows` and the Release configuration. Run the following
commands from Developer PowerShell for VS in Windows Terminal:

```powershell
cmake -S projects/project1 -B out/build/project1-offline `
    -DPROJECT1_OFFLINE_ONLY=ON `
    "-DCMAKE_TOOLCHAIN_FILE=$PWD/vcpkg/scripts/buildsystems/vcpkg.cmake" `
    "-DVCPKG_MANIFEST_DIR=$PWD" `
    -DVCPKG_TARGET_TRIPLET=x64-windows
cmake --build out/build/project1-offline --target project1 --config Release --parallel
```

This option removes the online renderer and its OpenGL/GLFW/GLM link
dependencies from the target. The resulting executable accepts offline
rendering only, so it must be invoked with `--offline`.

## Submission

Run the submission target from the repository root with a configured vcpkg
build preset.

On Windows, run the following command from Developer PowerShell for VS in
Windows Terminal:

```powershell
cmake --build --preset windows-vs-release-vcpkg --target submit_project1
```

On Linux:

```sh
cmake --build --preset linux-gcc-release-vcpkg --target submit_project1
```

The archive is written to
`out/build/<preset>/projects/project1/submission/project1.zip`.
