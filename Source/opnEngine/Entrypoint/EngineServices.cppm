export module opn.System.EngineServices;

import opn.System.ServiceManager;
import opn.System.SystemTypeList;
import opn.System.Services;
import opn.Renderer.RenderPackage;
import opn.ECS;

export namespace opn {
    using EngineServices = SystemTypeList<
        Time,                             // Mandatory service
        WindowSystem,                     // Mandatory service
        Rendering< VulkanImpl >,
        AssetSystem,
        EntityComponentSystem
    >;

    using EngineServiceManager = ServiceManager_Impl< EngineServices >;

    template<typename T>
    const T& getService() {
        return EngineServiceManager::getService<T>();
    }

    template<typename T>
    bool isServiceRegistered() {
        return EngineServiceManager::isRegistered<T>();
    }
}
