module;
#include <atomic>
#include <type_traits>
#include <concepts>
#include <source_location>
#include <string_view>

/// AS OF CURRENT DEPRECATED UNTIL FURTHER NOTICE ////

export module opn.Utils.Singleton;
import opn.Utils.Exceptions;
import opn.Utils.Logging;

export namespace opn {
    /**
     * @brief Concept to ensure a Singleton class implements required static methods.
     */
    template<typename T>
    concept IsSingletonSystem = requires
    {
        { T::init() } -> std::same_as<void>;
        { T::shutdown() } -> std::same_as<void>;
    };

    template<typename T>
    class iSingleton {
    public:
        static_assert(std::is_base_of_v<iSingleton<T>, T>,
                      "Singleton Error: T must derive from iSingleton<T>");

        static_assert(std::is_constructible_v<T>,
                      "Singleton Error: iSingleton cannot construct T."
                      " Did you forget 'friend class iSingleton<T>; and a private constructor?"
        );

        static_assert(IsSingletonSystem<T>,
                      "Singleton Error: T must implement 'static void init()'"
                      " and 'static void shutdown()'");

        static T &get() {
            static T instance;
            return instance;
        }

        iSingleton(const iSingleton &) = delete;

        iSingleton &operator=(const iSingleton &) = delete;

        iSingleton(iSingleton &&) = delete;

        iSingleton &operator=(iSingleton &&) = delete;

    protected:
        iSingleton() = default;

        virtual ~iSingleton() = 0;

        template<typename Func>
        static void exectuteInit(const std::string_view _systemName, const std::source_location _loc, Func &&_setup) {
            auto &instance = get();
            if (instance.m_initialized.exchange(true)) {
                throw MultipleInit_Exception(_systemName.data(), _loc);
            }
            std::forward<Func>(_setup)(instance);
        }

        template<typename Func>
        static void executeShutdown(std::string_view _systemName, Func &&_cleanup) {
            auto &instance = get();
            if (instance.m_initialized.exchange(false)) {
                std::forward<Func>(_cleanup)(instance);
            } else {
                logWarning("Singleton:", "Execute shutdown called by {}, failed as it wasn't previously initialized.",
                           _systemName);
            }
        }

    private:
        std::atomic_bool m_initialized{false};
    };

    template<typename T>
    iSingleton<T>::~iSingleton() = default;
}
