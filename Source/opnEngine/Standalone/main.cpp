#include <iostream>
#include <exception>

import opn.system.JobDispatcher;

int main() {
    try {
        opn::JobDispatcher::init();
    } catch (const std::exception &e) {
        std::println(std::cerr, "Exception caught: {}\n...TERMINATING...", e.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
