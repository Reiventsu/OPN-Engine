module;

#include <span>

#include "volk.h"
#include <vector>

export module opn.Rendering.Util.vk.vkDescriptors;
import opn.Rendering.Util.vk.vkTypes;

export namespace vkDesc {
    struct sDescriptorLayoutBuilder {

        std::pmr::vector<VkDescriptorSetLayoutBinding> bindings;

        void add_binding( uint32_t         _binding
                        , VkDescriptorType _type
        ) {
            VkDescriptorSetLayoutBinding newBind{ };
            newBind.binding         = _binding;
            newBind.descriptorCount = 1;
            newBind.descriptorType  = _type;

            bindings.push_back( newBind );
        }

        void clear() {
            bindings.clear();
        }

        VkDescriptorSetLayout build( VkDevice                         _device
                                   , VkShaderStageFlags               _shaderStages
                                   , void*                            _pNext = nullptr
                                   , VkDescriptorSetLayoutCreateFlags _flags = 0
        ) {
            for( auto& binding : bindings ) {
                binding.stageFlags |= _shaderStages;
            }

            VkDescriptorSetLayoutCreateInfo info{ .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
            info.pNext     = _pNext;
            info.pBindings = bindings.data();
            info.bindingCount = static_cast< uint32_t >( bindings.size() );
            info.flags     = _flags;

            VkDescriptorSetLayout set;
            vkUtil::vkCheck( vkCreateDescriptorSetLayout( _device
                                                        , &info
                                                        , nullptr
                                                        , &set )
                                                        , "vkCreateDescriptorSetLayout"
            );

            return set;
        }
    };

    struct sDescriptorAllocator {

        struct sPoolSizeRatio {
            VkDescriptorType type;
            float            ratio;
        };

        VkDescriptorPool pool;

        void initPool( VkDevice                    _device
                     , uint32_t                    _maxSets
                     , std::span< sPoolSizeRatio > _poolRatios
        ) {
            std::vector< VkDescriptorPoolSize > poolSizes;
            for( sPoolSizeRatio ratio : _poolRatios ) {
                poolSizes.push_back( VkDescriptorPoolSize{
                    .type            = ratio.type,
                    .descriptorCount = static_cast< uint32_t >( ratio.ratio * static_cast< float >( _maxSets ) )
                } );
            }

            VkDescriptorPoolCreateInfo poolInfo{ .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
            poolInfo.flags         = 0;
            poolInfo.maxSets       = _maxSets;
            poolInfo.poolSizeCount = static_cast< uint32_t >( poolSizes.size() );
            poolInfo.pPoolSizes    = poolSizes.data();

            vkCreateDescriptorPool( _device
                                  , &poolInfo
                                  , nullptr
                                  , &pool
            );
        };

        void clearDescriptors( VkDevice _device ) {
            vkResetDescriptorPool( _device, pool, 0 );
        };

        void destroyPool( VkDevice _device ) {
            vkDestroyDescriptorPool( _device, pool, nullptr );
        };

        VkDescriptorSet allocate( VkDevice              _device
                                , VkDescriptorSetLayout _layout
        ) {
            VkDescriptorSetAllocateInfo allocInfo = { .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };

            allocInfo.pNext              = nullptr;
            allocInfo.descriptorPool     = pool;
            allocInfo.descriptorSetCount = 1;
            allocInfo.pSetLayouts        = &_layout;

            VkDescriptorSet descriptorSet;
            vkUtil::vkCheck( vkAllocateDescriptorSets( _device
                                                     , &allocInfo
                                                     , &descriptorSet )
                                                     , "vkAllocateDescriptorSets"
            );

            return descriptorSet;
        }
    };
}
