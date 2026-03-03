#include "application/ClientApplication.hpp"
#include "minecraft-reborn/version.h"

#include <spdlog/spdlog.h>
#include <iostream>
#include <string>

namespace {

void printBanner()
{
    std::cout << R"(
  __  __      _____           _____ ______
 |  \/  | /\  / ____|    /\   |  __ \___  /
 | \  /  /  \ | |       /  \  | |__) | / /
 | |\/| / /\ \| |      / /\ \ |  ___/ / /
 | |  |/ ____ \ |____ / ____ \| |   / /__
 |_|  _/_/    \_\_____/_/    \_\_|  /_____/
    | |         | |
    | |__   __ _| | _____ _ __
    | '_ \ / _` | |/ / _ \ '__|
    | |_) | (_| |   <  __/ |
    |_.__/ \__,_|_|\_\___|_|

)" << std::endl;

    std::cout << "  Minecraft Reborn Client v"
              << MR_VERSION_MAJOR << "."
              << MR_VERSION_MINOR << "."
              << MR_VERSION_PATCH << std::endl;
    std::cout << "  ========================================\n" << std::endl;
}

void printHelp()
{
    std::cout << "Usage: minecraft-client [options]\n\n"
              << "Options:\n"
              << "  -h, --help          Show this help message\n"
              << "  -w, --width <px>    Set window width (default: 1280)\n"
              << "  -H, --height <px>   Set window height (default: 720)\n"
              << "  -f, --fullscreen    Start in fullscreen mode\n"
              << "  -s, --server <addr> Server address to connect to\n"
              << "  -p, --port <port>   Server port (default: 19132)\n"
              << "  -u, --username <n>  Player username\n"
              << "  -v, --verbose       Enable verbose logging\n"
              << std::endl;
}

} // namespace

int main(int argc, char* argv[])
{
    // 打印Banner
    printBanner();

    // 默认配置
    mr::client::ClientConfig config;

    // 解析命令行参数
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg == "-h" || arg == "--help") {
            printHelp();
            return 0;
        }
        if ((arg == "-w" || arg == "--width") && i + 1 < argc) {
            config.windowWidth = std::stoi(argv[++i]);
        }
        if ((arg == "-H" || arg == "--height") && i + 1 < argc) {
            config.windowHeight = std::stoi(argv[++i]);
        }
        if (arg == "-f" || arg == "--fullscreen") {
            config.fullscreen = true;
        }
        if ((arg == "-s" || arg == "--server") && i + 1 < argc) {
            config.serverAddress = argv[++i];
        }
        if ((arg == "-p" || arg == "--port") && i + 1 < argc) {
            config.serverPort = static_cast<mr::u16>(std::stoi(argv[++i]));
        }
        if ((arg == "-u" || arg == "--username") && i + 1 < argc) {
            config.username = argv[++i];
        }
        if (arg == "-v" || arg == "--verbose") {
            config.logLevel = "debug";
        }
    }

    try {
        // 创建客户端实例
        mr::client::ClientApplication client;

        // 初始化
        auto initResult = client.initialize(config);
        if (initResult.failed()) {
            spdlog::error("Failed to initialize client: {}", initResult.error().toString());
            return 1;
        }

        // 运行主循环
        auto runResult = client.run();
        if (runResult.failed()) {
            spdlog::error("Client error: {}", runResult.error().toString());
            return 1;
        }

        spdlog::info("Client exited successfully");
        return 0;

    } catch (const std::exception& e) {
        spdlog::critical("Fatal error: {}", e.what());
        return 1;
    }
}
