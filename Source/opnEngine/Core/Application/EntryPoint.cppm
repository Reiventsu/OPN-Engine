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
import opn.Utils.Locator;
import opn.System.Jobs.Dispatcher;
import opn.Application;
import opn.System.EngineServiceList;
import opn.System.Service.Time;
import opn.System.Service.WindowSystem;
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

        const auto &application = opn::linkApplication();
        try {
            EngineServiceManager Services;
            JobDispatcher Jobs;
            application->onPreInit();


            Jobs.init();
            Services.init();

            auto [submit, submitAfter, waitFence, checkFence] = Jobs.getLocatorBridge();
            Locator::registerJobDispatcher(
                std::move(submit),
                std::move(submitAfter),
                std::move(waitFence),
                std::move(checkFence)
            );
            Locator::registerServiceManager(Services.getLocatorBridge());

            Services.registerServices();

            logInfo("OPN Engine", "Engine initialized. Starting application: {}.", application->getName());
            application->onInit();

            Services.postInitAll();
            application->onPostInit();

            const auto* time = Locator::getService<Time>();
            const auto* window = Locator::getService<WindowSystem>();



            while (!window->shouldClose()) {
                const auto dt = static_cast<float>(time->getDeltaTime());
                window->pollEvents();

                Services.updateAll(dt);
                application->onUpdate(dt);
            }

            logInfo("OPN Engine", "Application closing.");
            application->onShutdown();

            logInfo("OPN Engine", "Shutting down...");
            Services.shutdown();
            Jobs.shutdown();

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
