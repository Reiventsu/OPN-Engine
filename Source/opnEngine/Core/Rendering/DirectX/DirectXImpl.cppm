module;

// THIS IS JUST KINDA HERE PLEASE IGNORE FOR NOW

export module opn.Renderer.DirectX;
import opn.Renderer.Backend;
import opn.System.WindowSurfaceProvider;
import opn.Utils.Logging;

export namespace opn {
    class DirectXImpl : public RenderBackend {
    public:
        void initDirectX() {
            logInfo("DirectX Backend", "Initializing...");
        };

        void shutdownDirectX() {
            logInfo("DirectX Backend", "Shutting down...");
        }

        void init() override {
            initDirectX();
        }

        void shutdown() override {
            shutdownDirectX();
        }

        void update(float /*_deltaTime*/) override {
            // TODO
        }

        void draw() override {
            // TODO
        }

        void bindToWindow(WindowSurfaceProvider & /*_window*/) override {
            // TODO
        }
    };
}
