#include <cstdlib>
#include <cunistd>
#include <utility>

auto main() -> int {
    char const* p_str = "Hello, Conscat!\n";
    char const* p_move_str = std::move(p_str);
    write(1, p_move_str, 16);
    return EXIT_SUCCESS;
}
