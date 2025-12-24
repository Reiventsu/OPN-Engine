module;
#include <stdexcept>
#include <string>
#include <source_location>
#include <format>
#include <string_view>

export module opn.Utils.Exceptions;

export namespace opn {
    class MultipleInit_Exception : public std::runtime_error {
    public:
        explicit MultipleInit_Exception(
            const std::string &systemName,
            const std::source_location loc = std::source_location::current()
        ) : std::runtime_error(std::format(
            "System [{}] was initialized multiple times! (at {}:{})",
            systemName, 
            std::string_view{loc.file_name()}.substr(std::string_view{loc.file_name()}.find_last_of("/\\") + 1),
            loc.line()
        )) {
        }
    };
}
