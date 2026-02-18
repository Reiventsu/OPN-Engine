module;
// Use #include directives between here and the
// export module as you would normally.
#include <string>

export module opn.UserApp; // Mandatory
import opn.Application;    // Mandatory
import opn.Utils.Logging;  // Optional
// Use import directives between here and the class
import opn.ECS;
import opn.ECS.Components;
import opn.System.Jobs.Types;

export class UserApplication final : public opn::iApplication {
protected:
    void onPreInit() override {
    }

    void onInit() override {
        opn::logInfo("Application", "Hello World!");
    }

    void onPostInit() override {
        using namespace opn;

        if (auto* ecs = Locator::getService<EntityComponentSystem>()) {
            Locator::submit(eJobType::General, [ecs]() {
                logInfo("App", "Starting mass entity spawn...");

                for (int i = 0; i < 1000; ++i) {
                    auto e = ecs->createEntity();
                    ecs->addComponent(e, components::Transform{
                        .position = { static_cast<float>(i), 0.0f, 0.0f },
                        .rotation = {},
                        .scale    = {}
                    });
                }

                logInfo("App", "Successfully queued 1000 entities!");
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
