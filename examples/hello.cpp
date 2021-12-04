#include <cstdlib>
#include <cunistd>

void meow() {
    char const* p_str = "Hello, Conscat!\n";
    write(1, p_str, 16).or_panic();
}
