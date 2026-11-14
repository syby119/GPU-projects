# Project 2 作业：OptiX 光线追踪器

[English](ASSIGNMENT.md) | [简体中文](ASSIGNMENT.zh-CN.md)

## 1. 概述

构建一个 OptiX 光线追踪器。完成后的 `project2` 程序必须渲染提供的场景，其中包含三个 Lucy 模型、一个地面球体、一组较小球体和天空盒。离线模式必须按照指定的每像素采样数渲染并输出 PNG。启用 OpenGL 的构建还应提供交互式查看器，逐步改善图像质量。

## 2. 学习目标

本作业旨在练习：

- 使用 OptiX 为三角形和内置球体图元构建加速结构；
- 对共享几何进行实例化，并组织实例加速结构（IAS）；
- 创建 OptiX 模块、程序组、管线和着色器绑定表（SBT）记录；
- 追踪不同材质上的多次反弹，并累积蒙特卡洛样本；
- 使用立方体贴图作为环境光照，并进行正确的色彩空间转换。

## 3. 项目范围与共享代码

场景专用代码和资源应放在 `projects/project2` 下。可以扩展共享的 `base/` 目录，为可复用的 OptiX、CUDA 和图形功能添加封装，但必须保证其他项目仍然能够构建。保留仓库的 CMake 预设、`project2` 可执行目标和 `submit_project2` 源码打包目标。

提供的 `assets/models/lucy.obj` 和 `assets/skyboxrt/` 图像属于场景的一部分。
默认资源查找必须独立于进程的工作目录，支持通过 `--assets DIR` 指定外部资源目录；
显式指定的相对路径以当前工作目录为基准解析。

## 4. 必做内容

### 4.1 构建场景加速结构

将 Lucy OBJ 加载为三角形，并构建一个三角形几何加速结构（GAS）。三个 Lucy 实例共用该 GAS，各自使用独立的静态变换。使用 OptiX 内置球体图元，为地面和较小球体构建另一个 GAS。将三个 Lucy 实例与球体实例组合到实例加速结构（IAS）中。

为三角形和球体使用独立的命中程序与 SBT 记录。将表面法线和材质关联到正确的几何体。合理放置相机和模型，使三个 Lucy 实例、地面、球群和天空盒均可见。为场景分配 Lambertian 漫反射、金属和电介质材质。球体布局使用固定随机种子，便于比较多次运行的输出。

完成 `src/optix_programs.cu` 中 `__closesthit__triangle` 和 `__closesthit__sphere` 的 TODO。通过 SBT 读取命中记录，利用重心坐标插值三角形法线，根据命中位置和球心计算球体法线。将每次命中的法线、材质和表面标识存入光线载荷（payload）。

### 4.2 路径追踪与着色

将设备程序编译为 OptiX IR，并构建包含 raygen、miss 和 closest-hit 程序组的管线。raygen 程序必须追踪最多 16 次反弹的路径，不得在主机端递归发起追踪。实现 Lambertian 漫反射、模糊金属和电介质散射。分别维护辐亮度与路径吞吐量，使每次反弹贡献正确的光照。

对于未命中场景的光线，通过立方体贴图采样六张天空盒图像。用于照明前，将其中的 sRGB JPEG 数值转换为线性光照值；显示和输出 PNG 前，将最终 RGB 数值转换为 sRGB。使用浮点采样，并以浮点格式保存累积图像。

完成 `src/optix_programs.cu` 中标记的 raygen TODO：路径离开场景时采样天空盒，根据命中材质选择散射方向。将路径吞吐量乘到天空光照贡献上。

### 4.3 累积样本

对连续样本分别执行 OptiX 启动，然后维护原始线性辐亮度的运行平均值。离线渲染器必须使用指定的采样数。构建查看器时，应随时间持续增加样本；相机移动或帧缓冲区尺寸变化时，清空平均值。

完成 `OptixRenderer::render` 中的 OptiX 启动 TODO：通过启动参数传入相机和采样状态，执行指定数量的采样，并正确推进样本索引和帧索引。完成 `OptixRenderer::resize` 的 TODO，为指定尺寸分配逐像素渲染资源。

### 4.4 补全 OpenGL 构建中的查看器

启用 OpenGL 支持时，通过已向 CUDA 注册的 OpenGL 缓冲区将 OptiX 结果显示在窗口中。CUDA 只能在缓冲区处于映射状态时持有它；解除映射后，OpenGL 才可以读取。帧缓冲区尺寸变化时，重新创建输出存储，调整渲染器资源大小，并重置累积。

使用带有位置和四元数旋转的相机，并将相机控制与相机状态分离。支持 WASD 和 Q/E 移动、鼠标滚轮移动，以及按 Escape 关闭查看器。相机运动必须重置渐进式累积。

### 4.5 补全离线渲染

支持 `--offline`，且不创建 OpenGL 上下文。从命令行接收图像宽度、高度、正数采样数和 PNG 输出路径。默认分辨率为 1920×1080，每像素采样 64 次。未提供输出路径时，生成包含分辨率和采样数的文件名。打印离线渲染耗时。

生成的 PNG 必须具有指定尺寸，并显示提供的场景。构建查看器时，查看器必须显示同一场景。除非用户提供绝对路径，否则输出路径应相对于当前工作目录解析。

### 4.6 构建并部署运行时资源

将从 `src/optix_programs.cu` 构建 OptiX IR 纳入 CMake 构建过程。IR 放在可执行程序旁的 `assets/optix/` 下，Lucy OBJ 和天空盒图像也随程序部署。能够查询时，使用第一个可见 GPU 的计算能力确定 IR 目标架构；无法查询时回退到 `compute_75`。

源码中缺少场景资源时跳过模型和图像复制，仍正常构建并部署 IR。
支持通过 `PROJECT2_OPTIX_IR` 指定生成的 IR 输出文件，默认路径为项目构建目录下的
`assets/optix/project2.optixir`。`--assets` 统一指定模型、天空盒和 IR 的资源根目录。

支持 `PROJECT2_OFFLINE_ONLY=ON`，使构建不需要 GLAD、GLFW 或窗口系统开发环境。`submit_project2` 目标打包项目源码和共享的 `base/` 文件，不包含提供的场景资源。

## 5. 加分项：降噪

可以选择添加 OptiX 降噪器，使用累积图像和表面引导数据进行降噪。在 OpenGL 构建中，如果相邻帧可以安全复用运动数据和历史数据，也可以实现时域降噪。降噪输出与原始样本平均值应分别保存。这部分对应 `OptixRenderer::render` 中的降噪 TODO；如果实现，需要在尺寸变化时调整引导和历史缓冲区大小。

如果实现了降噪，离线模式应支持 `--denoiser on|off|1|0`；构建查看器时，应允许使用 Space 键切换降噪。未提供输出路径时，文件名可以区分 `raw` 与 `denoised`。降噪及其控制不属于基础作业的必做要求。

## 6. 功能要求

正确的实现必须满足以下全部要求：

1. 使用文档中的 Windows CMake 预设，能够完成 Debug 和 Release 构建。
2. 在 OpenGL 构建中，查看器显示三个 Lucy 模型、球群、地面和天空盒；相机移动或窗口尺寸变化时重置样本平均值。
3. 离线模式按照指定的分辨率和路径写入有效 PNG。
4. 增加离线采样数能够降低原始输出中的蒙特卡洛噪声。
5. 通过路径启动可执行程序时，包括从其他工作目录启动时，都能够从程序旁的 `assets/` 目录加载资源和 OptiX IR。
   `--assets DIR` 在在线和离线模式中覆盖模型、天空盒和 IR 的资源根目录。
6. 仅离线构建可以在不初始化 OpenGL 或 GLFW 的情况下完成渲染。
7. `submit_project2` 生成不含 `assets/` 的源码压缩包。

## 7. 验证建议

先进行小尺寸离线渲染，验证路径和资源加载：

```text
project2 --offline --width 320 --height 180 --samples 1 --output sample-1.png
project2 --offline --width 320 --height 180 --samples 64 --output sample-64.png
```

检查两张图像是否显示同一场景，并比较噪声的降低情况。在 OpenGL 构建中，移动查看器相机并调整窗口大小，确认累积被重置。如果实现了降噪加分项且默认启用，比较采样数时应使用 `--denoiser off`。验证加分项时，在相同采样数下比较原始输出和降噪输出。在 OpenGL 构建中，还应检查查看器的降噪切换和时域稳定性。

## 8. 提交内容

需要提交：

- `projects/project2` 下补全后的源代码，以及该项目所需的可复用共享代码修改；
- 位于 `projects/project2/report` 目录下的简短报告 `REPORT.md`，即项目内的 `report/REPORT.md`，说明 GAS/IAS 与 SBT 布局、材质采样、样本累积、验证命令，包含一张离线截图，并说明已知限制。

如果实现了降噪加分项，应在报告中说明其实现和验证方式。

不要提交 `assets/`。提供的 Lucy 模型和天空盒图像会单独提供，运行时可通过 `--assets DIR` 指定。

将辅助截图放在同一个 `report/` 目录中。该目录存在时，提交目标会将其包含在压缩包中。

源码提交中不要包含构建目录、生成的 OptiX IR、包管理器的检出目录或与本机环境有关的产物。完成仓库构建配置后，使用提供的提交目标。

在 Windows 上，从 Windows Terminal 的 Developer PowerShell for VS 中执行：

```powershell
cmake --build --preset windows-vs-release-vcpkg --target submit_project2
```

在 Linux 上：

```sh
cmake --build --preset linux-gcc-release-vcpkg --target submit_project2
```
