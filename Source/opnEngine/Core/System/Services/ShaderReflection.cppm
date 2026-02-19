module;
#include <expected>
#include <filesystem>
#include <unordered_map>
#include <vector>

#include "slang-com-ptr.h"
#include "volk.h"
export module opn.System.Service.ShaderReflection;
import opn.Renderer.Types;
import opn.System.ServiceInterface;
import opn.Utils.Logging;

using Slang::ComPtr;

export namespace opn {

    class ShaderCompiler final : public Service<ShaderCompiler> {
        ComPtr<slang::IGlobalSession> m_globalSession;
        mutable std::unordered_map<std::string, sShaderReflection> m_reflectionCache;

    protected:
        void onInit() override {
            if (SLANG_FAILED(slang::createGlobalSession(m_globalSession.writeRef())))
                opn::logCritical("Service: ShaderCompiler", "Failed to create global session.");
            opn::logInfo("Service: ShaderCompiler", "ShaderCompiler initialized.");
        }

        void onShutdown() override {
            m_globalSession.setNull();
        }

    public:
        [[nodiscard]] std::expected<sShaderReflection, std::string>
        compile(const std::filesystem::path &_filePath, const std::string_view _entryPoint = "main") const {
            const auto key = _filePath.string();
            if (const auto itr = m_reflectionCache.find(key); itr != m_reflectionCache.end()) {
                return itr->second;
            }

            slang::SessionDesc sessionDesc{};
            slang::TargetDesc targetDesc{};
            targetDesc.format = SLANG_SPIRV;
            targetDesc.profile = m_globalSession->findProfile("spirv_1_5");

            sessionDesc.targets = &targetDesc;
            sessionDesc.targetCount = 1;

            ComPtr<slang::ISession> session;
            if (SLANG_FAILED(m_globalSession->createSession(sessionDesc, session.writeRef())))
                return std::unexpected("Failed to create Slang session.");

            ComPtr<slang::IBlob> diagnosticBlob;
            slang::IModule *module = session->loadModule(_filePath.string().c_str(), diagnosticBlob.writeRef());

            if (diagnosticBlob) {
                opn::logInfo("Slang", "Diagnostics: {}",
                             static_cast<const char *>(diagnosticBlob->getBufferPointer()));
            }
            if (!module) return std::unexpected("Failed to load module: " + _filePath.string());

            ComPtr<slang::IEntryPoint> entryPoint;
            module->findEntryPointByName(_entryPoint.data(), entryPoint.writeRef());

            slang::IComponentType *components[] = {module, entryPoint.get()};
            ComPtr<slang::IComponentType> linkedProgram;
            if (SLANG_FAILED(session->createCompositeComponentType(components, 2 ,linkedProgram.writeRef())))
                return std::unexpected("Failed to link shader components.");

            ComPtr<slang::IBlob> spirvBlob;
            ComPtr<slang::IBlob> outDiagnostic;
            if (SLANG_FAILED(linkedProgram->getEntryPointCode(0,0,spirvBlob.writeRef(), outDiagnostic.writeRef()))) {
                if (outDiagnostic)
                    opn::logError("Service: ShaderCompiler", "Linker: {}",
                                  static_cast<const char *>(outDiagnostic->getBufferPointer()));
                return std::unexpected("SPIR-V generation failed.");
            }

            sShaderReflection result;
            auto code = static_cast<const uint32_t *>(spirvBlob->getBufferPointer());
            result.byteCode.assign(code, code + (spirvBlob->getBufferSize() / sizeof(uint32_t)));

            slang::ProgramLayout *layout = linkedProgram->getLayout();
            reflectResources(layout, result);

            m_reflectionCache.emplace(key, result);
            return result;
        }

    private:
        void reflectResources(slang::ProgramLayout *_layout, sShaderReflection &_out) const {
            uint32_t parameterCount = _layout->getParameterCount();

            std::unordered_map<uint32_t, std::vector<VkDescriptorSetLayoutBinding> > setGroups;

            for (uint32_t i = 0; i < parameterCount; ++i) {
                slang::VariableLayoutReflection *variable = _layout->getParameterByIndex(i);
                slang::ParameterCategory category = variable->getCategory();

                if (category == slang::ParameterCategory::PushConstantBuffer) {
                    _out.pushConstants.push_back({
                        .stageFlags = VK_SHADER_STAGE_ALL,
                        .offset = static_cast<uint32_t>(variable->getOffset()),
                        .size = static_cast<uint32_t>(variable->getTypeLayout()->getSize())
                    });
                    continue;
                }

                uint32_t setIndex = variable->getBindingSpace();
                uint32_t bindingIndex = variable->getBindingIndex();

                VkDescriptorSetLayoutBinding binding{};
                binding.binding = bindingIndex;
                binding.descriptorCount = 1;
                binding.descriptorType = mapType(variable->getTypeLayout());
                binding.stageFlags = VK_SHADER_STAGE_ALL;

                setGroups[setIndex].push_back(binding);
            }

            for (auto &[index, bindings]: setGroups) {
                _out.setLayouts.push_back({
                    .setIndex = index,
                    .bindings = std::move(bindings)
                });
            }
        }

        VkDescriptorType mapType(slang::TypeLayoutReflection *_typeLayout) const {
            auto kind = _typeLayout->getType()->getKind();

            switch (kind) {
                case slang::TypeReflection::Kind::ConstantBuffer:
                    return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                case slang::TypeReflection::Kind::Resource: {
                    const auto shape = _typeLayout->getType()->getResourceShape();
                    if (shape & SLANG_TEXTURE_2D) return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                    return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                }
                case slang::TypeReflection::Kind::SamplerState:
                    return VK_DESCRIPTOR_TYPE_SAMPLER;
                default: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            }
        }
    };
}
