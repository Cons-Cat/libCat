#include <cunistd>
#include <utility>

auto main() -> i32 {
    char const* p_str = "Hello, Conscat!\n";
    write(1, p_str, 16);
    return EXIT_SUCCESS;
}
