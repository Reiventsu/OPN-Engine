#include <chrono>
#include <exception>
#include <iostream>
#include <thread>

// all my homies hate the Windows terminal
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#define NOMINMAX
#include <windows.h>

void initTerminal() {
    // Enable ANSI for output

    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);

    HANDLE hErr = GetStdHandle(STD_ERROR_HANDLE);
    GetConsoleMode(hErr, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hErr, dwMode);
}
#endif

import opn.System.JobDispatcher;
import opn.System.Services;
import opn.System.EngineServices;
import opn.Utils.Logging;
import opn.Renderer.RenderPackage;
import opn.Application;

int main() {
    opn::logInfo("OPN Engine", "Starting engine...");
#ifdef _WIN32
    initTerminal();
#endif

    try {
        opn::JobDispatcher::init();
        opn::EngineServiceManager::init();
        application->onPreInit();

        opn::EngineServiceManager::registerServices();
        application->onInit();

        opn::EngineServiceManager::postInitAll();
        application->onPostInit();

        auto &time = opn::EngineServiceManager::getService<opn::Time>();
        auto &window = opn::EngineServiceManager::getService<opn::WindowSystem>();

        while (!window.shouldClose()) {
            const auto dt = static_cast<float>(time.getDeltaTime());
            window.pollEvents();

            application->onUpdate(dt);
            opn::EngineServiceManager::updateAll(dt);
        }

        application->onShutdown();
        opn::logInfo("OPN Engine", "Shutting down...");

        opn::EngineServiceManager::shutdown();
        opn::JobDispatcher::shutdown();
        application->onPostShutdown();

        opn::logInfo("OPN Engine", "Shutdown successful.");
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
