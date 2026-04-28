#include <cat/arithmetic>
#include <cat/array>
#include <cat/memory>

#include "../unit_tests.hpp"

// Regression for `arithmetic_ptr`: `void const*` must deduce
// `uintptr<void const>` without duplicate conversion operators. This matches
// `copy_memory_impl` holding addresses as `cat::uintptr` from `void const*` /
// `void*`.
static_assert(
   cat::is_same<decltype(cat::uintptr{static_cast<void const*>(nullptr)}),
                cat::uintptr<void const>>);
static_assert(cat::is_same<decltype(cat::uintptr{static_cast<void*>(nullptr)}),
                           cat::uintptr<void>>);

test(copy_memory) {
   using namespace cat::literals;
   using namespace cat::integers;

   // Disjoint small block (scalar tail path for these sizes).
   {
      cat::array<unsigned char, 64> source{};
      cat::array<unsigned char, 64> dest{};
      for (uword::raw_type i = 0u; i < 64u; ++i) {
         source[i] = static_cast<unsigned char>(i + 3u);
      }
      cat::copy_memory(source.data(), dest.data(), 64u);
      for (uword::raw_type i = 0u; i < 64u; ++i) {
         cat::verify(dest[i] == source[i]);
      }
   }

   // Large enough to run the SIMD pipeline in `detail::copy_memory_impl`
   // (`bytes` greater than eight native vectors of `int8`).
   {
      constexpr idx n = 2'000;
      cat::array<int4, n> source{};
      cat::array<int4, n> dest{};
      for (idx i = 0; i < n; ++i) {
         source[i] = static_cast<int4>(i);
      }
      cat::copy_memory(source.data(), dest.data(), sizeof(dest));
      for (idx i = 0; i < n; ++i) {
         cat::verify(source[i] == dest[i]);
      }
   }

   // Overlapping ranges use `copy_memory_backward_scalar` / memmove semantics.
   // Source low, destination higher: implementable by a backward pass.
   {
      cat::array<unsigned char, 48> buf{};
      cat::array<unsigned char, 48> initial{};
      for (uword::raw_type i = 0u; i < 48u; ++i) {
         initial[i] = static_cast<unsigned char>(i);
         buf[i] = initial[i];
      }
      unsigned char* const p_base = buf.data();
      cat::copy_memory_backward(p_base + 8, p_base + 12, 24u);
      cat::array<unsigned char, 48> expected{};
      for (uword::raw_type i = 0u; i < 48u; ++i) {
         expected[i] = initial[i];
      }
      for (uword::raw_type k = 24u; k > 0u;) {
         --k;
         expected[12u + k] = initial[8u + k];
      }
      for (uword::raw_type i = 0u; i < 48u; ++i) {
         cat::verify(buf[i] == expected[i]);
      }
   }

   // Source higher than destination: forward pass matches a snapshot copy.
   {
      cat::array<unsigned char, 48> buf{};
      cat::array<unsigned char, 48> initial{};
      for (uword::raw_type i = 0u; i < 48u; ++i) {
         initial[i] = static_cast<unsigned char>(i);
         buf[i] = initial[i];
      }
      unsigned char* const p_base = buf.data();
      cat::copy_memory_backward(p_base + 12, p_base + 8, 24u);
      cat::array<unsigned char, 48> expected{};
      for (uword::raw_type i = 0u; i < 48u; ++i) {
         expected[i] = initial[i];
      }
      for (uword::raw_type k = 0u; k < 24u; ++k) {
         expected[8u + k] = initial[12u + k];
      }
      for (uword::raw_type i = 0u; i < 48u; ++i) {
         cat::verify(buf[i] == expected[i]);
      }
   }
}
