module;

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

#endif

export module opn.Engine;
import opn.Application;
import opn.System.EngineServices;
import opn.System.Service.Time;
import opn.System.Service.WindowSystem;
import opn.System.JobDispatcher;
import opn.Utils.Logging;

export namespace opn::detail {
#ifdef _WIN32
    void initTerminal() {
        // Enable ANSI for output

        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD dwMode = -1;
        GetConsoleMode(hOut, &dwMode);
        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hOut, dwMode);

        HANDLE hErr = GetStdHandle(STD_ERROR_HANDLE);
        GetConsoleMode(hErr, &dwMode);
        dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hErr, dwMode);
    }
#endif

    int runEngine() {
        opn::logInfo("OPN Engine", "Starting engine...");

#ifdef _WIN32
        initTerminal();
#endif

        const auto &application = linkApplication();

        try {
            application->onPreInit();

            JobDispatcher::init();
            EngineServiceManager::init();
            EngineServiceManager::registerServices();

            logInfo("OPN Engine", "Engine initialized. Starting application: {}.", application->getName());
            application->onInit();

            EngineServiceManager::postInitAll();
            application->onPostInit();

            auto &time = EngineServiceManager::getService<Time>();
            auto &window = EngineServiceManager::getService<WindowSystem>();

            while (!window.shouldClose()) {
                const auto dt = static_cast<float>(time.getDeltaTime());
                window.pollEvents();

                EngineServiceManager::updateAll(dt);
                application->onUpdate(dt);
            }

            logInfo("OPN Engine", "Application closing.");
            application->onShutdown();

            logInfo("OPN Engine", "Shutting down...");
            EngineServiceManager::shutdown();
            JobDispatcher::shutdown();

            logInfo("OPN Engine", "Application post-shutdown cleanup...");
            application->onPostShutdown();

            logInfo("OPN Engine", "Shutdown successful.");
        } catch (const std::exception &e) {
            std::cerr << e.what() << std::endl;
            return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
    }
}
