module;
#include "volk.h"
#include <vector>
#include <cstdint>
#include <cstring>
#include <typeindex>

export module opn.Renderer.Types;

export namespace opn {

    struct sSetLayoutData {
        uint32_t setIndex;
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        bool operator==(const sSetLayoutData &_other) const {
            if (setIndex != _other.setIndex) return false;
            if (bindings.size() != _other.bindings.size()) return false;

            for (size_t i = 0; i < bindings.size(); i++) {
                if (memcmp(&bindings[i], &_other.bindings[i],
                           sizeof(VkDescriptorSetLayoutBinding)) != 0)
                    return false;
            }
            return true;
        }
    };

    struct sShaderReflection {
        std::vector<uint32_t> byteCode;
        std::vector<sSetLayoutData> setLayouts;
        std::vector<VkPushConstantRange> pushConstants;
    };

    // Hashing
    struct sPipelineSignature {
        std::vector<sSetLayoutData> setLayouts;
        std::vector<VkPushConstantRange> pushConstants;

        bool operator==(const sPipelineSignature &_other) const {
            if (setLayouts.size() != _other.setLayouts.size()) return false;
            if (pushConstants.size() != _other.pushConstants.size()) return false;

            for (size_t i = 0; i < setLayouts.size(); i++) {
                if (!(setLayouts[i] == _other.setLayouts[i])) return false;
            }

            for (size_t i = 0; i < pushConstants.size(); i++) {
                if (memcmp(&pushConstants[i], &_other.pushConstants[i],
                           sizeof(VkPushConstantRange)) != 0)
                    return false;
            }
            return true;
        }

        static sPipelineSignature from(const sShaderReflection &_reflection) {
            return {_reflection.setLayouts, _reflection.pushConstants};
        }
    };

    struct sPipelineSignatureHash {
        static constexpr std::hash<uint32_t> hasher32{};

        size_t operator()(const sPipelineSignature &signature) const noexcept {
            size_t seed = 0;
            auto combine = [&seed](const size_t _h) {
                // magic number: 2^32 / golden ratio.
                seed ^= _h + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            };
            for (const auto &[setIndex, bindings]: signature.setLayouts) {
                combine(std::hash<uint32_t>{}(setIndex));
                for (const auto &binding: bindings) {
                    combine(hasher32(binding.binding));
                    combine(hasher32(static_cast<uint32_t>(binding.descriptorType)));
                    combine(hasher32(binding.stageFlags));
                    combine(hasher32(binding.descriptorCount));
                }
            }
            for (const auto &[stageFlags, offset, size]: signature.pushConstants) {
                combine(hasher32(offset));
                combine(hasher32(size));
                combine(hasher32(stageFlags));
            }
            return seed;
        }
    };
}
