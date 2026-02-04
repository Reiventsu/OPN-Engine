export module opn.ECS.Components:Renderable;
import opn.Assets.Types;

export namespace opn::components {
    struct Renderable {
        sAssetHandle meshHandle;
        sAssetHandle materialHandle;
        bool visible{true};
    };
}