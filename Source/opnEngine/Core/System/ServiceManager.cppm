module;
#include <algorithm>
#include <atomic>
#include <expected>
#include <functional>
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
    public:
        ServiceManager_Impl() = default;

        ServiceManager_Impl(const ServiceManager_Impl &) = delete;

        ServiceManager_Impl &operator=(const ServiceManager_Impl &) = delete;

        ServiceManager_Impl(ServiceManager_Impl &&) = delete;

        ServiceManager_Impl &operator=(ServiceManager_Impl &&) = delete;

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

    private:
        struct iServiceHolder {
            virtual ~iServiceHolder() = default;

            virtual void shutdown() = 0;

            virtual void update(float _deltaTime) = 0;

            virtual void postInit() = 0;

            virtual iService *getRaw() = 0;

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

            void postInit() override { service->postInit(); }

            void shutdown() override {
                if (service) {
                    opn::logTrace("ServiceManager", "Shutting down service holder...");
                    service->shutdown();
                } else {
                    opn::logWarning("ServiceManager", "Service holder already shutdown or was never started.");
                }
            }

            void update(float _deltaTime) override {
                if (service) service->update(_deltaTime);
                else opn::logWarning("ServiceManager", "Update called on service holder but failed.");
            }

            T &get() { return *service; }
            const T &get() const { return *service; }
            iService *getRaw() override { return service.get(); }
        };

        std::atomic_bool m_initialized{false};
        std::unordered_map<std::type_index, std::unique_ptr<iServiceHolder> > m_services;
        std::vector<std::type_index> m_initOrder;

        template<IsService T>
        T &getServiceInternal() {
            auto itr = m_services.find(typeid(T));
            auto *holder = static_cast<ServiceHolder<T> *>(itr->second.get());
            return holder->get();
        }

    public:

        void init(const std::source_location _loc = std::source_location::current()) {
            if (m_initialized.exchange(true, std::memory_order_acq_rel)) {
                throw MultipleInit_Exception("ServiceManager", _loc);
            }
            opn::logInfo("ServiceManager", "ServiceManager initialized successfully. Services: {}",
                         ServiceList::size());
        }

        void shutdown() {
            if (!m_initialized.exchange(false, std::memory_order_acq_rel)) {
                return; // Already shut down or never initialized
            }

            opn::logInfo("ServiceManager", "Shutting down {} services...", m_services.size());

            for (auto &itr: std::ranges::reverse_view(m_initOrder)) {
                auto servItr = m_services.find(itr);
                if (servItr != m_services.end()) {
                    servItr->second->shutdown();
                }
            }
            m_services.clear();
            m_initOrder.clear();

            opn::logInfo("ServiceManager", "All services shutdown successfully.");
        }

        void updateAll(const float _deltaTime) {
            if (!m_initialized.load(std::memory_order_acquire)) {
                opn::logWarning("ServiceManager", "Update called, but ServiceManager wasn't initialized.");
                return;
            }

            for (const auto &typeIndex: m_initOrder) {
                auto itr = m_services.find(typeIndex);
                if (itr != m_services.end()) {
                    itr->second->update(_deltaTime);
                }
            }
        }

        void postInitAll() {
            opn::logInfo("ServiceManager", "Post-Initializing services...");
            if (!m_initialized.load(std::memory_order_acquire)) {
                opn::logWarning("ServiceManager", "PostInit called, but ServiceManager wasn't initialized.");
                return;
            }
            for (const auto &typeIndex: m_initOrder) {
                auto itr = m_services.find(typeIndex);
                if (itr != m_services.end()) {
                    itr->second->postInit();
                    itr->second->state = ServiceState::Ready;
                }
            }
            opn::logInfo("ServiceManager", "Post-Initialization complete.");
        }

        iService *getRawService(std::type_index _type) {
            auto itr = m_services.find(_type);
            if (itr != m_services.end()) {
                return itr->second->getRaw();
            }
            return nullptr;
        }

        std::function<opn::iService*(std::type_index)> getLocatorBridge() {
            return [this](std::type_index _type) -> opn::iService * {
                return this->getRawService(_type);
            };
        }

        template<IsService T>
        T &registerService() {
            static_assert(ServiceList::template contains<T>(),
                          "ERROR: Service not found in ServiceList.");

            const auto typeIdx = std::type_index(typeid(T));

            if (m_services.contains(typeIdx)) {
                opn::logWarning("ServiceManager", "Service already registered. Returning existing instance.");
                return getServiceInternal<T>();
            }

            auto holder = std::make_unique<ServiceHolder<T> >();
            holder->state = ServiceState::Registered;
            holder->init();

            T &serviceRef = holder->get();
            m_services.emplace(typeIdx, std::move(holder));
            m_initOrder.emplace_back(typeIdx);

            opn::logInfo("ServiceManager", "Service registered successfully.");
            return serviceRef;
        }

        template<IsService T>
        [[nodiscard]] bool isRegistered() const noexcept {
            static_assert(ServiceList::template contains<T>(), "Service not in ServiceList.");
            return m_services.contains(typeid(T));
        }

        [[nodiscard]] size_t getServiceCount() const noexcept { return m_services.size(); }

        [[nodiscard]] std::vector<std::type_index> getServiceTypes() const noexcept {
            std::vector<std::type_index> result;
            result.reserve(m_services.size());
            for (const auto &[type, _]: m_services) {
                result.emplace_back(type);
            }
            return result;
        }

        template<IsService T>
        [[nodiscard]] T &getService() {
            static_assert(ServiceList::template contains<T>(), "Service not in ServiceList.");

            auto itr = m_services.find(typeid(T));
            if (itr == m_services.end()) {
                opn::logCritical("ServiceManager", "Service not registered!");
                throw std::runtime_error("Service not registered!");
            }

            auto *holder = static_cast<ServiceHolder<T> *>(itr->second.get());
            return holder->get();
        }

        template<IsService T>
        [[nodiscard]] std::expected<std::reference_wrapper<T>, ServiceError> tryGetService() noexcept {
            const auto typeIdx = std::type_index(typeid(T));
            auto itr = m_services.find(typeIdx);

            if (itr == m_services.end()) return std::unexpected(ServiceError::NotRegistered);

            const auto &holder = itr->second;
            if (holder->state == ServiceState::ShuttingDown) return std::unexpected(ServiceError::ShuttingDown);
            if (holder->state != ServiceState::Ready) return std::unexpected(ServiceError::NotInitialized);

            auto *concreteHolder = static_cast<ServiceHolder<T> *>(holder.get());
            return std::ref(concreteHolder->get());
        }

        void registerServices() {
            opn::logInfo("ServiceManager", "Registering services from declaration list.");
            ServiceList::forEach([this]<typename T>(std::type_identity<T>) {
                if constexpr (IsService<T>) {
                    this->registerService<T>();
                } else {
                    static_assert(IsService<T>, "Type in ServiceList does not inherit from iService!");
                }
            });
            opn::logInfo("ServiceManager", "{} services registered successfully.", m_services.size());
        }

        template<IsService T, typename Func>
        void useService(Func &&func) noexcept {
            if (auto result = tryGetService<T>(); result.has_value()) {
                func(result->get());
            } else {
                const ServiceError err = result.error();
                std::string_view typeName = typeid(T).name();

                switch (err) {
                    case ServiceError::NotRegistered: logError( "ServiceManager"
                                                              , "Access failed: [{}] not registered."
                                                              , typeName );
                        break;
                    case ServiceError::NotInitialized: logError( "ServiceManager"
                                                               , "Access failed: [{}] not 'Ready'."
                                                               , typeName );
                        break;
                    case ServiceError::ShuttingDown: logError( "ServiceManager"
                                                             , "Access denied: [{}] shutting down."
                                                             , typeName );
                        break;
                    case ServiceError::DependencyMissing: logError( "ServiceManager"
                                                                  , "Access failed: [{}] missing dependencies."
                                                                  , typeName );
                        break;
                    case ServiceError::HardwareUnsupported: logCritical( "ServiceManager"
                                                                       , "Access failed: Hardware unsupported for [{}]."
                                                                       , typeName );
                        break;
                    default: logError( "ServiceManager"
                                     , "Access failed: [{}] unknown error."
                                     , typeName );
                        break;
                }
            }
        }
    };
} // namespace opn
