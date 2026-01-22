export module opn.Renderer.Backend;

import opn.System.WindowSurfaceProvider;

export namespace opn {
    struct RenderBackend {
        virtual ~RenderBackend() = default;
        virtual void init() = 0;
        virtual void shutdown() = 0;
        virtual void update(float _deltaTime) = 0;
        virtual void render() = 0;
        virtual void bindToWindow(const WindowSurfaceProvider&) = 0;
    };
}