#include <iostream>
#include <exception>
#include <typeinfo>

import opn.System.JobDispatcher;
import opn.System.ServiceManager;
import opn.System.SystemTypeList;
import opn.System.Services;
import opn.Utils.Logging;

using AppServices = opn::SystemTypeList<
    opn::AssetSystem,
    opn::TestSystem
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
