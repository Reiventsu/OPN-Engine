module;
#include <memory>
export module opn.Application:AppLink;
import :iApp;

export extern "C++" namespace opn {
    std::unique_ptr<iApplication> linkApplication();
}
