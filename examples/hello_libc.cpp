#include <cstdio>
#include <cstdlib>

auto main() -> int32_t {
    constexpr char const* p_string = "Hello, Conscat!\n";
    // `printf()` is actually optimized here better by GCC than `write()`.
    if (printf(p_string) < 0) {
        exit(1);
    };
};
