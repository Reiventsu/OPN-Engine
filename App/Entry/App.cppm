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

export class UserApplication final : public opn::iApplication {
protected:
    void onPreInit() override {
    }

    void onInit() override {
        opn::logInfo("Application", "Hello World!");
    }

    void onPostInit() override {

        using namespace opn;

        Locator::useService<EntityComponentSystem>([](const auto& _ecs) {
            // 2. Dispatch the spawning job to a worker thread
            Dispatch::execute([&_ecs]() {
                logInfo("App", "Starting mass entity spawn...");

                // Spawn 1000 entities with transforms
                for (int i = 0; i < 1000; ++i) {
                    // Get an immediate handle (Atomic)
                    auto e = _ecs.createEntity();

                    // Queue component additions (Deferred to Playback)
                    _ecs.addComponent(e, components::Transform{
                        .position = { i, 0.0f, 0.0f},
                        .rotation = {},
                        .scale = {}
                    });
                }

                logInfo("App", "Successfully queued 1000 entities!");
            });
        });
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
