module;
#include <concepts>

export module opn.System.iService;

export namespace opn {
    /**
     * @brief Base interface for engine services. Required to participate in SystemsManager
     */
    class iService {
        public:
        virtual ~iService() = default;
        virtual void init() = 0;
        virtual void shutdown() = 0;
    };
    template<typename T>
    concept IsService = std::derived_from<T, iService>;
}