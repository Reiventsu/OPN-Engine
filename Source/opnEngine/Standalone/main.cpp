#include <iostream>
#include <exception>
#include <typeinfo>

import opn.system.JobDispatcher;

int main() {
    try {
        opn::JobDispatcher::init();
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
