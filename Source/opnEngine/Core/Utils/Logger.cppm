module;

#include <atomic>
#include <chrono>
#include <cstdint>
#include <format>
#include <iostream>
#include <source_location>
#include <string_view>

export module opn.Utils.Logging;

export namespace opn
{
    enum class eLogLevel : int8_t
    {
        Trace = 0,
        Debug,
        Info,
        Warn,
        Error,
        Critical,
    };

    class Logger
    {
        inline static std::mutex log_mutex;

        inline static std::atomic_int8_t min_level = {
#ifdef NDEBUG
            static_cast<int8_t>(eLogLevel::Info)
#else
            static_cast<int8_t>(eLogLevel::Debug)
#endif
        };

    public:
        static void setLevel(const eLogLevel& _level) noexcept
        {
            min_level.store(static_cast<int8_t>(_level), std::memory_order_relaxed);
        }

        static eLogLevel getLevel() noexcept
        {
            return static_cast<eLogLevel>(min_level.load(std::memory_order_relaxed));
        }

        template <typename... Args>
        static void log(eLogLevel _level,
                        std::string_view _category,
                        std::format_string<Args...> _fmt,
                        Args&&... _args
#ifndef NDEBUG
                        , const std::source_location& _loc = std::source_location::current()
#endif
        )
        {
            if (static_cast<int8_t>(_level) < min_level.load(std::memory_order_relaxed))
                return;

            std::lock_guard lock(log_mutex);
            std::string message = std::format(_fmt, std::forward<Args>(_args)...);

            const auto now = std::chrono::system_clock::now();
            auto time = std::chrono::system_clock::to_time_t(now);
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()).count() % 1000;

#ifdef NDEBUG
            std::println(std::cerr, "[{:%H:%M:%S}.{%03d}] [{}] [{}] {}",
                         std::chrono::system_clock::to_time_t(time),
                         ms,
                         levelToString(_level),
                         _category,
                         message
            );
#else
            std::string_view filename = _loc.file_name();
            if (const auto pos = filename.find_last_of('/\\'); pos != std::string_view::npos)
            {
                filename = filename.substr(pos + 1);
            }

            std::println(std::cerr, "[{:%H:%M:%S}.{%03d}] [{}] [{}] {} ({}:{})",
                         std::chrono::system_clock::from_time_t(time),
                         ms,
                         levelToString(_level),
                         _category,
                         message,
                         filename,
                         _loc.line()
            );
#endif
        }

        // Convenience
        template <typename... Args>
        static void trace(std::string_view category, std::format_string<Args...> fmt, Args&&... args,
                          const std::source_location& loc = std::source_location::current()
        )
        {
            log(eLogLevel::Trace, category, fmt, std::forward<Args>(args)..., loc);
        }

        template <typename... Args>
        static void debug(std::string_view category, std::format_string<Args...> fmt, Args&&... args,
                          const std::source_location& loc = std::source_location::current()
        )
        {
            log(eLogLevel::Debug, category, fmt, std::forward<Args>(args)..., loc);
        }

    private:
        static std::string_view levelToString(const eLogLevel _level) noexcept
        {
            switch (_level)
            {
            case eLogLevel::Trace: return "TRACE";
            case eLogLevel::Debug: return "DEBUG";
            case eLogLevel::Info: return "INFO";
            case eLogLevel::Warn: return "WARN";
            case eLogLevel::Error: return "ERROR";
            case eLogLevel::Critical: return "CRITICAL";
            default: return "UNKNOWN";
            }
        }
    };

// MACROS
#ifdef NDEBUG
#define OPN_LOG_TRACE(category, fmt, ...) ((void)0)
#define OPN_LOG_DEBUG(category, fmt, ...) ((void)0)
#else
#define OPN_LOG_TRACE(category, ...) Logger::trace(category, __VA_ARGS__)
#define OPN_LOG_DEBUG(category, ...) Logger::debug(category, __VA_ARGS__)
#endif

}
