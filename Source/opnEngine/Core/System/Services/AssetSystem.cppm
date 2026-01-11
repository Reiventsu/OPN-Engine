module;


export module opn.System.Service.AssetSystem;
import opn.System.iService;

namespace opn {
    export class AssetSystem : public iService {
    public:
        AssetSystem() = default;
        
        void init() override {}
        void shutdown() override {}
    };
}
