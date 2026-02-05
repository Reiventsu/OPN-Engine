module;
#include <string>

export module opn.Application:App;

import opn.Utils.Logging;

export namespace opn {
    /**
     * @brief Base class for all applications built with OPN Engine
     *
     * Inherit from this and override lifecycle methods.
     * The engine manages calling these at the right times.
     *
     * Example:
     * @code
     * class MyGame : public opn::Application {
     * protected:
     *     void onInit() override {
     *         // Create entities, load assets
     *     }
     *     void onUpdate(float dt) override {
     *         // Game logic
     *     }
     * };
     * @endcode
     */
    class Application {
    public:
        virtual ~Application() = default;

        /**
         * @brief Get the application name
         * Override to set your game's name (shown in window title, logs, etc.)
         */
        [[nodiscard]] virtual std::string getName() const {
            return "OPN Application";
        }

    protected:
        /**
         * @brief Called BEFORE engine services are initialized
         */
        virtual void onPreInit() {
            logInfo("Application", "Pre-initialization (override onPreInit for custom behavior)");
        }

        /**
         * @brief Called AFTER engine services are initialized
         */
        virtual void onInit() {
            logInfo("Application", "Initialization (override onInit for custom behavior)");
        }

        /**
         * @brief Called AFTER all services have completed postInit
         */
        virtual void onPostInit() {
            logInfo("Application", "Post-initialization (override onPostInit for custom behavior)");
        }

        /**
         * @brief Called every frame
         */
        virtual void onUpdate(float _deltaTime) {
            // Default: do nothing (override to add game logic)
        }

        /**
         * @brief Called BEFORE engine shutdown begins
         */
        virtual void onPreShutdown() {
            logInfo("Application", "Pre-shutdown (override onPreShutdown for custom behavior)");
        }

        /**
         * @brief Called AFTER engine has shut down
         */
        virtual void onShutdown() {
            logInfo("Application", "Shutdown (override onShutdown for custom behavior)");
        }

        friend class ApplicationRunner;
    };
}
