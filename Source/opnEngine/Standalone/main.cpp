#include <chrono>
#include <exception>
#include <iostream>
#include <thread>

// all my homies hate the Windows terminal
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

void initTerminal() {
#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_ERROR_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
#endif
}

import opn.System.JobDispatcher;
import opn.System.ServiceManager;
import opn.System.SystemTypeList;
import opn.System.Services;
import opn.Utils.Logging;
import opn.Renderer.RenderPackage;

using AppServices = opn::SystemTypeList<
    opn::Time,         // Mandatory service
    opn::WindowSystem, // Mandatory service
    opn::Rendering< opn::VulkanImpl >,
    opn::AssetSystem
>;

using ServiceManager = opn::ServiceManager_Impl< AppServices >;

int main() {
#ifdef _WIN32
    initTerminal();
#endif
    try {
        opn::logInfo( "Main", "Starting application..." );

        opn::JobDispatcher::init();
        ServiceManager::init();

        ServiceManager::registerServices();
        ServiceManager::postInitAll();

        auto &time = ServiceManager::getService<opn::Time>();
        auto &window = ServiceManager::getService<opn::WindowSystem>();

        while (!window.shouldClose()) {
            window.pollEvents();

            const auto dt = static_cast<float>(time.getDeltaTime());
            ServiceManager::updateAll(dt);
        }

        opn::logError("test","error");
        opn::logDebug("test", "debug");
        opn::logWarning("test", "warning");
        opn::logCritical("test", "critical");

        opn::logInfo("Main", "Engine Shutting down.");
        ServiceManager::shutdown();
        opn::JobDispatcher::shutdown();

        opn::logInfo("Main", "Engine shutdown successfully.");
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
