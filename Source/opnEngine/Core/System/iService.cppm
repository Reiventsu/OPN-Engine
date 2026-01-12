module;
#include <concepts>

export module opn.System.ServiceInterface;

namespace opn {
    /**
     * @brief Base interface for engine services. Required to participate in SystemsManager
     */
    export class iService {
    public:
        virtual ~iService() = default;

        virtual void init() = 0;

        virtual void shutdown() = 0;
    };

    export template<typename T>
    concept IsService = std::derived_from<T, iService>;

    export template<typename T>
    class Service : public iService {
    public:
        static T *getInstance() { return s_instance; }
        static bool isActive() { return s_instance != nullptr; }

        void init() override final {
            s_instance = static_cast<T *>(this);
            onInit();
        }

        void shutdown() override final {
            if (s_instance == nullptr) return;
            onShutdown();
            s_instance = nullptr;
        }

    protected:
        Service() = default;

        virtual void onInit() = 0;

        virtual void onShutdown() = 0;

    private:
        inline static T *s_instance = nullptr;
    };
}
