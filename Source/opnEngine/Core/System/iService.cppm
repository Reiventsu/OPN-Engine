module;
#include <concepts>
#include <typeindex>

export module opn.System.ServiceInterface;

namespace opn {
    /**
     * @brief Base interface for engine services. Required to participate in SystemsManager
     */
    export class iService {
    public:
        virtual ~iService() = default;

        virtual void init() = 0;

        virtual void postInit() {
        }

        virtual void shutdown() = 0;

        virtual void update(float _deltaTime) {
        }
    };

    export template<typename T>
    concept IsService = std::derived_from<T, iService>;

    export template<typename T>
    class Service : public iService {
    public:
        static bool isActive() { return s_instance != nullptr; }

        void init() final {
            s_instance = static_cast<T *>(this);
            onInit();
        }

        void postInit() final {
            onPostInit();
        }

        void shutdown() final {
            onShutdown();
            s_instance = nullptr;
        }

        void update(const float _deltaTime) final {
            onUpdate(_deltaTime);
        }

    protected:
        Service() = default;

        /**
         * @brief Create a bootstrap sequence for your service.
         */
        virtual void onInit() = 0;

        /**
         * @brief OPTIONAL: If you need post-setup steps such as binding
         *        a service to another or just need an extra setup step.
         */
        virtual void onPostInit() {
        }

        /**
         * @brief Create a shutdown sequence for your service.
         */
        virtual void onShutdown() = 0;

        /**
         * @brief OPTIONAL: For updates in your service,
         *        will be propagated to by lower level
         *        systems no need to manually call.
         */
        virtual void onUpdate(const float _deltaTime) {
        };

    private:
        inline static T *s_instance = nullptr;
    };

    export class iServiceRegistry {
        public:
        virtual ~iServiceRegistry() = default;
        virtual iService* getRawService(std::type_index _type) = 0;
    };
}
