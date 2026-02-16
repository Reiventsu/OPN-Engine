module;
#include <typeindex>
#include <unordered_map>
#include <functional>
#include <string_view>

export module opn.Locator;
import opn.System.ServiceInterface;
import opn.Utils.Logging;

export namespace opn {
    class Locator {
    public:
        template<typename T, typename Func>
        static void useService(Func&& func) noexcept {
            T* service = _internal_getService<T>();

            if (service) {
                func(*service);
            } else {
                opn::logError("Locator", "Access failed: Service [{}] not found or not registered.", typeid(T).name());
            }
        }

        // --- Standard Accessor ---
        template<typename T>
        [[nodiscard]] static T& Services() {
            return *_internal_getService<T>();
        }

        // Internal bridge used by the Manager
        static void _internal_registerService(std::type_index type, iService* service) {
            m_services[type] = service;
        }

    private:
        template<typename T>
        static T* _internal_getService() {
            auto it = m_services.find(typeid(T));
            if (it != m_services.end()) {
                return static_cast<T*>(it->second);
            }
            return nullptr;
        }

        inline static std::unordered_map<std::type_index, iService*> m_services;
    };
}