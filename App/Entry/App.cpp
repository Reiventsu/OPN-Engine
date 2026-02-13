module;
#include <memory>
module opn.UserApp;

extern "C++" namespace opn {
    std::unique_ptr<iApplication> linkApplication() {
        return std::make_unique<UserApplication>();
    }
}