module;
#include <algorithm>
#include <atomic>
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
    class ServiceManager_Impl{
        struct iServiceHolder {
            virtual ~iServiceHolder() = default;
            virtual void shutdown() = 0;
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

            void shutdown() override {
                if (service) {
                    opn::logTrace("ServiceManager", "Shutting down service holder...");
                    service->shutdown();
                }
            }

            T &get() { return *service; }
            const T &get() const { return *service; }
        };

    public:
        static void init(const std::source_location _loc = std::source_location::current()) {
            if (initialized.exchange(true, std::memory_order_acq_rel)) {
                throw MultipleInit_Exception("ServiceManager", _loc);
            }
            opn::logInfo("ServiceManager", "ServiceManager initialized successfully. Max Services: {}",
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

            opn::logInfo("ServiceManager", "Registering new service...");

            auto holder = std::make_unique<ServiceHolder<T>>();
            holder->init();

            T &serviceRef = holder->get();
            services.emplace(typeIdx, std::move(holder));
            initOrder.emplace_back(typeIdx);

            opn::logInfo("ServiceManager", "Service registered successfully. Total services: {}", services.size());

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
        [[nodiscard]] static std::optional<std::reference_wrapper<const T>> tryGetService() noexcept {
            if (!isRegistered<T>()) {
                return std::nullopt;
            }

            try {
                return std::ref(getService<T>());
            } catch (...) {
                return std::nullopt;
            }
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

            opn::logInfo("ServiceManager", "All services registered successfully.");
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
            inline static std::unordered_map<std::type_index, std::unique_ptr<iServiceHolder>> services;
            inline static std::vector<std::type_index> initOrder;
        };
    } // namespace opn
