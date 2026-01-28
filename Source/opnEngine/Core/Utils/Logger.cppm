module;

#include <atomic>
#include <chrono>
#include <format>
#include <iostream>
#include <mutex>
#include <source_location>
#include <string_view>

export module opn.Utils.Logging;

// TODO Add colours to logger for different levels

#define OPN_COL_RESET       "\x1b[0m"
#define OPN_COL_TIME        "\x1b[38;5;250m"
#define OPN_COL_CTX         "\x1b[34m"
#define OPN_COL_BRACKET     "\x1b[37m"

#define OPN_COL_PINK        "\x1b[95m"
#define OPN_COL_CYAN        "\x1b[36m"
#define OPN_COL_WHITE       "\x1b[37m"
#define OPN_COL_YELLOW      "\x1b[33m"
#define OPN_COL_ORANGE      "\x1b[38;5;208m"
#define OPN_COL_BOLD_RED    "\x1b[1;31m"

// Defines all forms of logging code will automatically
// generate new levels if added to this X macro list.
#define LOG_LEVELS                                   \
    X(Trace,    "TRACE", 0, false, OPN_COL_PINK)     \
    X(Debug,    "DEBUG", 1, false, OPN_COL_CYAN)     \
    X(Info,     "INFO",  2, false, OPN_COL_WHITE)    \
    X(Warning,  "WARN",  3, true,  OPN_COL_YELLOW)   \
    X(Error,    "ERROR", 4, true,  OPN_COL_ORANGE)   \
    X(Critical, "CRIT",  5, true,  OPN_COL_BOLD_RED)

export namespace opn {
    enum class eLogLevel : int8_t {
#define X(name, str, val, showLoc, col) name = val,
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

            Context(const char* _category, const std::source_location _location = std::source_location::current())
                : category(_category), location(_location) {}
            Context(const std::string_view _category, const std::source_location _location = std::source_location::current())
                : category(_category), location(_location) {}
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

            std::string message = std::format(_fmt, std::forward<Args>(_args)...);

            const auto now = std::chrono::system_clock::now();
            const auto now_seconds = std::chrono::floor<std::chrono::seconds>(now);
            const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - now_seconds).count();

            std::lock_guard lock(log_mutex);
            std::string_view colour = levelToColour(_level);
            std::string_view level = levelToString(_level);
            std::string paddedLevel = std::format("{:^5}", level);
            std::string paddedCtx = std::format("{:<1}", _category);
#ifndef NDEBUG
            bool shouldShowLoc = false;
            switch (_level) {
#define X(name, str, val, showLoc, col) case eLogLevel::name: shouldShowLoc = showLoc; break;
                LOG_LEVELS
#undef X
            }

            if (shouldShowLoc) {
                std::string_view filename = _loc.file_name();
                if (const auto pos = filename.find_last_of("/\\"); pos != std::string_view::npos) {
                    filename = filename.substr(pos + 1);
                }

                std::println( std::cout, "{}[ {:%H:%M:%S}.{:03d} ]{} {}[ {} ]{} {}[{}]:{} {} {}( {}:{} ){}"
                            , OPN_COL_TIME, now_seconds, ms, OPN_COL_RESET
                            , colour, paddedLevel, OPN_COL_RESET
                            , OPN_COL_CTX, paddedCtx, OPN_COL_RESET
                            , message
                            , OPN_COL_RESET , filename, _loc.line(), OPN_COL_RESET
                );
            } else {
                std::println( std::cout, "{}[ {:%H:%M:%S}.{:03d} ]{} {}[ {} ]{} {}[{}]:{} {}"
                            , OPN_COL_TIME, now_seconds, ms, OPN_COL_RESET
                            , colour, paddedLevel, OPN_COL_RESET
                            , OPN_COL_CTX, paddedCtx, OPN_COL_RESET
                            , message
                );
            }
#else
            std::println( std::cout, "{}[ {:%H:%M:%S}.{:03d} ]{} {}[ {} ]{} {}[{}]{}: {}"
                        , OPN_COL_TIME, now_seconds, ms, OPN_COL_RESET
                        , lvlCol, paddedLvl, OPN_COL_RESET
                        , OPN_COL_CTX, paddedCat, OPN_COL_RESET
                        , message
            );
#endif
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

        static std::string_view levelToColour(const eLogLevel _level) noexcept {
            switch (_level) {
#define X(name, str, val, showLoc, col) case eLogLevel::name: return col;
                LOG_LEVELS
#undef X
                default: return OPN_COL_RESET;
            }
        }

        static std::string_view levelToString(const eLogLevel _level) noexcept {
            switch (_level) {
#define X(name, str, val, showLoc, col) case eLogLevel::name: return str;
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

#define X(name, str, val, showLoc, col) OPN_GENERATE_LOG_FUNC(name, name)
LOG_LEVELS
#undef X
