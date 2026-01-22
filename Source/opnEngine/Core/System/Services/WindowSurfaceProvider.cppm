module;
#include <vulkan/vulkan_core.h>
export module opn.System.WindowSurfaceProvider;

export namespace opn {
    struct WindowSurfaceProvider {
        struct sWindowDimension {
            uint32_t width = 0;
            uint32_t height = 0;
        } dimension;

        virtual ~WindowSurfaceProvider() = default;

        virtual sWindowDimension getDimensions() {
            return dimension;
        };

        virtual void setDimensions(const uint32_t _width, const uint32_t _height) {
            dimension.width = _width;
            dimension.height = _height;
        };

        virtual VkSurfaceKHR createSurface(VkInstance _instance) const = 0;
    };
}
