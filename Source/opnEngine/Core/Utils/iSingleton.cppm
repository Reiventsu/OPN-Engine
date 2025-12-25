module;
#include <type_traits>

export module opn.Utils.Singleton;

export namespace opn {
    template<typename T>
    class iSingleton {
    public:
        static_assert(std::is_base_of_v<iSingleton<T>, T>,
                      "Singleton Error: T must derive from iSingleton<T>");

        static_assert(std::is_constructible_v<T>,
                      "Singleton Error: iSingleton cannot construct T."
                      " Did you forget 'friend class iSingleton<T>; and a private constructor?"
        );

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
    };

    template<typename T>
    iSingleton<T>::~iSingleton() = default;
}
