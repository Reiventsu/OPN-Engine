export module opn.System.Service.TestSystem;
import opn.System.iService;

namespace opn {
    export class TestSystem : public iService {
    public:
        TestSystem() = default;
        
        void init() override {}
        void shutdown() override {}
    };
}
