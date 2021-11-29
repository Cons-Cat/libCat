#include <cstdlib>
#include <unistd>

auto main() -> int {
    write(1, "Hello, Conscat!\n", 16);

    return EXIT_SUCCESS;
}
