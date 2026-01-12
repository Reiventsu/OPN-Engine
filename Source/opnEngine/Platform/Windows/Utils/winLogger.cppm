module;

#include <atomic>
#include <chrono>
#include <format>
#include <iostream>
#include <mutex>
#include <source_location>
#include <string>
#include <string_view>
#include <ctime>

export module opn.Utils.Logging;

// Defines all forms of logging code will automatically
// generate new levels if added to this X macro list.
#define LOG_LEVELS                 \
    X(Trace,    "TRACE", 0, false) \
    X(Debug,    "DEBUG", 1, false) \
    X(Info,     "INFO",  2, false) \
    X(Warning,  "WARN",  3, true)  \
    X(Error,    "ERROR", 4, true)  \
    X(Critical, "CRIT",  5, true)

export namespace opn {
    enum class eLogLevel : int8_t {
#define X(name, str, val, showLoc) name = val,
        LOG_LEVELS
#undef X
    };

    /**
     * @class Logger
     * @brief A utility class for logging messages with various levels of severity.
     *
     * @note When employing a log feature, two strings are expected:
     * the first serves as an identifier or context,
     * and the second contains the actual message.
     */
    class Logger {
    public:
        struct Context {
            std::string_view category;
            std::source_location location;

            Context(const char *_category, const std::source_location _location = std::source_location::current())
                : category(_category), location(_location) {
            }

            Context(const std::string_view _category,
                    const std::source_location _location = std::source_location::current())
                : category(_category), location(_location) {
            }
        };

        static void setLevel(const eLogLevel &_level) noexcept {
            min_level.store(static_cast<int8_t>(_level), std::memory_order_relaxed);
        }

        static eLogLevel getLevel() noexcept {
            return static_cast<eLogLevel>(min_level.load(std::memory_order_relaxed));
        }

        template<typename... Args>
        static void log(eLogLevel _level,
                        std::string_view _category,
                        const std::source_location &_loc,
                        std::format_string<Args...> _fmt,
                        Args &&... _args
            ) {
                if (static_cast<int8_t>(_level) < min_level.load(std::memory_order_relaxed))
                    return;

                // Use vformat to bypass MSVC's strict consteval validation in modules
                std::string message = std::vformat(_fmt.get(), std::make_format_args(_args...));

                const auto now = std::chrono::system_clock::now();
                const auto now_time_t = std::chrono::system_clock::to_time_t(now);

                std::tm tm_val{};
                localtime_s(&tm_val, &now_time_t);

                const int ms_val = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(
                    now.time_since_epoch()).count() % 1000);

                // Store string views/ints in locals to provide stable lvalues for MSVC's make_format_args
                const std::string_view level_str = levelToString(_level);
                const int h = tm_val.tm_hour;
                const int m = tm_val.tm_min;
                const int s = tm_val.tm_sec;

                std::lock_guard lock(log_mutex);

                std::string output = std::vformat("[{:02}:{:02}:{:02}.{:03}] [{}] [{}] {}",
                    std::make_format_args(h, m, s, ms_val, level_str, _category, message));

    #ifndef NDEBUG
                bool shouldShowLoc = false;
                switch (_level) {
    #define X(name, str, val, showLoc) case eLogLevel::name: shouldShowLoc = showLoc; break;
                    LOG_LEVELS
    #undef X
                }

                if (shouldShowLoc) {
                    std::string_view filename = _loc.file_name();
                    if (const auto pos = filename.find_last_of("/\\"); pos != std::string_view::npos) {
                        filename = filename.substr(pos + 1);
                    }
                    const uint32_t line_val = static_cast<uint32_t>(_loc.line());
                    output += std::vformat(" ({}:{})", std::make_format_args(filename, line_val));
                }
    #endif

                // Use .c_str() to bypass MSVC module bugs regarding std::string's operator<<
                std::cerr << output.c_str() << std::endl;
            }

    private:
        inline static std::mutex log_mutex;
        inline static std::atomic_int8_t min_level = {
#ifdef NDEBUG
            static_cast<int8_t>(eLogLevel::Info)
#else
            static_cast<int8_t>(eLogLevel::Debug)
#endif
        };

        static std::string_view levelToString(const eLogLevel _level) noexcept {
            switch (_level) {
#define X(name, str, val, showLoc) case eLogLevel::name: return str;
                LOG_LEVELS
#undef X
                default: return "UNKNOWN";
            }
        }
    };
}

// MACROS
#define OPN_GENERATE_LOG_FUNC(name, level)                        \
    export namespace opn {                                        \
        template<typename... Args>                                \
        inline void log##name(Logger::Context _ctx,               \
                              std::format_string<Args...> _fmt,   \
                              Args&&... _args) {                  \
                              Logger::log(                        \
                                 eLogLevel::level, _ctx.category, \
                                 _ctx.location,                   \
                                 _fmt,                            \
                                 std::forward<Args>(_args)...     \
                              );                                  \
        }                                                         \
    }

#define X(name, str, val, showLoc) OPN_GENERATE_LOG_FUNC(name, name)
LOG_LEVELS
#undef X
