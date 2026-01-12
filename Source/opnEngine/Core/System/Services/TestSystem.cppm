export module opn.System.Service.TestSystem;
import opn.System.ServiceInterface;

namespace opn {
    export class TestSystem : public Service<TestSystem> {
    public:
        TestSystem() = default;
        
        void onInit() override {}
        void onShutdown() override {}
    };
}
