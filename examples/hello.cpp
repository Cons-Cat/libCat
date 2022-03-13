#include <linux>

void meow() {
    char const* p_str = "Hello, Conscat!\n";
    // TODO: See if `std::print()` is zero-overhead.
    nix::write(1, p_str, 16).or_panic();
    std::exit();
}
