module;
#include <concepts>

export module opn.System.SystemTypeList;

export namespace opn {
    template<typename... Types>
    struct SystemTypeList {
        template<typename T>
        static constexpr bool contains() {
            return (std::is_same_v<T, Types> || ...);
        };

        static constexpr std::size_t size() {
            return sizeof...(Types);
        }

        template<typename Func>
        static void forEach(Func &&_func) {
            (_func(std::type_identity<Types>{}), ...);
        }
    };
}
