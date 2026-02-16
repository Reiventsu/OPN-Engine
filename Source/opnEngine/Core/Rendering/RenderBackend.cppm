export module opn.Renderer.Backend;
import opn.Renderer.Types;
import opn.System.WindowSurfaceProvider;

export namespace opn {
    struct RenderBackend {
        virtual ~RenderBackend() = default;

        virtual void init() = 0;

        virtual void shutdown() = 0;

        virtual void update(float _deltaTime) = 0;

        virtual void draw() =0;

        virtual void drawWithReflection(const void *_rawData, const sShaderReflection &_reflection) = 0;

        virtual void bindToWindow(WindowSurfaceProvider &) = 0;
    };
}
