#include <array>
#include <cstdlib>
#include <cstring>

auto main() -> int32_t {
    std::array<int32_t, 200000> source_2000;
    std::array<int32_t, 200000> dest_2000;
    memcpy(&dest_2000, &source_2000, sizeof(dest_2000));
    // Prevent these from being optimized out.
    asm volatile("" ::"m"(source_2000));
    asm volatile("" ::"m"(dest_2000));
    return 0;
}
