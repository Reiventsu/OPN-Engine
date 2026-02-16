module;
#include <algorithm>
#include <atomic>
#include <expected>
#include <memory>
#include <ranges>
#include <source_location>
#include <stdexcept>
#include <typeindex>
#include <unordered_map>
#include <vector>

export module opn.System.ServiceManager;
import opn.System.SystemTypeList;
import opn.System.ServiceInterface;
import opn.Utils.Logging;
import opn.Utils.Exceptions;

export namespace opn {
    template<typename ServiceList>
    class ServiceManager_Impl {
        enum class ServiceState {
            Unregistered,
            Registered,
            Initializing,
            Ready,
            ShuttingDown
        };

        enum class ServiceError {
            // The service wasn't found in the internal map
            NotRegistered,

            // The service exists but init() or postInit() hasn't finished
            NotInitialized,

            // The service is currently being destroyed or the manager is closing
            ShuttingDown,

            // A required dependency (another service) is missing or failed
            DependencyMissing,

            // The system is present, but the current hardware doesn't support it
            // (e.g., trying to run a RaytracingService on an old GPU)
            HardwareUnsupported,

            // A catch-all for unexpected logic failures
            Unknown
        };

        struct iServiceHolder {
            virtual ~iServiceHolder() = default;

            virtual void shutdown() = 0;

            virtual void update(float _deltaTime) = 0;

            virtual void postInit() = 0;

            virtual iService* getRaw() = 0;

            ServiceState state = ServiceState::Unregistered;
        };

        template<IsService T>
        struct ServiceHolder : iServiceHolder {
            std::unique_ptr<T> service;

            explicit ServiceHolder() {
                opn::logTrace("ServiceManager", "Constructing service holder...");
                service = std::make_unique<T>();
            }

            void init() {
                opn::logTrace("ServiceManager", "Initializing service...");
                service->init();
            }

            void postInit() override {
                service->postInit();
            }

            void shutdown() override {
                if (service) {
                    opn::logTrace("ServiceManager", "Shutting down service holder...");
                    service->shutdown();
                } else opn::logWarning("ServiceManager", "Service holder already shutdown or was never started.");
            }

            void update(float _deltaTime) override {
                if (service)
                    service->update(_deltaTime);
                else
                    opn::logWarning("ServiceManager", "Update called on service holder but failed.");
            }

            T &get() { return *service; }
            const T &get() const { return *service; }

            iService* getRaw() override { return service.get(); }
        };

    public:
        static void init(const std::source_location _loc = std::source_location::current()) {
            if (initialized.exchange(true, std::memory_order_acq_rel)) {
                throw MultipleInit_Exception("ServiceManager", _loc);
            }
            opn::logInfo("ServiceManager", "ServiceManager initialized successfully. Services: {}",
                         ServiceList::size());
        }

        static void shutdown() {
            if (!initialized.exchange(false, std::memory_order_acq_rel)) {
                opn::logWarning("ServiceManager", "Shutdown called, but ServiceManager wasn't initialized.");
                return;
            }

            opn::logInfo("ServiceManager", "Shutting down {} services...", services.size());

            for (auto &itr: std::ranges::reverse_view(initOrder)) {
                auto servItr = services.find(itr);
                if (servItr != services.end()) {
                    servItr->second->shutdown();
                }
            }
            services.clear();
            initOrder.clear();

            opn::logInfo("ServiceManager", "All services shutdown successfully.");
        }

        static void updateAll(const float _deltaTime) {
            if (!initialized.load(std::memory_order_acquire)) {
                opn::logWarning("ServiceManager", "Update called, but ServiceManager wasn't initialized.");
                return;
            }

            for (const auto &typeIndex: initOrder) {
                auto itr = services.find(typeIndex);
                if (itr != services.end()) {
                    itr->second->update(_deltaTime);
                }
            }
        }

        static void postInitAll() {
            opn::logInfo("ServiceManager", "Post-Initializing services...");
            if (!initialized.load(std::memory_order_acquire)) {
                opn::logWarning("ServiceManager", "PostInit called, but ServiceManager wasn't initialized.");
                return;
            }
            for (const auto &typeIndex: initOrder) {
                auto itr = services.find(typeIndex);
                if (itr != services.end()) {
                    itr->second->postInit();
                    itr->second->state = ServiceState::Ready;
                }
            }

            opn::logInfo("ServiceManager", "Post-Initialization complete.");
        }

        static iService* getRawService(std::type_index _type) {
            auto itr = services.find(_type);
            if (itr != services.end()) {
                return itr->second->getRaw();
            }
            return nullptr;
        }

        static auto getLocatorBridge() {
            return +[](std::type_index _type) -> opn::iService* {
                return getRawService(_type);
            };
        }

        template<IsService T>
        static T &registerService() {
            static_assert(ServiceList::template contains<T>(),
                          "ERROR: Service not found in ServiceList."
                          "You must add this service to your application's service list declaration."
            );

            const auto typeIdx = std::type_index(typeid(T));

            if (services.contains(typeIdx)) {
                opn::logWarning("ServiceManager", "Service already registered. Returning existing instance.");
                return getServiceInternal<T>();
            }

            auto holder = std::make_unique<ServiceHolder<T> >();
            holder->state = ServiceState::Registered;
            holder->init();

            T &serviceRef = holder->get();
            services.emplace(typeIdx, std::move(holder));
            initOrder.emplace_back(typeIdx);

            opn::logInfo("ServiceManager", "Service registered successfully.");

            return serviceRef;
        }

        template<IsService T>
        [[nodiscard]] static bool isRegistered() noexcept {
            static_assert(ServiceList::template contains<T>(),
                          "Service not in ServiceList.");

            return services.contains(typeid(T));
        }

        [[nodiscard]] static size_t getServiceCount() noexcept { return services.size(); }

        [[nodiscard]] static std::vector<std::type_index> getServiceTypes() noexcept {
            std::vector<std::type_index> result;
            result.reserve(services.size());

            for (const auto &[type, _]: services) {
                result.emplace_back(type);
            }
            return result;
        }

        template<IsService T>
        [[nodiscard]] static const T &getService() {
            static_assert(ServiceList::template contains<T>(),
                          "Service not in ServiceList. "
                          "Cannot access services not in your declared list."
            );

            auto itr = services.find(typeid(T));
            if (itr == services.end()) {
                opn::logCritical("ServiceManager", "Service not registered!");
                throw std::runtime_error("Service not registered!");
            }

            auto *holder = static_cast<ServiceHolder<T> *>(itr->second.get());
            return holder->get();
        }

        template<IsService T>
        [[nodiscard]] static std::expected<std::reference_wrapper<const T>, ServiceError> tryGetService() noexcept {
            const auto typeIdx = std::type_index(typeid(T));
            auto itr = services.find(typeIdx);

            if (itr == services.end()) {
                return std::unexpected(ServiceError::NotRegistered);
            }

            const auto &holder = itr->second;

            if (holder->state == ServiceState::ShuttingDown) {
                return std::unexpected(ServiceError::ShuttingDown);
            }

            if (holder->state != ServiceState::Ready) {
                return std::unexpected(ServiceError::NotInitialized);
            }

            auto *concreteHolder = static_cast<ServiceHolder<T> *>(holder.get());
            return std::ref(concreteHolder->get());
        }

        static void registerServices() {
            opn::logInfo("ServiceManager", "Registering services from declaration list.");

            ServiceList::forEach([]<typename T>(std::type_identity<T>) {
                if constexpr (IsService<T>) {
                    registerService<T>();
                } else {
                    static_assert(IsService<T>, "Type in ServiceList does not inherit from iService!");
                }
            });

            opn::logInfo("ServiceManager", "{} services registered successfully.", services.size());
        }

        template<IsService T, typename Func>
        static void useService(Func &&func) noexcept {
            if (auto result = tryGetService<T>(); result.has_value()) {
                // Success: Execute the user logic with the const service reference
                func(result->get());
            } else {
                const ServiceError err = result.error();
                std::string_view typeName = typeid(T).name();

                switch (err) {
                    case ServiceError::NotRegistered:
                        logError("ServiceManager",
                                 "Access failed: Service [{}] was never registered in the ServiceList.", typeName);
                        break;

                    case ServiceError::NotInitialized:
                        logError("ServiceManager",
                                 "Access failed: Service [{}] is registered but not yet 'Ready' (check init/postInit order).",
                                 typeName);
                        break;

                    case ServiceError::ShuttingDown:
                        logError("ServiceManager", "Access denied: Service [{}] is currently shutting down.", typeName);
                        break;

                    case ServiceError::DependencyMissing:
                        logError("ServiceManager", "Access failed: Service [{}] has missing or failed dependencies.",
                                 typeName);
                        break;

                    case ServiceError::HardwareUnsupported:
                        logCritical("ServiceManager",
                                    "Access failed: Hardware does not support requirements for Service [{}].",
                                    typeName);
                        break;

                    case ServiceError::Unknown:
                    default:
                        logError("ServiceManager",
                                 "Access failed: Service [{}] encountered an unhandled or unknown error.", typeName);
                        break;
                }
            }
        }

    private:
        friend class JobDispatcher;

        template<IsService T>
        static T &getServiceInternal() {
            auto itr = services.find(typeid(T));
            auto *holder = static_cast<ServiceHolder<T> *>(itr->second.get());
            return holder->get();
        }

        inline static std::atomic_bool initialized{false};
        inline static std::unordered_map<std::type_index, std::unique_ptr<iServiceHolder> > services;
        inline static std::vector<std::type_index> initOrder;
    };
} // namespace opn
