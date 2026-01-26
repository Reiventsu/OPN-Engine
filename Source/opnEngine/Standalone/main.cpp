#include <iostream>
#include <exception>
#include <thread>
#include <typeinfo>

import opn.System.JobDispatcher;
import opn.System.ServiceManager;
import opn.System.SystemTypeList;
import opn.System.Services;
import opn.Utils.Logging;
import opn.Renderer.RenderPackage;

using AppServices = opn::SystemTypeList<
    opn::Time,         // Mandatory service
    opn::WindowSystem, // Mandatory service
    opn::Rendering< opn::VulkanImpl >,
    opn::AssetSystem
>;

using ServiceManager = opn::ServiceManager_Impl< AppServices >;

int main() {
    try {
        opn::logInfo( "Main", "Starting application..." );

        opn::JobDispatcher::init();
        ServiceManager::init();

        ServiceManager::registerServices();
        ServiceManager::postInitAll();

        auto &time = ServiceManager::getService<opn::Time>();
        auto &window = ServiceManager::getService<opn::WindowSystem>();

        while (!window.shouldClose()) {
            window.pollEvents();

            const auto dt = static_cast<float>(time.getDeltaTime());
            ServiceManager::updateAll(dt);
        }

        opn::logInfo("Main", "Engine Shutting down. {}", typeid(ServiceManager).name());
        ServiceManager::shutdown();
        opn::JobDispatcher::shutdown();

        opn::logInfo("Main", "Engine shutdown successfully.");
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
