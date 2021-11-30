#include <cstdlib>
#include <cunistd>
#include <utility>

auto main() -> int {
    char const* p_str = "Hello, Conscat!\n";
    write(1, std::move(p_str), 16);
    return EXIT_SUCCESS;
}
