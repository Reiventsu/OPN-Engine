module;

#include <iterator>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

export module opn.System.Services.ImGui;
import opn.System.ServiceInterface;

export namespace opn {
    class ImGuiService final : public Service< ImGuiService > {
        VkDescriptorPool m_descriptorPool{};
        VkDescriptorPoolCreateInfo m_descriptorPoolCreateInfo{};
    protected:
        void onInit() override {
            VkDescriptorPoolSize m_descriptorPool[] = { { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
                                                        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
                                                        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
                                                        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
                                                        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
                                                        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
                                                        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
                                                        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
                                                        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
                                                        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
                                                        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
            };

            VkDescriptorPoolCreateInfo m_descriptorPoolCreateInfo = {};

            m_descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            m_descriptorPoolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
            m_descriptorPoolCreateInfo.maxSets = 1000;
            m_descriptorPoolCreateInfo.poolSizeCount = static_cast<uint32_t>(std::size(m_descriptorPool));
            m_descriptorPoolCreateInfo.pPoolSizes = m_descriptorPool;
        }
        void onPostInit() override {

        }

        void onShutdown() override {}
        void onUpdate(float _deltaTime) override { (void)_deltaTime; }
    };
}
