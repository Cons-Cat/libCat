#include "../unit_tests.hpp"

// TODO: We need to rethink how this is tested across different computers.
// Currently the test is disabled. It is partially tested by extension of
// `$simd_switch` and `cat::bitset`.
$test(cpuid) {
   cat::verify(__builtin_cpu_supports("cmov"));
   cat::verify(__builtin_cpu_supports("mmx"));
   cat::verify(__builtin_cpu_supports("sse"));
   cat::verify(__builtin_cpu_supports("sse2"));

   cat::verify(__builtin_cpu_supports("avx2"));

   cat::verify(__builtin_cpu_is("skylake"));

   cat::verify(__builtin_cpu_supports("x86-64"));
   cat::verify(__builtin_cpu_supports("x86-64-v2"));
   cat::verify(__builtin_cpu_supports("x86-64-v3"));
}
