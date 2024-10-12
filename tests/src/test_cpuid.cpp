#include "../unit_tests.hpp"

TEST(test_cpuid) {
   bool has_avx2 = __builtin_cpu_supports("avx2");
   cat::verify(has_avx2);
}
