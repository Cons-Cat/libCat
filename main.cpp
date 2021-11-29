#include "cstdlib"
#include "unistd.h"

auto main() -> int {
    write(1, "Hello, Conscat!\n", 16);

    return 0;
}
