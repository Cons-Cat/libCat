#include <ccatio>
#include <ccatlib>
// #include <cstring>
// #include <unicat.h>

auto main() -> int32_t {
    constexpr char const* p_string = "Hello, Conscat!\n";
    // `printf()` is actually optimized here better by GCC than `write()`,
    // somehow.
    if (printf(p_string) < 0) {
        // if (write(1, p_string, strlen(p_string)) <= 0) {
        exit(1);
    };
};
