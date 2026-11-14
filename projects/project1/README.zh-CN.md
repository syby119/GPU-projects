# Project 1：Mandelbrot 渲染器

[English](README.md) | [简体中文](README.zh-CN.md)

`project1` 使用 CUDA 渲染 Mandelbrot 集，支持交互式 OpenGL 模式，以及将一张 RGBA PNG 图像写入磁盘的单次离线模式。仓库级依赖和构建说明见[仓库 README](../../README.zh-CN.md)，作业要求见 [ASSIGNMENT.zh-CN.md](ASSIGNMENT.zh-CN.md)。

![Mandelbrot 集](screenshots/mandelbrot_set.png)

## 依赖

`project1` 需要 CUDA Toolkit，以及 vcpkg 清单中的以下依赖：

- Boost.Multiprecision：用于高精度相机坐标
- STB：用于写入离线 PNG 图像
- GLAD、GLFW 和 GLM：用于在线 OpenGL 渲染

仅离线构建不会编译或链接在线 OpenGL 渲染器，因此不需要 GLAD、GLFW、GLM 或窗口系统开发环境，但仍需要 CUDA、Boost.Multiprecision 和 STB。

在 Ubuntu 或 Debian 上，在线 OpenGL 渲染还需要安装 GLFW 的 X11 后端所用的系统开发包：

```sh
sudo apt-get update
sudo apt-get install -y \
    pkg-config \
    libxinerama-dev \
    libxcursor-dev \
    xorg-dev \
    libglu1-mesa-dev
```

CMake/vcpkg 清单提供 C++ 库；这些系统包则提供构建 GLFW 时所需的 X11/OpenGL 开发文件。

## 构建

构建整个仓库时，从仓库根目录使用 vcpkg 预设配置并构建 `project1`。

在 Linux 上：

```sh
cmake --preset linux-gcc-release-vcpkg
cmake --build --preset linux-gcc-release-vcpkg --target project1
```

在 Windows 上，建议直接用 Visual Studio 打开仓库根目录，选择 `windows-vs-release-vcpkg` CMake 预设。也可以在 Windows Terminal 的 Developer PowerShell for VS 中执行相同配置的构建命令：

```powershell
cmake --preset windows-vs-release-vcpkg
cmake --build --preset windows-vs-release-vcpkg --target project1
```

调试构建使用 `windows-vs-debug-vcpkg`。

## 在线模式

默认模式会创建 OpenGL 窗口并持续渲染 Mandelbrot 集。不带参数运行时，窗口大小为 640×360，使用 1×1 采样。程序从渲染器的默认中心坐标和缩放值开始，随后可以交互控制相机。

```sh
./project1
```

常用操作包括：Space 播放或暂停，Home/R 重新开始，End 跳到结尾，左/右方向键调整播放进度，上/下方向键改变播放速度，A 切换 1×1/2×2 采样，`[`/`]` 调整调色板偏移。按 Escape 退出。

## 离线模式

`--offline` 将一张图像渲染到磁盘，不创建 OpenGL 窗口。默认分辨率为 1920×1080，使用 2×2 采样。省略 `--output` 时，程序会根据指定的中心坐标和缩放值生成输出文件名。

```sh
./project1 --offline
```

命令行选项：

| 选项 | 含义 |
| --- | --- |
| `--offline` | 使用单次离线渲染；默认使用在线模式。 |
| `--re <decimal>` / `--center-re <decimal>` | 图像中心的实部。 |
| `--im <decimal>` / `--center-im <decimal>` | 图像中心的虚部。 |
| `--zoom <decimal>` | 正数缩放值。 |
| `--width <pixels>` | 图像或窗口宽度，必须为正数。 |
| `--height <pixels>` | 图像或窗口高度，必须为正数。 |
| `--aa <value>` | `0`、`false` 或 `1x1` 使用 1×1 采样；`1`、`true` 或 `2x2` 使用 2×2 采样。 |
| `--output <path>` | 离线输出图像的路径。 |
| `--help` / `-h` | 打印使用说明。 |

在线模式默认使用 640×360 分辨率和 1×1 采样；离线模式默认使用 1920×1080 分辨率和 2×2 采样。两种模式共用默认中心坐标和缩放值，这两个参数可以分别覆盖。

## 不依赖 OpenGL 的构建

对于没有 OpenGL 开发库的机器或课程提交环境，可以使用 `PROJECT1_OFFLINE_ONLY=ON` 配置 `project1`。

### 从顶层构建

在仓库根目录使用 vcpkg 预设配置顶层项目时，传入该选项。

在 Linux 上：

```sh
cmake --preset linux-gcc-release-vcpkg -DPROJECT1_OFFLINE_ONLY=ON
cmake --build --preset linux-gcc-release-vcpkg --target project1
```

在 Windows 上，从 Windows Terminal 的 Developer PowerShell for VS 中执行：

```powershell
cmake --preset windows-vs-release-vcpkg -DPROJECT1_OFFLINE_ONLY=ON
cmake --build --preset windows-vs-release-vcpkg --target project1
```

### 单独构建

如需仅配置 `project1`，在仓库根目录执行以下命令。这种方式使用仓库 README 中说明的本地 `vcpkg/` 目录和仓库根目录的 `vcpkg.json`。

在 Linux 上：

```sh
cmake -S projects/project1 -B out/build/project1-offline \
    -DPROJECT1_OFFLINE_ONLY=ON \
    -DCMAKE_TOOLCHAIN_FILE="$PWD/vcpkg/scripts/buildsystems/vcpkg.cmake" \
    -DVCPKG_MANIFEST_DIR="$PWD" \
    -DVCPKG_TARGET_TRIPLET=x64-linux \
    -DCMAKE_BUILD_TYPE=Release
cmake --build out/build/project1-offline --target project1 --parallel
```

在 Windows 上，使用 `x64-windows` 和 Release 配置。从 Windows Terminal 的 Developer PowerShell for VS 中执行：

```powershell
cmake -S projects/project1 -B out/build/project1-offline `
    -DPROJECT1_OFFLINE_ONLY=ON `
    "-DCMAKE_TOOLCHAIN_FILE=$PWD/vcpkg/scripts/buildsystems/vcpkg.cmake" `
    "-DVCPKG_MANIFEST_DIR=$PWD" `
    -DVCPKG_TARGET_TRIPLET=x64-windows
cmake --build out/build/project1-offline --target project1 --config Release --parallel
```

该选项从构建目标中移除在线渲染器及其 OpenGL、GLFW、GLM 链接依赖。生成的可执行程序仅支持离线渲染，因此运行时必须传入 `--offline`。

## 提交打包

在仓库根目录使用已配置的 vcpkg 构建预设运行提交目标。

在 Windows 上，从 Windows Terminal 的 Developer PowerShell for VS 中执行：

```powershell
cmake --build --preset windows-vs-release-vcpkg --target submit_project1
```

在 Linux 上：

```sh
cmake --build --preset linux-gcc-release-vcpkg --target submit_project1
```

压缩包输出到 `out/build/<preset>/projects/project1/submission/project1.zip`。
