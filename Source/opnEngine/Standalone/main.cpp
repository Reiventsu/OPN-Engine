#include <iostream>
#include <exception>
#include <thread>
#include <typeinfo>

import opn.System.JobDispatcher;
import opn.System.ServiceManager;
import opn.System.SystemTypeList;
import opn.System.Services;
import opn.Utils.Logging;

using AppServices = opn::SystemTypeList<
    opn::Time,
    opn::AssetSystem
>;

using ServiceManager = opn::ServiceManager_Impl<AppServices>;

int main() {
    try {
        opn::logInfo("Main", "Starting application...");

        opn::JobDispatcher::init();
        ServiceManager::init();
        ServiceManager::registerServices();

        opn::logInfo("Main", "Application started successfully.");
        opn::logInfo("Main", "Engine running with {} services.", ServiceManager::getServiceCount());

        //// EXAMPLE BEGIN

        auto modelHandle = opn::JobDispatcher::submit(
            opn::eJobType::Asset,
            opn::AssetSystem::load("models/hero.gltf")
        );

        auto textureHandle = modelHandle.then(
            opn::eJobType::Asset,
            opn::AssetSystem::load("textures/mech_diffuse.png")
        );

        opn::logInfo("Main", "Main thread doing other work while assets load...");
        for (int i = 0; i < 3; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            opn::logTrace("Main", "Frame update...");
        }
        opn::logInfo("Main", "Waiting for Entity Spawn to complete...");
        textureHandle.wait();

        //// EXAMPLE END

        //// Shutdown

        opn::logInfo("Main", "Engine Shutting down.");
        ServiceManager::shutdown();
        opn::JobDispatcher::shutdown();

        opn::logInfo("Main", "Engine shutdown successfully.");
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
