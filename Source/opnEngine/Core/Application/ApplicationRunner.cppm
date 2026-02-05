module;
#include <memory>
#include <exception>

export module opn.Application:Runner;

import :App;
import opn.System.JobDispatcher;
import opn.System.EngineServices;
import opn.Utils.Logging;

export namespace opn {
    /**
     * @brief Runs the application lifecycle
     * This is what main.cpp calls to start the engine + your game
     */
    class ApplicationRunner {
    public:
        /**
         * @brief Run an application
         * @param _app Unique pointer to your application instance
         * @return Exit code (0 = success)
         */
        static int run(std::unique_ptr<Application> _app) {
            if (!_app) {
                logCritical("ApplicationRunner", "Application pointer is null!");
                return EXIT_FAILURE;
            }

            logInfo("ApplicationRunner", "Starting application: {}", _app->getName());

            try {
                // ===== PRE-INIT =====
                logInfo("ApplicationRunner", "=== PRE-INIT PHASE ===");
                _app->onPreInit();

                // ===== ENGINE INIT =====
                logInfo("ApplicationRunner", "=== ENGINE INIT PHASE ===");
                JobDispatcher::init();
                EngineServiceManager::init();
                EngineServiceManager::registerServices();

                // ===== APP INIT =====
                logInfo("ApplicationRunner", "=== APP INIT PHASE ===");
                _app->onInit();

                // ===== POST-INIT =====
                logInfo("ApplicationRunner", "=== POST-INIT PHASE ===");
                EngineServiceManager::postInitAll();
                _app->onPostInit();

                // ===== MAIN LOOP =====
                logInfo("ApplicationRunner", "=== ENTERING MAIN LOOP ===");
                runMainLoop(_app.get());

                // ===== PRE-SHUTDOWN =====
                logInfo("ApplicationRunner", "=== PRE-SHUTDOWN PHASE ===");
                _app->onPreShutdown();

                // ===== ENGINE SHUTDOWN =====
                logInfo("ApplicationRunner", "=== ENGINE SHUTDOWN PHASE ===");
                EngineServiceManager::shutdown();
                JobDispatcher::shutdown();

                // ===== APP SHUTDOWN =====
                logInfo("ApplicationRunner", "=== APP SHUTDOWN PHASE ===");
                _app->onShutdown();

                logInfo("ApplicationRunner", "Application exited successfully.");
                return EXIT_SUCCESS;

            } catch (const std::exception& e) {
                logCritical("ApplicationRunner", "Fatal error: {}", e.what());
                return EXIT_FAILURE;
            }
        }

    private:
        static void runMainLoop(Application* _app) {
            auto& time = EngineServiceManager::getService<Time>();
            auto& window = EngineServiceManager::getService<WindowSystem>();

            while (!window.shouldClose()) {
                window.pollEvents();

                // Update time
                const auto deltaTime = static_cast<float>(time.getDeltaTime());
                
                // Update app logic
                _app->onUpdate(deltaTime);

                // Update engine systems
                EngineServiceManager::updateAll(deltaTime);
            }
        }
    };
}