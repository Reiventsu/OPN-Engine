module;
// Use #include directives between here and the
// export module as you would normally.

#include <string>


export module opn.AppSpace.App;
import opn.Application;
import opn.Utils.Logging;
// Use import directives between here and the class


class MyApplication : public opn::iApplication {
protected:
    void onInit() override {
        opn::logInfo("Application", "Hello World!");
    }

    void onPostInit() override {
    }

    void onPreInit() override {
    }

    void onPostShutdown() override {
    }

    void onShutdown() override {
        opn::logInfo("Application", "Goodbye World!");
    }

    void onUpdate(float _deltaTime) override {
    }

public:
    [[nodiscard]] ::std::string getName() const override {
        return "My first OPN app!";
    }
};
