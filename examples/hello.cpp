#include <cstdlib>
#include <cunistd>

auto meow() -> Result<> {
    char const* p_str = "Hello, Conscat!\n";
    return write(1, p_str, 16);
}
