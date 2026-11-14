# GPU-projects

[English](README.md) | [简体中文](README.zh-CN.md)

面向研究生的 GPU 计算课程作业与项目。本仓库使用 CMake 构建 CUDA 示例、独立课程项目，以及可复用的 CPU、GPU 和 OpenGL 支持模块。

## 仓库结构

```text
.
├── base/       共享的 CPU、GPU 和图形模块
├── examples/   独立 CUDA 示例
├── projects/   课程作业和较大的项目
└── cmake/      提交包生成辅助文件
```

`base/` 目录不单独定义 CMake 构建目标。每个项目在自己的 `CMakeLists.txt` 中明确列出所用的共享文件，使依赖关系清晰，并让提交包只包含必要的文件。

## 环境要求

- CMake 3.25 或更新版本
- 支持 C++20 的编译器
- CUDA Toolkit，包含 `nvcc` 和 CUDA Runtime 开发文件
- 运行 CUDA 目标时，需要支持 CUDA 的 GPU 和兼容的驱动程序
- 构建使用第三方库的目标时，需要 vcpkg 或等效的预安装依赖包

## 使用 vcpkg 配置依赖

建议将 vcpkg 克隆到仓库根目录：

```sh
git clone https://github.com/microsoft/vcpkg.git vcpkg
```

首次使用时，执行一次 vcpkg 引导脚本：

```sh
# Windows PowerShell
.\vcpkg\bootstrap-vcpkg.bat

# Linux/macOS
./vcpkg/bootstrap-vcpkg.sh
```

然后编辑 `CMakeUserPresets.json.example`，将其中每个 `$env{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake` 替换为 `${sourceDir}/vcpkg/scripts/buildsystems/vcpkg.cmake`，再将结果复制为 `CMakeUserPresets.json`：

```sh
cp CMakeUserPresets.json.example CMakeUserPresets.json
```

之后选择名称以 `*-vcpkg` 结尾的配置和构建预设。CMake 会以清单模式读取仓库中的 `vcpkg.json`，根据选定的目标平台配置（triplet）自动安装声明的依赖包。

## 配置与构建

仓库中的 `CMakePresets.json` 提供适用于 Visual Studio、GCC 和 Clang 的预设，不绑定特定的依赖管理器。完成上述 vcpkg 配置后，使用对应的 `*-vcpkg` 预设。

在 Windows 上，建议直接用 Visual Studio 打开仓库目录，在 CMake 配置选择器中选择 `windows-vs-debug-vcpkg` 或 `windows-vs-release-vcpkg`，然后构建所需目标。

也可以在 Windows Terminal 的 Developer PowerShell for VS 中执行等效命令：

```sh
cmake --preset windows-vs-release-vcpkg
cmake --build --preset windows-vs-release-vcpkg
```

在 Linux 上，直接在终端中使用对应预设：

```sh
# GCC 发布构建
cmake --preset linux-gcc-release-vcpkg
cmake --build --preset linux-gcc-release-vcpkg
```

如需调试构建，将预设名称中的 `release` 替换为 `debug`。

如果依赖已经安装且 CMake 能够找到它们，可以直接使用 `linux-gcc-release` 等不绑定依赖管理器的预设：

```sh
cmake --preset linux-gcc-release
cmake --build --preset linux-gcc-release
```

## 提交打包

`projects/` 下的每个项目都提供 `submit_<project-name>` 构建目标。例如，打包 `project1`：

在 Windows 上，从 Windows Terminal 的 Developer PowerShell for VS 中执行：

```sh
cmake --build --preset windows-vs-release-vcpkg --target submit_project1
```

在 Linux 上执行：

```sh
cmake --build --preset linux-gcc-release-vcpkg --target submit_project1
```

生成的压缩包位于所选预设的构建目录下：`out/build/<preset>/projects/<project-name>/submission/<project-name>.zip`。

## Project 0：CUDA 设备查询

`projects/project0` 使用 CUDA Runtime、Driver API 和 NVML 查询设备信息，报告检测到的每个 CUDA 设备的属性和能力。CUDA Core 总数优先通过 NVML 查询，不支持时根据 SM 数量和架构表计算；存在多个 GPU 时，还会检查设备之间的对等访问能力。

## Project 1：Mandelbrot 渲染器

`projects/project1` 是 Mandelbrot 集渲染器，支持交互式 OpenGL 显示和单次离线 PNG 渲染。它支持高精度中心坐标与缩放值、可配置的分辨率，以及 1×1 或 2×2 采样。启用 CMake 选项 `PROJECT1_OFFLINE_ONLY` 后，可以构建不依赖在线 OpenGL 显示功能的版本。用法、命令行选项、交互操作和离线构建方式详见 [project1 README](projects/project1/README.zh-CN.md)。

## Project 2：OptiX 光线追踪器

`projects/project2` 使用 OptiX 渲染由三个 Lucy 模型、一个地面球体、一组随机分布的球体和天空盒组成的场景，提供渐进式 OpenGL 查看器和离线 PNG 渲染。离线模式可以配置采样数和降噪，查看器也支持切换降噪。CMake 选项 `PROJECT2_OFFLINE_ONLY` 可以移除查看器对 OpenGL 和 GLFW 的依赖。Project 2 需要兼容的 NVIDIA GPU、CUDA 和 OptiX 9.1。用法、命令行选项、交互操作和离线构建方式详见 [project2 README](projects/project2/README.zh-CN.md)。

## Project 3：AlexNet 推理

`projects/project3` 使用 CUDA 执行 AlexNet 批量推理，支持使用合成图像进行性能测试，以及在真实图像上对比 CPU/GPU 预测，可配置 batch size 和 stream 数量。学生使用 Nsight Systems 和 Nsight Compute 分析推理流程，通过 CUDA stream 重叠数据传输与计算，并优化卷积和全连接内核。项目已包含预训练权重和验证图像，获取权重需要 Git LFS。依赖、构建方式和命令行选项详见 [project3 README](projects/project3/README.zh-CN.md)。
