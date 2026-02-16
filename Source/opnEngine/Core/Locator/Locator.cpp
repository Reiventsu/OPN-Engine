module opn.Locator;

import opn.System.EngineServiceList;

namespace opn {
    class JobDispatcher;

    iService* Locator::_internal_getService(std::type_index _type) {
        return EngineServiceManager::getRawService(_type);
    }

    JobDispatcher* Locator::_internal_getDispatcher() {
        static JobDispatcher dummy;
        return &dummy;
    }
}