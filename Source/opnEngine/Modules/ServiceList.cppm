module;
#include <concepts>
#include <type_traits>

export module opn.System.EngineServiceList;

import opn.Modules.Services;
import opn.Renderer.RenderPackage;
import opn.System.SystemTypeList;
import opn.System.ServiceManager;

export namespace opn {
    using EngineServices = SystemTypeList<
        Time,
        WindowSystem,
        Rendering<VulkanImpl>,
        ShaderCompiler,
        AssetSystem,
        EntityComponentSystem
    >;

    using EngineServiceManager = ServiceManager_Impl<EngineServices>;

}
