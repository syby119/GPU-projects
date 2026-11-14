#include <optix_offline_render.h>

#ifndef PROJECT2_OFFLINE_ONLY
#include <optix_viewer.h>
#endif

#include <charconv>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>

struct CommandLineOptions {
    bool offline{false};
    uint32_t width{1280};
    uint32_t height{720};
    uint32_t samples{64};
    bool denoiser{true};
    std::filesystem::path outputPath;
    std::filesystem::path assetRootDir;
};

static uint32_t parsePositiveUint32(std::string_view value, std::string_view option) {
    uint32_t result = 0;
    auto const [end, error] = std::from_chars(value.data(), value.data() + value.size(), result);
    if (error != std::errc{} || end != value.data() + value.size() || result == 0) {
        throw std::invalid_argument(std::string(option) + " must be a positive integer");
    }
    return result;
}

static CommandLineOptions parseCommandLine(int argc, char* argv[]) {
    CommandLineOptions options{.assetRootDir = std::filesystem::absolute(argv[0]).parent_path() / "assets"};
    for (int index = 1; index < argc; ++index) {
        if (std::string_view(argv[index]) == "--offline") {
            options.offline = true;
        }
    }
    if (options.offline) {
        options.width = 1920;
        options.height = 1080;
    }

    for (int index = 1; index < argc;) {
        std::string_view const option(argv[index++]);
        if (option == "--offline") {
            continue;
        }

        if (index >= argc) {
            throw std::invalid_argument("Option is missing its value: " + std::string(option));
        }

        std::string_view const value(argv[index++]);
        if (option == "--width") {
            options.width = parsePositiveUint32(value, option);
        }
        else if (option == "--height") {
            options.height = parsePositiveUint32(value, option);
        }
        else if (option == "--samples") {
            options.samples = parsePositiveUint32(value, option);
        }
        else if (option == "--denoiser") {
            if (value != "on" && value != "off" && value != "1" && value != "0") {
                throw std::invalid_argument("--denoiser must be on, off, 1, or 0");
            }
            options.denoiser = value == "on" || value == "1";
        }
        else if (option == "--assets") {
            if (value.empty()) {
                throw std::invalid_argument("--assets must not be empty");
            }
            options.assetRootDir = value;
        }
        else if (option == "--output") {
            if (value.empty()) {
                throw std::invalid_argument("--output must not be empty");
            }
            options.outputPath = value;
        }
        else {
            throw std::invalid_argument("Unknown option: " + std::string(option));
        }
    }
    if (!options.offline && !options.outputPath.empty()) {
        throw std::invalid_argument("--output requires --offline");
    }
    if (options.offline && (options.width > static_cast<uint32_t>(std::numeric_limits<int>::max() / 4) ||
                            options.height > static_cast<uint32_t>(std::numeric_limits<int>::max()))) {
        throw std::invalid_argument("Offline image dimensions exceed PNG limits");
    }
    if (options.offline && options.outputPath.empty()) {
        options.outputPath = "output_" + std::to_string(options.width) + "x" + std::to_string(options.height) + "_" +
                             std::to_string(options.samples) + "s_" + (options.denoiser ? "denoised" : "raw") + ".png";
    }
    return options;
}

static void printUsage() {
#ifdef PROJECT2_OFFLINE_ONLY
    std::cout << "Usage: project2 --offline [options]\n"
              << "This build only supports offline rendering.\n\n";
#else
    std::cout << "Usage: project2 [options]\n"
              << "       project2 --offline [options]\n\n"
              << "Default: display an OptiX image in an OpenGL window.\n"
              << "--offline: write one PNG without opening a window.\n\n";
#endif
    std::cout << "Options:\n"
              << "  --assets <path>     Asset directory (default: assets/ beside the executable)\n"
              << "  --width <pixels>    Image/window width\n"
              << "  --height <pixels>   Image/window height\n"
              << "  --output <path>     Offline PNG path (default pattern):\n"
              << "                       output_<width>x<height>_<samples>s_<denoised|raw>.png\n"
              << "  --samples <count>   Offline samples per pixel (default: 64)\n"
              << "  --denoiser <on|off|1|0>  Offline denoising (default: on; Space toggles in viewer)\n"
              << "  --help, -h         Show this help\n\n"
              << "Window defaults: 1280x720; offline defaults: 1920x1080.\n";
}

int main(int argc, char* argv[]) {
    try {
        for (int index = 1; index < argc; ++index) {
            if (auto const arg = std::string_view(argv[index]); arg == "--help" || arg == "-h") {
                printUsage();
                return EXIT_SUCCESS;
            }
        }

        CommandLineOptions const options = parseCommandLine(argc, argv);
#ifdef PROJECT2_OFFLINE_ONLY
        if (!options.offline) {
            throw std::invalid_argument("This build only supports offline rendering; pass --offline");
        }
#endif

        if (options.offline) {
            renderOptixOffline({
                .width = options.width,
                .height = options.height,
                .samples = options.samples,
                .denoiser = options.denoiser,
                .outputPath = options.outputPath,
                .assetRootDir = options.assetRootDir,
            });
        }
#ifndef PROJECT2_OFFLINE_ONLY
        else {
            OptixViewer viewer({
                .windowConfig{
                    .title = "OptiX Ray Tracer",
                    .width = options.width,
                    .height = options.height,
                    .vsync = true,
                    .resizable = true,
                    .fullscreen = false,
                    .maximize = false,
                    .msaa = false,
                    .driverVersion = {4, 5},
                },
                .assetRootDir = options.assetRootDir,
            });
            viewer.run();
        }
#endif
    }
    catch (std::exception const& error) {
        std::cerr << "project2: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
    catch (...) {
        std::cerr << "project2: unknown error\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
