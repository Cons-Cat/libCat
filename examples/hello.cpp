#include <cstdlib>
#include <cunistd>
#include <pthread.h>

auto main() -> i32 {
    char const* p_str = "Hello, Conscat!\n";
    write(1, p_str, 16).or_panic();
    return EXIT_SUCCESS;
}
