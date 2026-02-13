module;
// Use #include directives between here and the
// export module as you would normally.
#include <string>

export module opn.UserApp;
// Use import directives between here and the class
import opn.Application;
import opn.Utils.Logging;


export class UserApplication final : public opn::iApplication {
protected:
    void onPreInit() override {
    }

    void onInit() override {
        opn::logInfo("Application", "Hello World!");
    }

    void onPostInit() override {
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
