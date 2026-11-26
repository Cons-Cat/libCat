#include <cat/detail/movsb.hpp>

#include <cat/allocator_page>
#include <cat/arithmetic>
#include <cat/array>
#include <cat/bit>
#include <cat/cpuid>
#include <cat/memory>
#include <cat/simd_switch>

#include "../../src/libraries/memory/implementations/copy_memory_backward_large.tpp"
#include "../../src/libraries/memory/implementations/copy_memory_large.tpp"
#include "../../src/libraries/memory/implementations/scan_memory.tpp"
#include "../../src/libraries/memory/implementations/scan_memory_avx2.tpp"
#include "../../src/libraries/memory/implementations/scan_memory_medium.tpp"
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
   cat::span page = g_tests_alloc.alloc_multi<cat::byte>(capacity).or_exit();
   cat::span expected =
      g_tests_alloc.alloc_multi<cat::byte>(capacity).or_exit();
   $defer {
      g_tests_alloc.free(page);
      g_tests_alloc.free(expected);
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

// Order two ranges by the bytes where they first differ, like `memcmp`.
auto
memory_order(
   cat::byte const* _Nonnull p_left, cat::byte const* _Nonnull p_right,
   cat::idx bytes
) -> std::strong_ordering {
   cat::idx const offset = cat::find_disjoint_memory(p_left, p_right, bytes);
   if (offset == bytes) {
      return std::strong_ordering::equal;
   }

   return p_left[offset].value <=> p_right[offset].value;
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
      cat::find_disjoint_memory(
         left.data() + left_skew, right.data() + right_skew, bytes
      )
      == bytes
   );
   cat::verify(
      cat::is_memory_equal(
         left.data() + left_skew, right.data() + right_skew, bytes
      )
   );
   cat::verify(
      cat::is_memory_equal_scalar(
         left.data() + left_skew, right.data() + right_skew, bytes
      )
   );
}

void
compare_mismatch_case(cat::idx bytes, cat::idx mismatch_index, bool left_less) {
   cat::array<cat::byte, 8_uki> left;
   cat::array<cat::byte, 8_uki> right;

   // Vary both ranges' alignment with the mismatch position.
   cat::byte* _Nonnull const p_left = left.data() + (mismatch_index % 7u);
   cat::byte* _Nonnull const p_right = right.data() + (mismatch_index % 5u);
   initialize_bytes(left.data(), bytes + 8u);
   for (cat::idx i = 0u; i < bytes; ++i) {
      p_right[i] = p_left[i];
   }

   p_left[mismatch_index] = cat::byte(left_less ? 1u : 240u);
   p_right[mismatch_index] = cat::byte(left_less ? 240u : 1u);
   // Only one byte differs, so equality must notice it alone.
   cat::verify(!cat::is_memory_equal(p_left, p_right, bytes));
   cat::verify(!cat::is_memory_equal_scalar(p_left, p_right, bytes));

   // A later difference ordered the other way must not be found instead.
   cat::idx const last = previous_index(bytes);
   if (mismatch_index != last) {
      p_left[last] = cat::byte(left_less ? 255u : 0u);
      p_right[last] = cat::byte(left_less ? 0u : 255u);
   }

   std::strong_ordering const order = memory_order(p_left, p_right, bytes);
   cat::verify(left_less ? order < 0 : order > 0);
   cat::verify(
      cat::find_disjoint_memory(p_left, p_right, bytes) == mismatch_index
   );
   cat::verify(
      cat::find_disjoint_memory_scalar(p_left, p_right, bytes) == mismatch_index
   );
}

void
compare_avx2_case(cat::idx bytes, cat::idx mismatch_index, bool left_less) {
   cat::array<cat::byte, 512> left;
   cat::array<cat::byte, 512> right;

   initialize_bytes(left.data(), bytes);
   initialize_bytes(right.data(), bytes);
   using cat::detail::memory_scan;
   cat::verify(!cat::detail::scan_memory_avx2<memory_scan::equality>(
      left.data(), right.data(), bytes
   ));
   cat::verify(
      cat::detail::scan_memory_avx2<memory_scan::mismatch>(
         left.data(), right.data(), bytes
      )
         .is_empty()
   );

   left[mismatch_index] = cat::byte(left_less ? 1u : 240u);
   right[mismatch_index] = cat::byte(left_less ? 240u : 1u);
   cat::verify(
      cat::detail::scan_memory_avx2<memory_scan::equality>(
         left.data(), right.data(), bytes
      )
   );

   // A later difference must not be found instead.
   if (mismatch_index != previous_index(bytes)) {
      left[previous_index(bytes)] = cat::byte(0u);
      right[previous_index(bytes)] = cat::byte(255u);
   }

   cat::maybe<cat::idx> const result =
      cat::detail::scan_memory_avx2<memory_scan::mismatch>(
         left.data(), right.data(), bytes
      );
   cat::verify(result.has_value());
   cat::verify(result.value() == mismatch_index);
   cat::verify((left[mismatch_index] < right[mismatch_index]) == left_less);
}

void
compare_medium_case(cat::idx bytes, cat::idx mismatch_index) {
   cat::array<cat::byte, 512> left{};
   cat::array<cat::byte, 512> right{};

   initialize_bytes(left.data(), left.size());
   initialize_bytes(right.data(), right.size());
   using cat::detail::memory_scan;
   cat::verify(!cat::detail::scan_memory_medium<memory_scan::equality>(
      left.data(), right.data(), bytes
   ));
   cat::verify(
      cat::detail::scan_memory_medium<memory_scan::mismatch>(
         left.data(), right.data(), bytes
      )
         .is_empty()
   );

   left[mismatch_index] = cat::byte(1u);
   right[mismatch_index] = cat::byte(240u);
   cat::verify(
      cat::detail::scan_memory_medium<memory_scan::equality>(
         left.data(), right.data(), bytes
      )
   );

   // A later difference must not be found instead.
   left[previous_index(bytes)] = cat::byte(0u);
   right[previous_index(bytes)] = cat::byte(255u);
   cat::verify(
      cat::detail::scan_memory_medium<memory_scan::mismatch>(
         left.data(), right.data(), bytes
      )
         .value()
      == mismatch_index
   );
}

// Verify `scan_memory_large` with `Simd` in both modes. It is inlined, so the
// `$simd_switch` arm that calls it supplies its `gnu::target`.
template <typename Simd>
[[gnu::always_inline]]
inline void
verify_large(
   cat::byte const* _Nonnull p_left, cat::byte const* _Nonnull p_right,
   cat::idx bytes, cat::maybe<cat::idx> mismatch
) {
   using cat::detail::memory_scan;
   cat::verify(
      cat::detail::scan_memory_large<memory_scan::equality, Simd>(
         p_left, p_right, bytes
      )
      == mismatch.has_value()
   );
   cat::verify(
      cat::detail::scan_memory_large<memory_scan::mismatch, Simd>(
         p_left, p_right, bytes
      )
      == mismatch
   );
}

template <typename Simd>
[[gnu::always_inline]]
inline void
compare_large_case(cat::idx bytes, cat::idx mismatch_index) {
   cat::array<cat::byte, 1_uki> left;
   cat::array<cat::byte, 1_uki> right;

   initialize_bytes(left.data(), bytes);
   initialize_bytes(right.data(), bytes);
   left[mismatch_index] = cat::byte(1u);
   right[mismatch_index] = cat::byte(240u);
   verify_large<Simd>(left.data(), right.data(), bytes, mismatch_index);

   // A later difference must not be found instead.
   left[previous_index(bytes)] = cat::byte(0u);
   right[previous_index(bytes)] = cat::byte(255u);
   verify_large<Simd>(left.data(), right.data(), bytes, mismatch_index);
}

// Call `check` with SSE2 and AVX2 `Simd`s, each forced by a single `$abi`.
template <typename Check>
[[gnu::always_inline]]
inline void
for_each_simd(Check const& check) {
   $simd_switch($abi(sse2, { check.template operator()<char1x_>(); }));
   $simd_switch($abi(avx2, { check.template operator()<char1x_>(); }),
                default : break;);
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

$test(copy_memory_backward_small_overlap) {
   for (cat::idx bytes = 1u; bytes <= 127u; ++bytes) {
      overlap_case(bytes, 16u, 17u);
      overlap_case(bytes, 17u, 16u);
      overlap_case(bytes, 3u, 8u);
      overlap_case(bytes, 8u, 3u);
   }

   overlap_case(127u, 5u, 6u);
   overlap_case(128u, 5u, 6u);
   overlap_case(200u, 3u, 4u);
}

$test(copy_memory_overlap_large_non_temporal_tiers) {
   cat::idx const bytes = 2_umi + 257u;
   large_overlap_case(bytes, 512u, 128u);
   large_overlap_case(bytes, 128u, 517u);
}

static_assert([] {
   cat::array<unsigned char, 4u> const left = {1u, 2u, 3u, 4u};
   cat::array<unsigned char, 4u> const right = {1u, 2u, 9u, 0u};
   return cat::find_disjoint_memory(left.data(), right.data(), 4u) == 2u
          && cat::find_disjoint_memory(left.data(), left.data(), 4u) == 4u
          && cat::is_memory_equal(left.data(), left.data(), 4u)
          && !cat::is_memory_equal(left.data(), right.data(), 4u);
}());

$test(memory_comparison_edges) {
   cat::array<cat::byte, 4u> const left = {
      cat::byte(1u), cat::byte(2u), cat::byte(3u), cat::byte(4u)
   };
   cat::array<cat::byte, 4u> const right = {
      cat::byte(9u), cat::byte(9u), cat::byte(9u), cat::byte(9u)
   };

   // Zero bytes are equal whatever they point to.
   cat::verify(cat::find_disjoint_memory(left.data(), right.data(), 0u) == 0u);
   cat::verify(cat::is_memory_equal(left.data(), right.data(), 0u));
   cat::verify(
      cat::find_disjoint_memory_scalar(left.data(), right.data(), 0u) == 0u
   );
   cat::verify(cat::is_memory_equal_scalar(left.data(), right.data(), 0u));

   // The same range is equal to itself.
   cat::verify(cat::find_disjoint_memory(left.data(), left.data(), 4u) == 4u);
   cat::verify(cat::is_memory_equal(left.data(), left.data(), 4u));
}

$test(find_disjoint_memory) {
   using namespace cat::literals;

   {
      cat::array<cat::byte, 64> left{};
      cat::array<cat::byte, 64> right{};

      // Zero-length compare is equal regardless of contents.
      cat::verify(memory_order(left.data(), right.data(), 0u) == 0);

      initialize_bytes(left.data(), left.size(), cat::byte(0u));
      initialize_bytes(right.data(), right.size(), cat::byte(0u));
      cat::verify(memory_order(left.data(), right.data(), 64u) == 0);
      right[10] = cat::byte(240u);
      cat::verify(memory_order(left.data(), right.data(), 64u) < 0);
      right[10] = cat::byte(0u);
      cat::verify(memory_order(left.data(), right.data(), 64u) > 0);
   }

   // Large enough to run the SIMD pipeline in
   // `detail::find_disjoint_memory_impl` (`bytes` greater than eight native
   // `char1x_` vectors).
   {
      constexpr idx n = 2'000;
      cat::array<cat::byte, n> left{};
      cat::array<cat::byte, n> right{};
      initialize_bytes(left.data(), n);
      initialize_bytes(right.data(), n);
      cat::verify(memory_order(left.data(), right.data(), n) == 0);
      right[1'500] = cat::byte(0u);
      cat::verify(memory_order(left.data(), right.data(), n) > 0);
      right[1'500] = cat::byte(255u);
      cat::verify(memory_order(left.data(), right.data(), n) < 0);
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

   // Every position, so no byte between overlapping chunks goes unchecked.
   // Sizes past 255 run the large path's batches and tail.
   for (cat::idx bytes = 1u; bytes <= 600u; ++bytes) {
      for (cat::idx mismatch = 0u; mismatch < bytes; ++mismatch) {
         compare_mismatch_case(bytes, mismatch, mismatch % 2u == 0u);
      }
   }

   // Equal ranges at every size, from unaligned addresses.
   for (cat::idx bytes = 1u; bytes <= 300u; ++bytes) {
      compare_equal_case(bytes, 1u, 3u);
   }

   // Positions around the large path's batches and its overlapping tail.
   for (cat::idx bytes : {1'024_idx, 1'057_idx, 4'095_idx}) {
      for (cat::idx mismatch = 0u; mismatch < 160u; ++mismatch) {
         compare_mismatch_case(bytes, mismatch, true);
         compare_mismatch_case(
            bytes, cat::idx(previous_index(bytes) - mismatch), false
         );
      }
   }

   compare_equal_case(257u, 3u, 5u);
}

$test(compare_memory_large_non_temporal_size) {
   cat::idx const bytes = 2_umi + 129u;
   cat::span left = g_tests_alloc.alloc_multi<cat::byte>(bytes).or_exit();
   cat::span right = g_tests_alloc.alloc_multi<cat::byte>(bytes).or_exit();
   $defer {
      g_tests_alloc.free(left);
      g_tests_alloc.free(right);
   };

   initialize_bytes(left.data(), bytes);
   initialize_bytes(right.data(), bytes);
   cat::verify(memory_order(left.data(), right.data(), bytes) == 0);
   cat::idx const last = previous_index(bytes);
   left[last] = cat::byte(1u);
   right[last] = cat::byte(240u);
   cat::verify(memory_order(left.data(), right.data(), bytes) < 0);
}

// The large path with each SIMD ISA.
$test(compare_memory_direct_large_paths) {
   for_each_simd([]<typename Simd>() [[gnu::always_inline]] {
      // It needs at least four `Simd`s. Cover six batches and every tail.
      for (cat::idx bytes = 4u * sizeof(Simd); bytes <= 24u * sizeof(Simd);
           ++bytes) {
         // Equal ranges only depend on the size, so check them once here.
         cat::array<cat::byte, 1_uki> left;
         cat::array<cat::byte, 1_uki> right;
         initialize_bytes(left.data(), bytes);
         initialize_bytes(right.data(), bytes);
         verify_large<Simd>(left.data(), right.data(), bytes, cat::nullopt);
         for (cat::idx mismatch = 0u; mismatch < bytes; ++mismatch) {
            compare_large_case<Simd>(bytes, mismatch);
         }
      }
   });
}

// The SSE2 path that runs when AVX2 is unavailable.
$test(compare_memory_direct_medium_paths) {
   for (cat::idx bytes = 33u; bytes <= 127u; ++bytes) {
      for (cat::idx mismatch = 0u; mismatch < bytes; ++mismatch) {
         compare_medium_case(bytes, mismatch);
      }
   }
}

$test(compare_memory_direct_avx2_paths_when_supported) {
   if (cat::detail::simd_dispatch_priority < 80) {
      return;
   }

   for (cat::idx bytes = 17u; bytes <= 255u; ++bytes) {
      for (cat::idx mismatch = 0u; mismatch < bytes; ++mismatch) {
         compare_avx2_case(bytes, mismatch, mismatch % 2u == 0u);
      }
   }
}

namespace {

// Map a page between two inaccessible pages, and return its first byte.
// Every size up to 1100 covers every tier and tail length. Past that, sizes
// near each 64-byte multiple still reach both ends of a page.
auto
is_guarded_size(cat::idx bytes) -> bool {
   cat::idx const remainder = bytes % 64u;
   return bytes <= 1'100u || remainder <= 2u || remainder >= 62u;
}

auto
map_guarded_page() -> cat::byte* _Nonnull {
   cat::byte* const p_mapping =
      nix::sys_mmap(
         nullptr, 3u * cat::page_size, nix::memory_protection_flags::read_write,
         nix::memory_flags::privately | nix::memory_flags::anonymous,
         nix::invalid_file_descriptor, 0
      )
         .verify();
   nix::sys_mprotect(
      p_mapping, cat::page_size, nix::memory_protection_flags::none
   )
      .verify();
   nix::sys_mprotect(
      p_mapping + 2u * cat::page_size, cat::page_size,
      nix::memory_protection_flags::none
   )
      .verify();
   return p_mapping + cat::page_size;
}

// Run every comparison over `[p_left, p_left + bytes)` and
// `[p_right, p_right + bytes)`, which hold equal bytes.
void
verify_in_bounds(
   cat::byte* _Nonnull p_left, cat::byte* _Nonnull p_right, cat::idx bytes
) {
   using cat::detail::memory_scan;
   bool const has_avx2 = cat::detail::simd_dispatch_priority >= 80;

   auto const verify_tiers = [&](cat::maybe<cat::idx> mismatch) {
      if (bytes >= 33u && bytes <= 127u) {
         cat::verify(
            cat::detail::scan_memory_medium<memory_scan::equality>(
               p_left, p_right, bytes
            )
            == mismatch.has_value()
         );
         cat::verify(
            cat::detail::scan_memory_medium<memory_scan::mismatch>(
               p_left, p_right, bytes
            )
            == mismatch
         );
      }

      if (has_avx2 && bytes >= 17u && bytes <= 255u) {
         cat::verify(
            cat::detail::scan_memory_avx2<memory_scan::equality>(
               p_left, p_right, bytes
            )
            == mismatch.has_value()
         );
         cat::verify(
            cat::detail::scan_memory_avx2<memory_scan::mismatch>(
               p_left, p_right, bytes
            )
            == mismatch
         );
      }

      for_each_simd([&]<typename Simd>() [[gnu::always_inline]] {
         if (bytes >= 4u * sizeof(Simd)) {
            verify_large<Simd>(p_left, p_right, bytes, mismatch);
         }
      });
   };

   cat::verify(cat::find_disjoint_memory(p_left, p_right, bytes) == bytes);
   cat::verify(
      cat::find_disjoint_memory_scalar(p_left, p_right, bytes) == bytes
   );
   cat::verify(cat::is_memory_equal(p_left, p_right, bytes));
   cat::verify(cat::is_memory_equal_scalar(p_left, p_right, bytes));
   verify_tiers(cat::nullopt);

   if (bytes == 0u) {
      return;
   }

   // A difference in the first or last byte, next to an inaccessible page.
   for (cat::idx const position : {0_idx, previous_index(bytes)}) {
      cat::byte const original = p_right[position];
      p_right[position] = cat::byte(original.value ^ 1u);
      cat::verify(
         cat::find_disjoint_memory(p_left, p_right, bytes) == position
      );
      cat::verify(!cat::is_memory_equal(p_left, p_right, bytes));
      verify_tiers(position);
      p_right[position] = original;
   }
}

}  // namespace

// Each range starts right after or ends right before an inaccessible page, so
// reading outside it segfaults.
$test(memory_comparison_stays_in_bounds) {
   cat::byte* _Nonnull const p_left_page = map_guarded_page();
   cat::byte* _Nonnull const p_right_page = map_guarded_page();
   $defer {
      nix::sys_munmap(p_left_page - cat::page_size, 3u * cat::page_size)
         .verify();
      nix::sys_munmap(p_right_page - cat::page_size, 3u * cat::page_size)
         .verify();
   };

   for (cat::idx bytes = 0u; bytes <= cat::page_size; ++bytes) {
      if (!is_guarded_size(bytes)) {
         continue;
      }

      for (bool const is_left_at_end : {false, true}) {
         for (bool const is_right_at_end : {false, true}) {
            cat::byte* _Nonnull const p_left =
               is_left_at_end ? p_left_page + cat::page_size - bytes
                              : p_left_page;
            cat::byte* _Nonnull const p_right =
               is_right_at_end ? p_right_page + cat::page_size - bytes
                               : p_right_page;
            initialize_bytes(p_left, bytes);
            initialize_bytes(p_right, bytes);
            verify_in_bounds(p_left, p_right, bytes);
         }
      }
   }
}

namespace {

// Bytes around a destination that a copy must not change.
inline constexpr cat::byte copy_canary = cat::byte(0xa5u);
inline constexpr cat::idx copy_margin = 64u;

// Copy `bytes` between buffers skewed by `source_skew` and `destination_skew`
// with `copy`, and verify the destination and the bytes around it.
template <typename Copy>
[[gnu::always_inline]]
inline void
verify_copy_with(
   Copy const& copy, cat::idx bytes, cat::idx source_skew,
   cat::idx destination_skew
) {
   cat::array<cat::byte, 4_uki> source;
   cat::array<cat::byte, 4_uki> destination;
   initialize_bytes(source.data(), source_skew + bytes);
   fill_bytes(
      destination.data(), destination_skew + bytes + copy_margin, copy_canary
   );

   copy(
      source.data() + source_skew, destination.data() + destination_skew, bytes
   );

   for (cat::idx i = 0u; i < destination_skew; ++i) {
      cat::verify(destination[i] == copy_canary);
   }

   verify_copy_result(
      source.data() + source_skew, destination.data() + destination_skew, bytes
   );
   for (cat::idx i = 0u; i < copy_margin; ++i) {
      cat::verify(destination[destination_skew + bytes + i] == copy_canary);
   }
}

void
verify_copy(cat::idx bytes, cat::idx source_skew, cat::idx destination_skew) {
   verify_copy_with(
      [](cat::byte const* p_source, cat::byte* p_destination, cat::idx size) {
         cat::copy_memory(p_source, p_destination, size);
      },
      bytes, source_skew, destination_skew
   );
   verify_copy_with(
      [](cat::byte const* p_source, cat::byte* p_destination, cat::idx size) {
         cat::copy_memory_scalar(p_source, p_destination, size);
      },
      bytes, source_skew, destination_skew
   );
}

// Move `bytes` from `source_offset` to `destination_offset` within one buffer
// with `move`, and verify the whole buffer.
template <typename Move>
[[gnu::always_inline]]
inline void
verify_move_with(
   Move const& move, cat::idx bytes, cat::idx source_offset,
   cat::idx destination_offset
) {
   cat::idx const size =
      max(source_offset, destination_offset) + bytes + copy_margin;
   cat::array<cat::byte, 4_uki> buffer;
   cat::array<cat::byte, 4_uki> expected;
   initialize_bytes(buffer.data(), size);
   initialize_bytes(expected.data(), size);
   for (cat::idx i = 0u; i < bytes; ++i) {
      expected[destination_offset + i] = buffer[source_offset + i];
   }

   move(
      buffer.data() + source_offset, buffer.data() + destination_offset, bytes
   );
   verify_buffer_result(expected.data(), buffer.data(), size);
}

void
verify_move(
   cat::idx bytes, cat::idx source_offset, cat::idx destination_offset
) {
   verify_move_with(
      [](cat::byte const* p_source, cat::byte* p_destination, cat::idx size) {
         cat::copy_memory_backward(p_source, p_destination, size);
      },
      bytes, source_offset, destination_offset
   );
   verify_move_with(
      [](cat::byte const* p_source, cat::byte* p_destination, cat::idx size) {
         cat::copy_memory_backward_scalar(p_source, p_destination, size);
      },
      bytes, source_offset, destination_offset
   );
}

// Overlap distances around every chunk width the copies use.
inline constexpr cat::array move_distances = {
   1_idx,   2_idx,   3_idx,   4_idx,   7_idx,   8_idx,   9_idx,   15_idx,
   16_idx,  17_idx,  31_idx,  32_idx,  33_idx,  63_idx,  64_idx,  65_idx,
   127_idx, 128_idx, 129_idx, 255_idx, 256_idx, 257_idx, 511_idx, 512_idx,
};

// Move `bytes` by every distance in both directions, from varied alignments.
void
verify_moves(cat::idx bytes) {
   for (cat::idx const distance : move_distances) {
      cat::idx const base = distance % 13u;
      verify_move(bytes, base, base + distance);
      verify_move(bytes, base + distance, base);
   }

   // Moves that overlap by exactly one byte, or not at all.
   if (bytes != 0u) {
      verify_move(bytes, 5u, 5u + previous_index(bytes));
      verify_move(bytes, 5u + previous_index(bytes), 5u);
   }

   verify_move(bytes, 5u, 5u + bytes);
   verify_move(bytes, 5u + bytes, 5u);
   verify_move(bytes, 5u, 5u);
}

// Run `check` with and without `rep movsb`, so both copy paths are tested.
template <typename Check>
void
for_each_rep_string_support(Check const& check) {
   auto const saved = cat::detail::memory_rep_string_support;
   $defer {
      cat::detail::memory_rep_string_support = saved;
   };

   cat::detail::memory_rep_string_support = {};
   check();
   cat::detail::memory_rep_string_support = {
      .has_erms = true, .has_fsrm = true
   };
   check();
}

}  // namespace

$test(copy_memory_every_size) {
   for_each_rep_string_support([] {
      for (cat::idx bytes = 0u; bytes <= 1'100u; ++bytes) {
         verify_copy(bytes, 0u, 0u);
         verify_copy(bytes, 1u, 0u);
         verify_copy(bytes, 0u, 1u);
         verify_copy(bytes, 3u, 17u);
         verify_copy(bytes, 15u, 16u);
         verify_copy(bytes, 31u, 1u);
      }
   });
}

$test(copy_memory_every_skew) {
   for_each_rep_string_support([] {
      for (cat::idx bytes :
           {127_idx, 128_idx, 255_idx, 256_idx, 1'023_idx, 1'024_idx}) {
         for (cat::idx source_skew = 0u; source_skew <= 32u; ++source_skew) {
            for (cat::idx destination_skew = 0u; destination_skew <= 32u;
                 ++destination_skew) {
               verify_copy(bytes, source_skew, destination_skew);
            }
         }
      }
   });
}

$test(copy_memory_backward_every_size_and_distance) {
   for (cat::idx bytes = 0u; bytes <= 1'100u; ++bytes) {
      verify_moves(bytes);
   }
}

// `copy_memory_large` and `copy_memory_backward_large` with each SIMD ISA.
$test(copy_memory_direct_large_paths) {
   for_each_simd([]<typename Simd>() [[gnu::always_inline]] {
      auto const forward =
         [](cat::byte const* p_source, cat::byte* p_destination, cat::idx size)
            [[gnu::always_inline]]
      {
         cat::detail::copy_memory_large<Simd>(p_source, p_destination, size);
      };
      auto const backward =
         [](cat::byte const* p_source, cat::byte* p_destination, cat::idx size)
            [[gnu::always_inline]]
      {
         cat::detail::copy_memory_backward_large<Simd>(
            p_source, p_destination, size
         );
      };

      // `copy_memory_large` first copies up to one `Simd` of alignment padding.
      for (cat::idx bytes = 64u; bytes <= 1'100u; ++bytes) {
         verify_copy_with(forward, bytes, 0u, 0u);
         verify_copy_with(forward, bytes, 3u, 17u);
         verify_copy_with(backward, bytes, 0u, 0u);
         verify_copy_with(backward, bytes, 17u, 3u);
         for (cat::idx const distance : {1_idx, 33_idx, 300_idx}) {
            cat::idx const base = distance % 13u;
            verify_move_with(forward, bytes, base + distance, base);
            verify_move_with(backward, bytes, base, base + distance);
         }
      }
   });
}

// Copies and moves past 2 MiB use non-temporal stores.
$test(copy_memory_non_temporal_sizes) {
   for_each_rep_string_support([] {
      cat::idx const bytes = 2_umi + 129u;
      cat::span source =
         g_tests_alloc.alloc_multi<cat::byte>(bytes + 64u).or_exit();
      cat::span destination =
         g_tests_alloc.alloc_multi<cat::byte>(bytes + 64u).or_exit();
      $defer {
         g_tests_alloc.free(source);
         g_tests_alloc.free(destination);
      };

      initialize_bytes(source.data(), source.size());
      for (cat::idx const skew : {0_idx, 1_idx, 31_idx}) {
         fill_bytes(destination.data(), destination.size(), copy_canary);
         cat::copy_memory(source.data() + 7u, destination.data() + skew, bytes);
         verify_copy_result(
            source.data() + 7u, destination.data() + skew, bytes
         );
         cat::verify(destination[skew + bytes] == copy_canary);
      }
   });

   for (cat::idx const distance : {1_idx, 31_idx, 32_idx, 33_idx, 4'096_idx}) {
      large_overlap_case(2_umi + 129u, distance, 0u);
      large_overlap_case(2_umi + 129u, 0u, distance);
   }
}

namespace {

// Copy and move within guarded pages, so that touching a byte outside a range
// segfaults. `p_source_page` and `p_destination_page` are guarded pages.
void
verify_copies_in_bounds(
   cat::byte* _Nonnull p_source_page, cat::byte* _Nonnull p_destination_page,
   cat::idx bytes
) {
   for (bool const is_source_at_end : {false, true}) {
      for (bool const is_destination_at_end : {false, true}) {
         cat::byte* _Nonnull const p_source =
            is_source_at_end ? p_source_page + cat::page_size - bytes
                             : p_source_page;
         cat::byte* _Nonnull const p_destination =
            is_destination_at_end ? p_destination_page + cat::page_size - bytes
                                  : p_destination_page;
         initialize_bytes(p_source, bytes);
         for_each_rep_string_support([&] {
            fill_bytes(p_destination, bytes, copy_canary);
            cat::copy_memory(p_source, p_destination, bytes);
            verify_copy_result(p_source, p_destination, bytes);
         });
         fill_bytes(p_destination, bytes, copy_canary);
         cat::copy_memory_backward(p_source, p_destination, bytes);
         verify_copy_result(p_source, p_destination, bytes);

         if (bytes >= 64u) {
            for_each_simd([&]<typename Simd>() [[gnu::always_inline]] {
               fill_bytes(p_destination, bytes, copy_canary);
               cat::detail::copy_memory_large<Simd>(
                  p_source, p_destination, bytes
               );
               verify_copy_result(p_source, p_destination, bytes);
               fill_bytes(p_destination, bytes, copy_canary);
               cat::detail::copy_memory_backward_large<Simd>(
                  p_source, p_destination, bytes
               );
               verify_copy_result(p_source, p_destination, bytes);
            });
         }
      }
   }

   // Overlapping moves whose ranges start or end at a guard.
   for (cat::idx const distance : {1_idx, 33_idx, 300_idx}) {
      if (bytes + distance > cat::page_size) {
         continue;
      }

      for (bool const is_at_end : {false, true}) {
         cat::byte* _Nonnull const p_low =
            is_at_end ? p_source_page + cat::page_size - bytes - distance
                      : p_source_page;
         for (bool const is_forward : {false, true}) {
            cat::byte* _Nonnull const p_source =
               is_forward ? p_low + distance : p_low;
            cat::byte* _Nonnull const p_destination =
               is_forward ? p_low : p_low + distance;
            initialize_bytes(p_low, bytes + distance);
            cat::array<cat::byte, 4_uki> expected{};
            for (cat::idx i = 0u; i < bytes; ++i) {
               expected[i] = p_source[i];
            }

            cat::copy_memory_backward(p_source, p_destination, bytes);
            verify_copy_result(expected.data(), p_destination, bytes);
         }
      }
   }
}

}  // namespace

$test(copy_memory_stays_in_bounds) {
   cat::byte* _Nonnull const p_source_page = map_guarded_page();
   cat::byte* _Nonnull const p_destination_page = map_guarded_page();
   $defer {
      nix::sys_munmap(p_source_page - cat::page_size, 3u * cat::page_size)
         .verify();
      nix::sys_munmap(p_destination_page - cat::page_size, 3u * cat::page_size)
         .verify();
   };

   for (cat::idx bytes = 0u; bytes <= cat::page_size; ++bytes) {
      if (is_guarded_size(bytes)) {
         verify_copies_in_bounds(p_source_page, p_destination_page, bytes);
      }
   }
}
