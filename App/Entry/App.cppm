module;
// Use #include directives between here and the
// export module as you would normally.
#include <string>

// Mandatory
export module opn.UserApp;
import opn.Application;
import opn.Utils.Logging;
// Optional: Use import directives between here and the class
import opn.System.Jobs.Types;
import opn.ECS;

export class UserApplication final : public opn::iApplication {
protected:
    void onPreInit() override {
    }

    void onInit() override {
        opn::logInfo("Application", "Hello World!");
    }

    void onPostInit() override {

        if (auto* ecs = opn::Locator::getService<opn::EntityComponentSystem>()) {
            opn::Locator::submit(opn::eJobType::General, [ecs]() {
                opn::logInfo("App", "Starting mass entity spawn...");

                for (int i = 0; i < 1000; ++i) {
                    const auto e = ecs->createEntity();
                    ecs->addComponent(e, opn::components::Transform{
                        .position = { static_cast<float>(i), 0.0f, 0.0f },
                        .rotation = {},
                        .scale    = {}
                    });
                }

                opn::logInfo("App", "Successfully queued 1000 entities!");
            });
        }
    }

    void onShutdown() override {
        opn::logInfo("Application", "Goodbye World!");
    }

    void onPostShutdown() override {
    }

    void onUpdate(float _deltaTime) override {
    }

public:
    [[nodiscard]] std::string getName() const override {
        return "My first OPN app!";
    }
};
