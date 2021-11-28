// #include <unistd.h>
#include "cstdlib"
#include "utility"

auto main() -> int {
    write(1, "Hello, Conscat!\n", 16);
    return EXIT_SUCCESS;
}
