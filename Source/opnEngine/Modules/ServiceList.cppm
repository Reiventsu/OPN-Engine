export module opn.Modules.ServiceList;

import opn.Modules.Services;
import opn.Renderer.RenderPackage;
import opn.System.SystemTypeList;
import opn.System.ServiceManager;

// Defines services to be used in order of dependency.
export namespace opn {
    using EngineServices = SystemTypeList<
        Time,
        WindowSystem,
        Rendering<VulkanImpl>,
        ShaderCompiler,
        AssetService,
        EntityComponentSystem
    >;
    using EngineServiceManager = ServiceManager_Impl<EngineServices>;
}
