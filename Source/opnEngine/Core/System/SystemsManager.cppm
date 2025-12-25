module;
#include <algorithm>
#include <memory>
#include <ranges>
#include <source_location>
#include <typeindex>
#include <oneapi/tbb/detail/_utils.h>


export module opn.System.SystemsManager;
import opn.System.SystemTypeList;
import opn.Utils.Logging;
import opn.Utils.Exceptions;

export namespace opn {
    template<typename... SystemList>
    class SystemsManagerImpl {
        struct iSystemHolder {
            virtual ~iSystemHolder() = default;

            virtual void shutdown() = 0;
        };

        template<typename T>
        struct SystemHolder : iSystemHolder {
            std::unique_ptr<T> system;

            explicit SystemHolder() {
                opn::logTrace("SystemsManager", "Constructing system holder...");
                system = std::make_unique<T>();
            }

            void init() {
                opn::logTrace("SystemsManager", "Initializing system...");
                system->init();
            }

            void shutdown() override {
                if (system) {
                    opn::logTrace("SystemsManager", "Shutting down system holder...");
                    system->shutdown();
                }
            }

            T &get() { return *system; }
            const T &get() const { return *system; }
        };

    public:
        static void init(const std::source_location _loc = std::source_location::current()) {
            if (initialized.exchange(true, std::memory_order_acq_rel)) {
                throw MultipleInit_Exception("SystemsManager", _loc);
            }
            opn::logInfo("SystemsManager", "SystemsManager initialized successfully. Max Systems: {}",
                         SystemList::template size());
        }

        static void shutdown() {
            if (!initialized.exchange(false, std::memory_order_acq_rel)) {
                opn::logWarning("SystemsManager", "Shutdown called, but SystemsManager wasn't initialized.");
                return;
            }

            opn::logInfo("SystemsManager", "Shutting down {} systems...", systems.size());

            for (auto &itr: std::ranges::reverse_view(initOrder)) {
                auto sysItr = systems.find(itr);
                if (sysItr != systems.end()) {
                    sysItr->second->shutdown();
                }
            }
            systems.clear();
            initOrder.clear();

            opn::logInfo("SystemsManager", "All systems shutdown successfully.");
        }

        template<typename T>
        static T &registerSystem() {
            static_assert(SystemList::template contains<T>(),
                          "ERROR: System not found in SystemList."
                          "You must add this system to your application's system list declaration."
            );

            const auto typeIdx = std::type_index(typeid(T));

            if (systems.contains(typeIdx)) {
                opn::logWarning("SystemsManager", "System {} already registered. Returning existing instance.");
                return getSystemInternal<T>();
            }

            opn::logInfo("SystemsManager", "Registering new system...");

            auto holder = std::make_unique<SystemHolder<T> >();
            holder->init();

            T &systemRef = holder->get();
            systems.emplace(typeIdx, std::move(holder));
            initOrder.emplace_back(typeIdx);

            opn::logInfo("SystemsManager", "System {} registered successfully. Total systems: {}",
                         typeIdx.name(), systems.size());

            return systemRef;
        }

        template<typename T>
        [[nodiscard]] static bool isRegistered() noexcept {
            static_assert(SystemList::template contains<T>(),
                          "System not in SystemList.");

            return systems.contains(typeid(T));
        }

        [[nodiscard]] static size_t getSystemCount() noexcept { return systems.size(); }

        [[nodiscard]] static std::vector<std::type_index> getSystemTypes() noexcept {
            std::vector<std::type_index> result;
            result.reserve(systems.size());

            for (const auto &[type, _]: systems) {
                result.emplace_back(type);
            }
            return result;
        }

        template<typename T>
        [[nodiscard]] static const T &getSystem() {
            static_assert(SystemList::template contains<T>(),
                          "System not in SystemList. "
                          "Cannot access systems not in your declared list."
            );

            auto itr = systems.find(typeid(T));
            if (itr == systems.end()) {
                opn::logCritical("SystemsManager", "System not registered!");
                throw std::runtime_error("System not registered!");
            }

            auto *holder = static_cast<SystemHolder<T> *>(itr->second.get());
            return holder->get();
        }

        template<typename T>
        [[nodiscard]] static std::optional<std::reference_wrapper<const T> > tryGetSystem() noexcept {
            if (!isRegistered<T>()) {
                return std::nullopt;
            }

            try {
                return std::ref(getSystem<T>());
            } catch (...) {
                return std::nullopt;
            }
        }

        static void registerSystems() {
            opn::logInfo("SystemsManager", "Registering systems from declaration list.");

            SystemList::forEach([](auto _typeTag) {
                using systemType = std::decay_t<decltype(_typeTag)>;
                registerSystem<systemType>();
            });

            opn::logInfo("SystemsManager", "All systems registered successfully.");
        }

    private:
        friend class JobDispatcher;

        template<typename T>
        static T &getSystemInternal() {
            static_assert(SystemList::template contains<T>(),
                          "ERROR: System not found in SystemList.");
            auto itr = systems.find(typeid(T));
            if (itr == systems.end()) {
                opn::logCritical("SystemsManager", "System not registered!");
                throw std::runtime_error("System not registered!");
            }

            auto *holder = static_cast<SystemHolder<T> *>(itr->second.get());
            return holder->get();
        }


        inline static std::atomic_bool initialized{false};
        inline static std::unordered_map<std::type_index, std::unique_ptr<iSystemHolder> > systems;
        inline static std::vector<std::type_index> initOrder;
    };
};
