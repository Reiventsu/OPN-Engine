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

        /**
         * @brief Public accessor to the instance of the service. Be mindful of how you use this.
         * @return Gives you a handle in the form of a pointer to the instance of the service.
         */
        static T *getInstance() { return s_instance; }
        static bool isActive() { return s_instance != nullptr; }

        /**
         * @brief Do not use. See onInit instead.
         */
        void init() final {
            s_instance = static_cast<T *>(this);
            onInit();
        }

        /**
         * @brief Do not use. See onShutdown instead.
         */
        void shutdown() final {
            if (s_instance == nullptr) return;
            onShutdown();
            s_instance = nullptr;
        }

        /**
         * @brief Do not use. See onUpdate instead.
         */
        void update(const float _deltaTime) override {
            onUpdate(_deltaTime);
        }

    protected:
        Service() = default;

        /**
         * @brief Create a bootstrap sequence for your service.
         */
        virtual void onInit() = 0;

        /**
         * @brief Create a shutdown sequence for your service.
         */
        virtual void onShutdown() = 0;

        /**
         * @brief Optional. For updates in your service,
         *        will be propagated to by lower level
         *        systems no need to manually call.
         */
        virtual void onUpdate(const float _deltaTime) {
        };

    private:
        inline static T *s_instance = nullptr;
    };
}
