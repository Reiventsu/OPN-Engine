module opn.UserApp;
#include <memory>

auto linkApplication() -> std::unique_ptr<opn::iApplication> {
    return std::make_unique<UserApplication>();
}
