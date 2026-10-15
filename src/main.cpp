#include "socket_handler.hpp"

#include <cstdlib>

int main(
    int argc,
    char** argv
) {
    if (
        argc != 2
    ) {
        return EXIT_FAILURE;
    }

    socket_handler socket1;
    socket_libary::initialise_socket(
        &socket1,
        3,
        argv[1]
    );
    return EXIT_SUCCESS;
}
