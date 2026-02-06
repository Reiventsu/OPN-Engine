module;
#include <memory>
export module opn.Application:AppLink;
import :iApp;

export namespace opn {
    std::unique_ptr<iApplication> linkApplication();
}
