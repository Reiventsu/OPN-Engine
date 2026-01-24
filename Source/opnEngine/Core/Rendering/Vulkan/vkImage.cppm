module;
#include <volk.h>
export module opn.Rendering.Util.vk.vkImage;
import opn.Rendering.Util.vk.vkInit;
import opn.Utils.Logging;

export namespace vkUtil {

    void transition_image( PFN_vkCmdPipelineBarrier2 _barrierFunc
                         , VkCommandBuffer _cmd
                         , VkImage _image
                         , VkImageLayout _currentLayout
                         , VkImageLayout _newLayout)
    {
        VkImageMemoryBarrier2 imageBarrier{.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2};
        imageBarrier.pNext = nullptr;

        imageBarrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        imageBarrier.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
        imageBarrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
        imageBarrier.dstAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT | VK_ACCESS_2_MEMORY_READ_BIT;

        imageBarrier.oldLayout = _currentLayout;
        imageBarrier.newLayout = _newLayout;

        VkImageAspectFlags aspectMask = (_newLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL)
                                            ? VK_IMAGE_ASPECT_DEPTH_BIT
                                            : VK_IMAGE_ASPECT_COLOR_BIT;
        imageBarrier.subresourceRange = vkinit::image_subresource_range(aspectMask);
        imageBarrier.image = _image;

        VkDependencyInfo depInfo = {
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            .pNext = nullptr,
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &imageBarrier,
        };

        if (_barrierFunc == nullptr)
            opn::logCritical("vkUtil","L bozo");

        _barrierFunc(_cmd, &depInfo);
    }
}
