module;

#include <filesystem>
#include <fstream>
#include <expected>
#include <volk.h>

export module opn.Rendering.Util.vk.vkPipeline;
import opn.Rendering.Util.vk.vkInit;
export namespace vkUtil {

    std::expected<VkShaderModule, std::string> loadShaderModule(const std::filesystem::path& _filePath, VkDevice _device) {
        std::ifstream file(_filePath, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            return std::unexpected(std::format("Failed to open shader: {}", _filePath.string()));
        }

        size_t fileSize = file.tellg();

        std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

        file.seekg(0);
        file.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>( fileSize ) );
        file.close();

        VkShaderModuleCreateInfo createInfo{
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = fileSize,
            .pCode = buffer.data()
        };

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            return std::unexpected("Vulkan failed to create shader module.");
        }

        return shaderModule;
    }

}
