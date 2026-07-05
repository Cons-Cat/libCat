#include <cat/detail/movsb.hpp>

#include <cat/cpuid>
#include <cat/memory>
#include <cat/page_allocator>

#include "../unit_tests.hpp"

namespace {

auto
previous_index(cat::idx index) -> cat::idx {
   cat::iword const previous = index - 1u;
   return previous.to_idx().assert();
}

void
verify_filled(unsigned char const* _Nonnull p_data, cat::idx bytes,
              unsigned char value) {
   for (cat::idx i = 0u; i < bytes; ++i) {
      cat::verify(p_data[i] == value);
   }
}

void
fill_case(cat::idx bytes, cat::idx skew, unsigned char value) {
   cat::array<unsigned char, 8_uki> buffer{};
   cat::fill_memory(buffer.data(), 0xA5_u1, buffer.size());

   cat::fill_memory(buffer.data() + skew, value, bytes);
   verify_filled(buffer.data() + skew, bytes, value);

   if (skew > 0u) {
      cat::verify(buffer[previous_index(skew)] == 0xA5_u1);
   }
   cat::verify(buffer[skew + bytes] == 0xA5_u1);
}

void
large_fill_case(cat::idx bytes, cat::idx skew, unsigned char value) {
   cat::page_allocator pager;
   cat::idx const capacity = bytes + skew + 256u;
   cat::span page = pager.alloc_multi<unsigned char>(capacity).or_exit();
   $defer {
      pager.free(page);
   };

   cat::fill_memory(page.data(), 0xA5_u1, capacity);
   cat::fill_memory(page.data() + skew, value, bytes);
   verify_filled(page.data() + skew, bytes, value);

   if (skew > 0u) {
      cat::verify(page[previous_index(skew)] == 0xA5_u1);
   }
   cat::verify(page[skew + bytes] == 0xA5_u1);
}

}  // namespace

$test(fill_memory) {
   using namespace cat::arithmetic_literals;

   // Small memset sizes exercise `fill_memory_small`.
   {
      cat::array<unsigned char, 128> buf{};
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
           }) {
         cat::fill_memory(buf.data(), 9_u1, size);
         for (idx i = 0u; i < size; ++i) {
            cat::verify(buf[i] == 9_u1);
         }
      }
   }

   cat::page_allocator allocator;
   cat::span page = allocator.alloc_multi<uint1>(4_uki).or_exit();
   uint1* p_page = page.data();

   cat::fill_memory(p_page, 1_u1, 4_uki);
   cat::verify(p_page[1'000] == 1_u1);

   // Test that unaligned memory still fills correctly.
   cat::fill_memory(p_page + 3, 2_u1, 2_uki - 6u);
   cat::verify(p_page[0] == 1_u1);
   cat::verify(p_page[2] == 1_u1);
   cat::verify(p_page[3] == 2_u1);
   cat::verify(p_page[2_uki - 4u] == 2_u1);
   cat::verify(p_page[2_uki - 3u] == 1_u1);

   // Test zeroing out memory.
   cat::zero_memory(p_page, 4_uki);
   cat::verify(p_page[0] == 0_u1);
   cat::verify(p_page[4_uki - 1u] == 0_u1);

   // Test filling values larger than 1-byte.
   cat::fill_memory(p_page, 1_i2, 2_uki);
   cat::verify(__builtin_bit_cast(int2*, p_page)[10] == 1_i2);
   // The next byte after this should be 0.
   cat::verify(__builtin_bit_cast(int1*, p_page)[21] == 0);

   cat::fill_memory(p_page, 1_i4, 1_uki);
   cat::verify(__builtin_bit_cast(int4*, p_page)[10] == 1_i4);

   cat::fill_memory(p_page, 1_i8, 0.5_uki);
   cat::verify(__builtin_bit_cast(int4*, p_page)[10] == 1_i4);

   // Test scalar `fill_memory()`.
   cat::fill_memory_scalar(p_page, 1_u1, 4_uki);
   cat::verify(p_page[1'001] == 1_u1);

   // Test scalar `zero_memory()`.
   cat::zero_memory_scalar(p_page, 4_uki);
   cat::verify(p_page[1'001] == 0_u1);
};

$test(fill_memory_all_size_tiers_and_skews) {
   auto const saved_rep_support = cat::detail::memory_rep_string_support;
   $defer {
      cat::detail::memory_rep_string_support = saved_rep_support;
   };
   cat::detail::memory_rep_string_support = {};

   for (cat::idx bytes : {
           33_idx,
           63_idx,
           64_idx,
           65_idx,
           127_idx,
           128_idx,
           129_idx,
           511_idx,
           512_idx,
           1'024_idx,
           2'047_idx,
           2'048_idx,
           2'111_idx,
        }) {
      fill_case(bytes, 0u, 0x33u);
      fill_case(bytes, 1u, 0x44u);
      fill_case(bytes, 17u, 0x55u);
   }
}

$test(fill_memory_rep_stosb_path) {
   auto const saved_rep_support = cat::detail::memory_rep_string_support;
   $defer {
      cat::detail::memory_rep_string_support = saved_rep_support;
   };
   cat::detail::memory_rep_string_support = {
      .has_erms = true,
      .has_fsrm = false,
   };

   fill_case(2'048u, 9u, 0x66u);
}

$test(fill_memory_direct_stosb_helper) {
   cat::array<unsigned char, 160> buffer{};
   x64::stosb(buffer.data(), cat::byte(0x77u), buffer.size());
   verify_filled(buffer.data(), buffer.size(), 0x77u);
}

$test(fill_memory_large_non_temporal_tiers) {
   auto const saved_rep_support = cat::detail::memory_rep_string_support;
   $defer {
      cat::detail::memory_rep_string_support = saved_rep_support;
   };
   cat::detail::memory_rep_string_support = {};

   cat::idx const bytes = 2_umi + 257u;
   large_fill_case(bytes, 0u, 0x88u);
   large_fill_case(bytes, 3u, 0x99u);
}
