#include <cat/detail/movsb.hpp>

#include <cat/arithmetic>
#include <cat/array>
#include <cat/bit>
#include <cat/cpuid>
#include <cat/memory>
#include <cat/page_allocator>

#include "../../src/libraries/memory/implementations/compare_memory_avx2.tpp"
#include "../unit_tests.hpp"

namespace {

void
initialize_bytes(
   cat::byte* _Nonnull p_data, cat::idx bytes, cat::byte seed = cat::byte(17u)
) {
   cat::uint1 value = seed.value;
   for (cat::idx i = 0u; i < bytes; ++i) {
      p_data[i] = cat::byte(value);
      value += 37u;
   }
}

void
fill_bytes(cat::byte* _Nonnull p_data, cat::idx bytes, cat::byte value) {
   for (cat::idx i = 0u; i < bytes; ++i) {
      p_data[i] = value;
   }
}

void
verify_copy_result(
   cat::byte const* _Nonnull p_source, cat::byte const* _Nonnull p_destination,
   cat::idx bytes
) {
   for (cat::idx i = 0u; i < bytes; ++i) {
      cat::verify(p_destination[i] == p_source[i]);
   }
}

void
verify_buffer_result(
   cat::byte const* _Nonnull p_expected, cat::byte const* _Nonnull p_actual,
   cat::idx bytes
) {
   for (cat::idx i = 0u; i < bytes; ++i) {
      cat::verify(p_actual[i] == p_expected[i]);
   }
}

auto
previous_index(cat::idx index) -> cat::idx {
   cat::iword const previous = index - 1u;
   return previous.to_idx().assert();
}

void
copy_case(cat::idx bytes, cat::idx source_skew, cat::idx destination_skew) {
   cat::array<cat::byte, 8_uki> source{};
   cat::array<cat::byte, 8_uki> destination{};

   initialize_bytes(source.data(), source.size());
   fill_bytes(destination.data(), destination.size(), cat::byte(0xA5u));

   cat::copy_memory(
      source.data() + source_skew, destination.data() + destination_skew, bytes
   );
   verify_copy_result(
      source.data() + source_skew, destination.data() + destination_skew, bytes
   );

   if (destination_skew > 0u) {
      cat::verify(destination[previous_index(destination_skew)] == 0xA5u);
   }
   cat::verify(destination[destination_skew + bytes] == 0xA5u);
}

void
overlap_case(
   cat::idx bytes, cat::idx source_offset, cat::idx destination_offset
) {
   cat::array<cat::byte, 8_uki> buffer{};
   cat::array<cat::byte, 8_uki> expected{};

   initialize_bytes(buffer.data(), buffer.size());
   initialize_bytes(expected.data(), expected.size());

   for (cat::idx i = 0u; i < bytes; ++i) {
      expected[destination_offset + i] = buffer[source_offset + i];
   }

   cat::copy_memory_backward(
      buffer.data() + source_offset, buffer.data() + destination_offset, bytes
   );
   verify_copy_result(expected.data(), buffer.data(), buffer.size());
}

void
large_overlap_case(
   cat::idx bytes, cat::idx source_offset, cat::idx destination_offset
) {
   cat::idx const capacity =
      bytes + max(source_offset, destination_offset) + 512u;
   cat::span page = pager.alloc_multi<cat::byte>(capacity).or_exit();
   cat::span expected = pager.alloc_multi<cat::byte>(capacity).or_exit();
   $defer {
      pager.free(page);
      pager.free(expected);
   };

   initialize_bytes(page.data(), capacity);
   initialize_bytes(expected.data(), capacity);
   for (cat::idx i = 0u; i < bytes; ++i) {
      expected[destination_offset + i] = page[source_offset + i];
   }

   cat::copy_memory_backward(
      page.data() + source_offset, page.data() + destination_offset, bytes
   );
   verify_buffer_result(expected.data(), page.data(), capacity);
}

void
compare_equal_case(cat::idx bytes, cat::idx left_skew, cat::idx right_skew) {
   cat::array<cat::byte, 8_uki> left{};
   cat::array<cat::byte, 8_uki> right{};

   initialize_bytes(left.data(), left.size());
   initialize_bytes(right.data(), right.size());
   for (cat::idx i = 0u; i < bytes; ++i) {
      right[right_skew + i] = left[left_skew + i];
   }

   cat::verify(
      cat::compare_memory(
         left.data() + left_skew, right.data() + right_skew, bytes
      )
      == 0
   );
}

void
compare_mismatch_case(cat::idx bytes, cat::idx mismatch_index, bool left_less) {
   cat::array<cat::byte, 8_uki> left{};
   cat::array<cat::byte, 8_uki> right{};

   initialize_bytes(left.data(), left.size());
   initialize_bytes(right.data(), right.size());
   left[mismatch_index] = cat::byte(left_less ? 1u : 240u);
   right[mismatch_index] = cat::byte(left_less ? 240u : 1u);

   auto const result = cat::compare_memory(left.data(), right.data(), bytes);
   cat::verify(left_less ? result < 0 : result > 0);
}

void
compare_avx2_case(cat::idx bytes, cat::idx mismatch_index, bool left_less) {
   cat::array<cat::byte, 512> left{};
   cat::array<cat::byte, 512> right{};

   initialize_bytes(left.data(), left.size());
   initialize_bytes(right.data(), right.size());
   left[mismatch_index] = cat::byte(left_less ? 1u : 240u);
   right[mismatch_index] = cat::byte(left_less ? 240u : 1u);

   auto const result =
      cat::detail::compare_memory_avx2(left.data(), right.data(), bytes);
   cat::verify(left_less ? result < 0 : result > 0);
}

}  // namespace

// Regression for `basic_intptr`: `void const*` must deduce
// `uintptr<void const>` without duplicate conversion operators. This matches
// `copy_memory_impl` holding addresses as `cat::uintptr` from `void const*` /
// `void*`.
static_assert(cat::is_same<
              decltype(cat::uintptr{static_cast<void const*>(nullptr)}),
              cat::uintptr<void const>>);
static_assert(
   cat::is_same<
      decltype(cat::uintptr{static_cast<void*>(nullptr)}), cat::uintptr<void>>
);

$test(copy_memory) {
   using namespace cat::literals;
   using namespace cat::arithmetic;

   // Small-copy sizes exercise `copy_memory_small` (Cosmopolitan-style switch).
   for (idx size : {
           0_idx,
           1_idx,
           2_idx,
           3_idx,
           4_idx,
           7_idx,
           8_idx,
           15_idx,
           16_idx,
           31_idx,
           32_idx,
           63_idx,
           64_idx,
           127_idx,
        }) {
      cat::array<cat::byte, 128> source{};
      cat::array<cat::byte, 128> dest{};
      initialize_bytes(source.data(), source.size(), cat::byte(11u));
      fill_bytes(dest.data(), dest.size(), cat::byte(0u));
      cat::copy_memory(source.data(), dest.data(), size);
      for (idx i = 0u; i < size; ++i) {
         cat::verify(dest[i] == source[i]);
      }
   }

   // Disjoint small block (scalar tail path for these sizes).
   {
      cat::array<cat::byte, 64> source{};
      cat::array<cat::byte, 64> dest{};
      initialize_bytes(source.data(), source.size(), cat::byte(3u));
      cat::copy_memory(source.data(), dest.data(), 64u);
      for (idx i = 0u; i < 64u; ++i) {
         cat::verify(dest[i] == source[i]);
      }
   }

   // Large enough to run the SIMD pipeline in `detail::copy_memory_impl`
   // (`bytes` greater than eight native `char1x_` vectors).
   {
      constexpr idx n = 2'000;
      cat::array<int4, n> source{};
      cat::array<int4, n> dest{};
      int4 value = 0;
      for (idx i = 0; i < n; ++i) {
         source[i] = value;
         ++value;
      }
      cat::copy_memory(source.data(), dest.data(), sizeof(dest));
      for (idx i = 0; i < n; ++i) {
         cat::verify(source[i] == dest[i]);
      }
   }

   // Overlapping ranges use `copy_memory_backward_scalar` / memmove semantics.
   // Source low, destination higher: implementable by a backward pass.
   {
      cat::array<cat::byte, 48> buf{};
      cat::array<cat::byte, 48> initial{};
      initialize_bytes(initial.data(), initial.size(), cat::byte(0u));
      for (idx i = 0u; i < 48u; ++i) {
         buf[i] = initial[i];
      }
      cat::byte* const p_base = buf.data();
      cat::copy_memory_backward(p_base + 8, p_base + 12, 24u);
      cat::array<cat::byte, 48> expected{};
      for (idx i = 0u; i < 48u; ++i) {
         expected[i] = initial[i];
      }
      for (idx k = 0u; k < 24u; ++k) {
         expected[12u + k] = initial[8u + k];
      }
      for (idx i = 0u; i < 48u; ++i) {
         cat::verify(buf[i] == expected[i]);
      }
   }

   // Source higher than destination: forward pass matches a snapshot copy.
   {
      cat::array<cat::byte, 48> buf{};
      cat::array<cat::byte, 48> initial{};
      initialize_bytes(initial.data(), initial.size(), cat::byte(0u));
      for (idx i = 0u; i < 48u; ++i) {
         buf[i] = initial[i];
      }
      cat::byte* const p_base = buf.data();
      cat::copy_memory_backward(p_base + 12, p_base + 8, 24u);
      cat::array<cat::byte, 48> expected{};
      for (idx i = 0u; i < 48u; ++i) {
         expected[i] = initial[i];
      }
      for (idx k = 0u; k < 24u; ++k) {
         expected[8u + k] = initial[12u + k];
      }
      for (idx i = 0u; i < 48u; ++i) {
         cat::verify(buf[i] == expected[i]);
      }
   }
}

$test(copy_memory_all_size_tiers_and_skews) {
   auto const saved_rep_support = cat::detail::memory_rep_string_support;
   $defer {
      cat::detail::memory_rep_string_support = saved_rep_support;
   };
   cat::detail::memory_rep_string_support = {};

   for (cat::idx bytes : {
           128_idx,
           129_idx,
           191_idx,
           255_idx,
           256_idx,
           511_idx,
           512_idx,
           513_idx,
           1'023_idx,
           1'024_idx,
           1'057_idx,
           2'048_idx,
           2'111_idx,
        }) {
      copy_case(bytes, 0u, 0u);
      copy_case(bytes, 3u, 5u);
      copy_case(bytes, 17u, 1u);
   }
}

$test(copy_memory_rep_movsb_path) {
   auto const saved_rep_support = cat::detail::memory_rep_string_support;
   $defer {
      cat::detail::memory_rep_string_support = saved_rep_support;
   };
   cat::detail::memory_rep_string_support = {
      .has_erms = true,
      .has_fsrm = false,
   };

   copy_case(2'048u, 7u, 11u);
}

$test(copy_memory_direct_rep_string_helpers) {
   cat::array<cat::byte, 160> source{};
   cat::array<cat::byte, 160> destination{};
   initialize_bytes(source.data(), source.size());

   x64::movsb(destination.data(), source.data(), 96u);
   verify_copy_result(source.data(), destination.data(), 96u);

   cat::array<cat::byte, 160> buffer{};
   cat::array<cat::byte, 160> expected{};
   initialize_bytes(buffer.data(), buffer.size());
   initialize_bytes(expected.data(), expected.size());
   for (cat::idx i = 0u; i < 96u; ++i) {
      expected[32u + i] = buffer[16u + i];
   }

   x64::movsb_backward(buffer.data() + 32u, buffer.data() + 16u, 96u);
   verify_copy_result(expected.data(), buffer.data(), buffer.size());
}

$test(copy_memory_overlap_all_size_tiers) {
   for (cat::idx bytes : {
           128_idx,
           129_idx,
           255_idx,
           511_idx,
           512_idx,
           777_idx,
           1'024_idx,
           1'057_idx,
        }) {
      overlap_case(bytes, 256u, 128u);
      overlap_case(bytes, 128u, 256u);
   }
}

$test(copy_memory_overlap_large_non_temporal_tiers) {
   cat::idx const bytes = 2_umi + 257u;
   large_overlap_case(bytes, 512u, 128u);
   large_overlap_case(bytes, 128u, 517u);
}

$test(compare_memory) {
   using namespace cat::literals;

   {
      cat::array<cat::byte, 64> left{};
      cat::array<cat::byte, 64> right{};

      // Zero-length compare is equal regardless of contents.
      cat::verify(cat::compare_memory(left.data(), right.data(), 0u) == 0);

      initialize_bytes(left.data(), left.size(), cat::byte(0u));
      initialize_bytes(right.data(), right.size(), cat::byte(0u));
      cat::verify(cat::compare_memory(left.data(), right.data(), 64u) == 0);
      right[10] = cat::byte(240u);
      cat::verify(cat::compare_memory(left.data(), right.data(), 64u) < 0);
      right[10] = cat::byte(0u);
      cat::verify(cat::compare_memory(left.data(), right.data(), 64u) > 0);
   }

   // Large enough to run the SIMD pipeline in `detail::compare_memory_impl`
   // (`bytes` greater than eight native `char1x_` vectors).
   {
      constexpr idx n = 2'000;
      cat::array<cat::byte, n> left{};
      cat::array<cat::byte, n> right{};
      initialize_bytes(left.data(), n);
      initialize_bytes(right.data(), n);
      cat::verify(cat::compare_memory(left.data(), right.data(), n) == 0);
      right[1'500] = cat::byte(0u);
      cat::verify(cat::compare_memory(left.data(), right.data(), n) > 0);
      right[1'500] = cat::byte(255u);
      cat::verify(cat::compare_memory(left.data(), right.data(), n) < 0);
   }
}

$test(compare_memory_all_size_tiers_and_mismatch_positions) {
   for (cat::idx bytes : {
           1_idx,
           16_idx,
           17_idx,
           31_idx,
           32_idx,
           33_idx,
           63_idx,
           64_idx,
           95_idx,
           96_idx,
           127_idx,
           128_idx,
           255_idx,
           256_idx,
           300_idx,
           1'024_idx,
           1'057_idx,
        }) {
      compare_equal_case(bytes, 0u, 0u);
      compare_mismatch_case(bytes, 0u, true);
      compare_mismatch_case(bytes, bytes / 2u, false);
      compare_mismatch_case(bytes, previous_index(bytes), true);
   }

   compare_equal_case(257u, 3u, 5u);
}

$test(compare_memory_large_non_temporal_size) {
   cat::idx const bytes = 2_umi + 129u;
   cat::span left = pager.alloc_multi<cat::byte>(bytes).or_exit();
   cat::span right = pager.alloc_multi<cat::byte>(bytes).or_exit();
   $defer {
      pager.free(left);
      pager.free(right);
   };

   initialize_bytes(left.data(), bytes);
   initialize_bytes(right.data(), bytes);
   cat::verify(cat::compare_memory(left.data(), right.data(), bytes) == 0);
   cat::idx const last = previous_index(bytes);
   left[last] = cat::byte(1u);
   right[last] = cat::byte(240u);
   cat::verify(cat::compare_memory(left.data(), right.data(), bytes) < 0);
}

$test(compare_memory_direct_avx2_paths_when_supported) {
   if (cat::detail::simd_dispatch_priority < 80) {
      return;
   }

   for (cat::idx bytes : {
           17_idx,
           32_idx,
           33_idx,
           63_idx,
           64_idx,
           95_idx,
           96_idx,
           127_idx,
           160_idx,
        }) {
      compare_avx2_case(bytes, 0u, true);
      compare_avx2_case(bytes, bytes / 2u, false);
      compare_avx2_case(bytes, previous_index(bytes), true);
   }
}
