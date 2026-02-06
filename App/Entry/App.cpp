module;
#include <memory>
import opn.Engine;
import opn.UserApp;

auto linkApplication() -> std::unique_ptr<opn::iApplication> {
    return std::make_unique<UserApplication>();
}
