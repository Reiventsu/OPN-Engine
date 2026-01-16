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

        virtual void update(float _deltaTime) {
        }
    };

    export template<typename T>
    concept IsService = std::derived_from<T, iService>;

    export template<typename T>
    class Service : public iService {
    public:
        static T *getInstance() { return s_instance; }
        static bool isActive() { return s_instance != nullptr; }

        void init() final {
            s_instance = static_cast<T *>(this);
            onInit();
        }

        void shutdown() final {
            if (s_instance == nullptr) return;
            onShutdown();
            s_instance = nullptr;
        }

        void update(const float _deltaTime) override {
            onUpdate(_deltaTime);
        }

    protected:
        Service() = default;

        virtual void onInit() = 0;

        virtual void onShutdown() = 0;

        virtual void onUpdate(float _deltaTime) {};

    private:
        inline static T *s_instance = nullptr;
    };
}
