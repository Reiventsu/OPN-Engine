module;

// THIS IS JUST KINDA HERE PLEASE IGNORE FOR NOW

export module opn.Renderer.DirectX;
import opn.Plugins.ThirdParty.hlslpp;
import opn.Utils.Logging;

export namespace opn {
    class DirectXImpl {
    public:
        void initDirectX() {
            logInfo("DirectX Backend", "Initializing...");
        };

        void shutdownDirectX() {
            logInfo("DirectX Backend", "Shutting down...");
        }
    };
}
