export module opn.ECS.Components.Renderable;
import opn.System.Service.AssetSystem;

export namespace opn::components {
    struct Renderable {
        sAssetHandle meshHandle;
        sAssetHandle materialHandle;
        bool visible{true};
    };
}