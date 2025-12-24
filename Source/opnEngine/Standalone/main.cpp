#include <iostream>
#include <ostream>
#include <stacktrace>


int main() {

    try {
    } catch ( const std::exception& e ) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
