module;
#include <cstdint>
#include <typeindex>
#include <functional>
export module opn.Utils.Locator;
import opn.System.Jobs.Types;
import opn.System.Jobs.Handle;
import opn.System.ServiceInterface;

namespace opn::Locator::detail {
    using ServiceFn     = std::function<iService*(std::type_index)>;
    using SubmitFn      = std::function<sJobHandle(eJobType, std::move_only_function<void()>)>;
    using SubmitAfterFn = std::function<sJobHandle(uint32_t, eJobType, std::move_only_function<void()>)>;
    using WaitFenceFn   = std::function<void(uint32_t)>;
    using CheckFenceFn  = std::function<bool(uint32_t)>;

    inline ServiceFn     s_serviceFn     = nullptr;
    inline SubmitFn      s_submitFn      = nullptr;
    inline SubmitAfterFn s_submitAfterFn = nullptr;
    inline WaitFenceFn   s_waitFenceFn   = nullptr;
    inline CheckFenceFn  s_checkFenceFn  = nullptr;
}

export namespace opn::Locator::registration {
    void registerServiceManager(detail::ServiceFn _fn) {
        detail::s_serviceFn = std::move(_fn);
    }

    void registerJobDispatcher(detail::SubmitFn _submit, detail::SubmitAfterFn _submitAfter,
                               detail::WaitFenceFn _wait, detail::CheckFenceFn _check) {
        detail::s_submitFn      = std::move(_submit);
        detail::s_submitAfterFn = std::move(_submitAfter);
        detail::s_waitFenceFn   = std::move(_wait);
        detail::s_checkFenceFn  = std::move(_check);
    }
}

export namespace opn::Locator {
    template<typename T>
    T* getService() {
        if (!detail::s_serviceFn) return nullptr;
        return static_cast<T*>(detail::s_serviceFn(std::type_index(typeid(T))));
    }

    sJobHandle submit(eJobType _type, std::move_only_function<void()> _fn) {
        return detail::s_submitFn(_type, std::move(_fn));
    }

    sJobHandle submitAfter(uint32_t _fence, eJobType _type, std::move_only_function<void()> fn) {
        return detail::s_submitAfterFn(_fence, _type, std::move(fn));
    }

    void waitFence(uint32_t _fenceID) {
        detail::s_waitFenceFn(_fenceID);
    }

    bool checkFence(uint32_t _fenceID) {
        return detail::s_checkFenceFn(_fenceID);
    }
}