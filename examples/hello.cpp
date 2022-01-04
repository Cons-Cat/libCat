#include <linux>

void meow() {
    char8_t const* p_str = u8"Hello, Conscat!\n";
    write(1, p_str, 16).or_panic();
    exit(0);
}
