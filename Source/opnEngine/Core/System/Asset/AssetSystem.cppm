module;


export module opn.System.AssetSystem;
import opn.Utils.Singleton;

namespace opn {
    export class AssetSystem : public iSingleton<AssetSystem> {
        friend class iSingleton;

        AssetSystem() = default;

        ~AssetSystem() override = default;
    };
}
