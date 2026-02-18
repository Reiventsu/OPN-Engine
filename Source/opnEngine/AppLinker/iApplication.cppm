module;
#include <string>
export module opn.Application:iApp;
import opn.Utils.Logging;

export namespace opn {
    class iApplication {
    public:
        virtual ~iApplication() = default;

        /**
         * @brief Get the application name
         * Override to set your game's name (shown in window title, logs, etc.)
         */
        [[nodiscard]] virtual std::string getName() const = 0;

        /**
         * @brief Called BEFORE engine services are initialized
         */
        virtual void onPreInit() {};

        /**
         * @brief Called AFTER engine services are initialized
         */
        virtual void onInit() = 0;

        /**
         * @brief Called AFTER all services have completed postInit
         */
        virtual void onPostInit() {};

        /**
         * @brief Called every frame
         */
        virtual void onUpdate(float _deltaTime) {};

        /**
         * @brief Called BEFORE engine shutdown begins
         */
        virtual void onShutdown() = 0;

        /**
         * @brief Called AFTER engine has shut down
         */
        virtual void onPostShutdown() {};

    };
}
