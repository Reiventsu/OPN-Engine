module;
#include <cstdint>
#include <typeindex>
#include <functional>
export module opn.Utils.Locator;
import opn.System.Jobs.Types;
import opn.System.Jobs.Handle;
import opn.System.ServiceInterface;

export namespace opn::Locator {
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

    void registerServiceManager(ServiceFn fn) {
        s_serviceFn = std::move(fn);
    }

    void registerJobDispatcher(SubmitFn submit, SubmitAfterFn submitAfter,
                                WaitFenceFn wait, CheckFenceFn check) {
        s_submitFn      = std::move(submit);
        s_submitAfterFn = std::move(submitAfter);
        s_waitFenceFn   = std::move(wait);
        s_checkFenceFn  = std::move(check);
    }

    sJobHandle submit(eJobType type, std::move_only_function<void()> fn) {
        return s_submitFn(type, std::move(fn));
    }

    sJobHandle submitAfter(uint32_t fence, eJobType type, std::move_only_function<void()> fn) {
        return s_submitAfterFn(fence, type, std::move(fn));
    }

    void waitFence(uint32_t fenceID) {
        s_waitFenceFn(fenceID);
    }

    bool checkFence(uint32_t fenceID) {
        return s_checkFenceFn(fenceID);
    }

    template<typename T>
    T* getService() {
        if (!s_serviceFn) return nullptr;
        return static_cast<T*>(s_serviceFn(std::type_index(typeid(T))));
    }
}