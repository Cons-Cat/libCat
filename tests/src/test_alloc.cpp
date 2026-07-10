#include <cat/bit>
#include <cat/linear_allocator>
#include <cat/page_allocator>

#include "../unit_tests.hpp"

struct alloc_huge_object {
   [[maybe_unused]]
   uint1 storage[64 + 1];
};

constinit int4 alloc_counter = 0;  // NOLINT

struct alloc_non_trivial {
   char storage;

   alloc_non_trivial() {
      ++alloc_counter;
   }
};

struct alloc_non_trivial_huge_object {
   [[maybe_unused]]
   uint1 storage[64];

   alloc_non_trivial_huge_object() {
      ++alloc_counter;
   }
};

consteval void
const_test() {
   int4* p_alloc = pager.alloc<int4>(1).value();
   pager.free(p_alloc);

   int4* p_xalloc = pager.xalloc<int4>(1);
   pager.free(p_xalloc);

   cat::span<int4> alloc_multi = pager.alloc_multi<int4>(5u).value();
   // `realloc_multi` is not a constant expression on Clang.
   pager.free_multi(alloc_multi);

   cat::span<int4> xalloc_multi = pager.xalloc_multi<int4>(5u);
   pager.free_multi(xalloc_multi);
}

consteval void
const_test_inline_alloc() {
   auto h_alloc = pager.inline_alloc<int4>(1).value();
   pager.free(h_alloc);

   auto hx = pager.inline_xalloc<int4>(1);
   pager.free(hx);

   // Inline multi uses placement `new` over the handle object, which is not a
   // constant expression on Clang.
}

// Smoke test that the SBO size template parameter on `inline_*` is usable in a
// constant-evaluation context.
consteval void
const_test_inline_alloc_sbo16() {
   auto h_alloc = pager.inline_alloc<int4, 16u>(1).value();
   pager.free(h_alloc);

   auto hx = pager.inline_xalloc<int4, 16u>(1);
   pager.free(hx);
}

$test(alloc) {
   const_test();
   const_test_inline_alloc();

   // Initialize an allocator.
   pager.reset();
   // Page the kernel for a linear allocator to $test with.
   cat::span page = pager.alloc_multi<cat::byte>(4_uki - 64u).or_exit();
   $defer {
      pager.free(page);
   };
   auto allocator = cat::make_linear_allocator(page);

   // Test `alloc`.
   auto* _ = allocator.alloc<int4>().value();
   auto* p_alloc = allocator.alloc<int4>(1).value();
   cat::verify(*p_alloc == 1);

   // Test `xalloc`.
   auto* _ = allocator.xalloc<int4>();
   auto* p_xalloc = allocator.xalloc<int4>(1);
   cat::verify(*p_xalloc == 1);

   // Test `alloc_multi`.
   cat::span<int4> alloc_multi = allocator.alloc_multi<int4>(5u).value();
   cat::verify(alloc_multi.size() == 5);

   // Test `xalloc_multi`.
   alloc_multi = allocator.xalloc_multi<int4>(5u);
   cat::verify(alloc_multi.size() == 5);

   // Test `align_alloc`.
   auto* _ = allocator.align_alloc<int4>(8u).value();
   auto* p_align_alloc = allocator.align_alloc<int4>(8u, 1).value();
   cat::verify(*p_align_alloc == 1);
   cat::verify(cat::is_aligned(p_align_alloc, 8u));

   // Test `align_xalloc`.
   auto* _ = allocator.align_xalloc<int4>(8u);
   auto* p_align_xalloc = allocator.align_xalloc<int4>(8u, 1);
   cat::verify(*p_align_xalloc == 1);
   cat::verify(cat::is_aligned(p_align_xalloc, 8u));

   // Test `unalign_alloc`.
   auto* _ = allocator.unalign_alloc<int4>(8u).value();
   auto* p_unalign_alloc = allocator.unalign_alloc<int4>(1).value();
   cat::verify(*p_unalign_alloc == 1);

   // Test `unalign_xalloc`.
   auto* _ = allocator.unalign_xalloc<int4>(8u);
   auto* p_unalign_xalloc = allocator.unalign_xalloc<int4>(1);
   cat::verify(*p_unalign_xalloc == 1);

   // Test `align_alloc_multi`.
   auto* p_align_alloc_multi =
      allocator.align_alloc_multi<int4>(8u, 5u).value().data();
   cat::verify(cat::is_aligned(p_align_alloc_multi, 8u));
   alloc_counter = 0;
   auto _ = allocator.align_alloc_multi<alloc_non_trivial>(8u, 5u);
   cat::verify(alloc_counter == 5);

   // Test `align_xalloc_multi`.
   auto _ = allocator.align_xalloc_multi<int4>(8u, 5u);
   alloc_counter = 0;
   auto _ = allocator.align_xalloc_multi<alloc_non_trivial>(8u, 5u);
   cat::verify(alloc_counter == 5);

   // Test `unalign_alloc_multi`.
   auto _ = allocator.unalign_alloc_multi<int1>(5u)
               .value();  // `int4` is 4-byte aligned.
   alloc_counter = 0;
   auto _ = allocator.unalign_alloc_multi<alloc_non_trivial>(5u);
   cat::verify(alloc_counter == 5);

   // Test `unalign_xalloc_multi`.
   auto _ =
      allocator.unalign_xalloc_multi<int1>(5u);  // `int4` is 4-byte aligned.
   alloc_counter = 0;
   auto _ = allocator.unalign_xalloc_multi<alloc_non_trivial>(5u);
   cat::verify(alloc_counter == 5);

   // Test `inline_alloc`.
   auto _ = allocator.inline_alloc<int4>().value();
   auto inline_alloc = allocator.inline_alloc<int4>(1).value();
   cat::verify(allocator.get(inline_alloc) == 1);
   cat::verify(inline_alloc.is_inline());
   alloc_counter = 0;
   auto _ = allocator.inline_alloc<alloc_non_trivial>();
   cat::verify(alloc_counter == 1);

   // `alloc_huge_object` is larger than the inline buffer.
   auto inline_alloc_2 = allocator.inline_alloc<alloc_huge_object>().value();
   cat::verify(!inline_alloc_2.is_inline());

   alloc_counter = 0;
   auto _ = allocator.inline_alloc<alloc_non_trivial_huge_object>();
   cat::verify(alloc_counter == 1);

   // Test `inline_xalloc`.
   auto _ = allocator.inline_xalloc<int4>();
   auto inline_xalloc = allocator.inline_xalloc<int4>(1);
   cat::verify(allocator.get(inline_xalloc) == 1);

   // Test `inline_alloc_multi`.
   auto inline_alloc_multi = allocator.inline_alloc_multi<int4>(5u).value();
   cat::verify(inline_alloc_multi.size() == 5);
   alloc_counter = 0;
   auto _ = allocator.inline_alloc_multi<alloc_non_trivial>(5u);
   cat::verify(alloc_counter == 5);

   // Test `inline_xalloc_multi`.
   auto inline_xalloc_multi = allocator.inline_xalloc_multi<int4>(5u);
   cat::verify(inline_xalloc_multi.size() == 5);
   alloc_counter = 0;
   auto _ = allocator.inline_xalloc_multi<alloc_non_trivial>(5u);
   cat::verify(alloc_counter == 5);

   // Test `inline_align_alloc`.
   auto _ = allocator.inline_align_alloc<int4>(8u).value();
   auto inline_align_alloc = allocator.inline_align_alloc<int4>(8u, 1).value();
   cat::verify(allocator.get(inline_align_alloc) == 1);
   cat::verify(cat::is_aligned(&allocator.get(inline_align_alloc), 8u));
   cat::verify(inline_align_alloc.is_inline());

   // Test `inline_unalign_alloc`.
   auto _ = allocator.inline_unalign_alloc<int4>(8u).value();
   auto inline_unalign_alloc = allocator.inline_unalign_alloc<int4>(1).value();
   cat::verify(allocator.get(inline_unalign_alloc) == 1);
   cat::verify(inline_unalign_alloc.is_inline());

   // Test `inline_unalign_xalloc`.
   auto _ = allocator.inline_unalign_xalloc<int4>(8u);
   auto inline_unalign_xalloc = allocator.inline_unalign_xalloc<int4>(1);
   cat::verify(allocator.get(inline_unalign_xalloc) == 1);
   cat::verify(inline_unalign_xalloc.is_inline());

   allocator.reset();

   // Test `inline_align_alloc_multi`.
   auto inline_align_alloc_multi =
      allocator.inline_align_alloc_multi<int4>(8u, 5u).value();
   cat::verify(
      cat::is_aligned(allocator.get_ptr(inline_align_alloc_multi), 8u)
   );
   cat::verify(inline_align_alloc_multi.is_inline());

   auto inline_align_alloc_multi_big =
      allocator.inline_align_alloc_multi<int4>(8u, 64u).value();
   cat::verify(!inline_align_alloc_multi_big.is_inline());

   // Test `inline_align_xalloc_multi`.
   auto inline_align_xalloc_multi =
      allocator.inline_align_xalloc_multi<int4>(8u, 5u);
   cat::verify(
      cat::is_aligned(allocator.get_ptr(inline_align_xalloc_multi), 8u)
   );
   cat::verify(inline_align_xalloc_multi.is_inline());

   // Test `inline_unalign_alloc_multi`.
   auto inline_unalign_alloc_multi =
      allocator.inline_unalign_alloc_multi<int4>(5u).value();
   cat::verify(inline_unalign_alloc_multi.is_inline());

   auto inline_unalign_alloc_multi_big =
      allocator.inline_unalign_alloc_multi<int4>(64u).value();
   cat::verify(!inline_unalign_alloc_multi_big.is_inline());

   // Test `inline_unalign_xalloc_multi`.
   auto inline_unalign_xalloc_multi =
      allocator.inline_unalign_xalloc_multi<int4>(5u);
   cat::verify(inline_unalign_xalloc_multi.is_inline());

   auto inline_unalign_xalloc_multi_big =
      allocator.inline_unalign_xalloc_multi<int4>(64u);
   cat::verify(!inline_unalign_xalloc_multi_big.is_inline());

   // Test `inline_nalloc`.
   allocator.reset();
   iword inline_nalloc = allocator.inline_nalloc<int4>().value();
   cat::verify(inline_nalloc == 64u);
   iword inline_nalloc_big =
      allocator.inline_nalloc<alloc_huge_object>().value();
   cat::verify(inline_nalloc_big == 64u + 1);

   // Test `inline_xnalloc`.
   allocator.reset();
   iword inline_xnalloc = allocator.inline_xnalloc<int4>();
   cat::verify(inline_xnalloc == 64u);
   iword inline_xnalloc_big = allocator.inline_xnalloc<alloc_huge_object>();
   cat::verify(inline_xnalloc_big == 64u + 1);

   // Test `inline_nalloc_multi`.
   allocator.reset();
   iword inline_nalloc_multi = allocator.inline_nalloc_multi<int4>(5u).value();
   cat::verify(inline_nalloc_multi == 64u);
   iword inline_nalloc_multi_big =
      allocator.inline_nalloc_multi<alloc_huge_object>(2u).value();
   cat::verify(inline_nalloc_multi_big == ((64u + 1) * 2));

   // Test `inline_xnalloc_multi`.
   allocator.reset();
   iword inline_xnalloc_multi = allocator.inline_xnalloc_multi<int4>(5u);
   cat::verify(inline_xnalloc_multi == 64u);
   iword inline_xnalloc_multi_big =
      allocator.inline_xnalloc_multi<alloc_huge_object>(2u);
   cat::verify(inline_xnalloc_multi_big == ((64u + 1) * 2));

   // Test `inline_align_nalloc`.
   allocator.reset();
   iword inline_align_nalloc = allocator.inline_align_nalloc<int4>(4u).value();
   cat::verify(inline_align_nalloc == 64u);
   iword inline_align_nalloc_big =
      allocator.inline_align_nalloc<alloc_huge_object>(1u).value();
   cat::verify(inline_align_nalloc_big == 64u + 1);

   // Test `inline_align_xnalloc`.
   allocator.reset();
   iword inline_align_xnalloc = allocator.inline_align_xnalloc<int4>(4u);
   cat::verify(inline_align_xnalloc == 64u);
   iword inline_align_xnalloc_big =
      allocator.inline_align_xnalloc<alloc_huge_object>(1u);
   cat::verify(inline_align_xnalloc_big == 64u + 1);

   // Test `inline_unalign_nalloc`.
   allocator.reset();
   iword inline_unalign_nalloc =
      allocator.inline_unalign_nalloc<int4>().value();
   cat::verify(inline_unalign_nalloc == 64u);
   iword inline_unalign_nalloc_big =
      allocator.inline_unalign_nalloc<alloc_huge_object>().value();
   cat::verify(inline_unalign_nalloc_big == 64u + 1);

   // Test `inline_unalign_xnalloc`.
   allocator.reset();
   iword inline_unalign_xnalloc = allocator.inline_unalign_xnalloc<int4>();
   cat::verify(inline_unalign_xnalloc == 64u);
   iword inline_unalign_xnalloc_big =
      allocator.inline_unalign_xnalloc<alloc_huge_object>();
   cat::verify(inline_unalign_xnalloc_big == 64u + 1);

   // Test `inline_align_nalloc_multi`.
   allocator.reset();
   iword inline_align_nalloc_multi =
      allocator.inline_align_nalloc_multi<int4>(4u, 5u).value();
   cat::verify(inline_align_nalloc_multi == 64u);
   iword inline_align_nalloc_multi_big =
      allocator.inline_align_nalloc_multi<alloc_huge_object>(1u, 2u).value();
   cat::verify(inline_align_nalloc_multi_big == ((64u + 1) * 2));

   // Test `inline_align_xnalloc_multi`.
   allocator.reset();
   iword inline_align_xnalloc_multi =
      allocator.inline_align_xnalloc_multi<int4>(4u, 5u);
   cat::verify(inline_align_xnalloc_multi == 64u);
   iword inline_align_xnalloc_multi_big =
      allocator.inline_align_xnalloc_multi<alloc_huge_object>(1u, 2u);
   cat::verify(inline_align_xnalloc_multi_big == ((64u + 1) * 2));

   // Test `inline_unalign_nalloc_multi`.
   allocator.reset();
   iword inline_unalign_nalloc_multi =
      allocator.inline_unalign_nalloc_multi<int4>(5u).value();
   cat::verify(inline_unalign_nalloc_multi == 64u);
   iword inline_unalign_nalloc_multi_big =
      allocator.inline_unalign_nalloc_multi<alloc_huge_object>(2u).value();
   cat::verify(inline_unalign_nalloc_multi_big == ((64u + 1) * 2));

   // Test `inline_unalign_xnalloc_multi`.
   allocator.reset();
   iword inline_unalign_xnalloc_multi =
      allocator.inline_unalign_xnalloc_multi<int4>(5u);
   cat::verify(inline_unalign_xnalloc_multi == 64u);
   iword inline_unalign_xnalloc_multi_big =
      allocator.inline_unalign_xnalloc_multi<alloc_huge_object>(2u);
   cat::verify(inline_unalign_xnalloc_multi_big == ((64u + 1) * 2));

   alloc_counter = 0;
   auto _ = allocator.salloc_multi<alloc_non_trivial>(5u);
   cat::verify(alloc_counter == 5);

   alloc_counter = 0;
   auto _ = allocator.xsalloc_multi<alloc_non_trivial>(5u);
   cat::verify(alloc_counter == 5);

   // Test `align_salloc`.
   auto _ = allocator.align_salloc<int4>(8u);
   allocator.reset();
   auto [p_align_salloc, p_align_salloc_bytes] =
      allocator.align_salloc<int4>(8u, 1).value();
   cat::verify(*p_align_salloc == 1);
   cat::verify(p_align_salloc_bytes == 8);
   cat::verify(cat::is_aligned(p_align_salloc, 8u));

   alloc_counter = 0;
   auto _ = allocator.align_salloc<alloc_non_trivial>(8u);
   cat::verify(alloc_counter == 1);

   // Test `align_xsalloc`.
   auto _ = allocator.align_xsalloc<int4>(8u);
   allocator.reset();
   auto [p_align_xsalloc, p_align_xsalloc_bytes] =
      allocator.align_xsalloc<int4>(8u, 1);
   cat::verify(*p_align_xsalloc == 1);
   cat::verify(p_align_xsalloc_bytes == 8);
   cat::verify(cat::is_aligned(p_align_xsalloc, 8u));

   alloc_counter = 0;
   auto _ = allocator.align_xsalloc<alloc_non_trivial>(8u);
   cat::verify(alloc_counter == 1);

   // Test `unalign_salloc`.
   auto _ = allocator.unalign_salloc<int1>();
   allocator.reset();
   auto [p_unalign_salloc, p_unalign_salloc_bytes] =
      allocator.unalign_salloc<int1>(1).value();
   cat::verify(*p_unalign_salloc == 1);
   cat::verify(p_unalign_salloc_bytes == 1);

   alloc_counter = 0;
   auto _ = allocator.unalign_salloc<alloc_non_trivial>();
   cat::verify(alloc_counter == 1);

   // Test `unalign_xsalloc`.
   auto _ = allocator.unalign_xsalloc<int1>();
   allocator.reset();
   auto [p_unalign_xsalloc, p_unalign_xsalloc_bytes] =
      allocator.unalign_xsalloc<int1>(1);
   cat::verify(*p_unalign_xsalloc == 1);
   cat::verify(p_unalign_xsalloc_bytes == 1);

   alloc_counter = 0;
   auto _ = allocator.unalign_xsalloc<alloc_non_trivial>();
   cat::verify(alloc_counter == 1);

   // Test `align_salloc_multi`.
   allocator.reset();
   auto [p_align_salloc_multi, p_align_salloc_multi_bytes] =
      allocator.align_salloc_multi<int4>(8u, 5u).value();
   cat::verify(p_align_salloc_multi_bytes == 24);
   cat::verify(cat::is_aligned(p_align_salloc_multi.data(), 8u));

   alloc_counter = 0;
   auto _ = allocator.align_salloc_multi<alloc_non_trivial>(8u, 5u);
   cat::verify(alloc_counter == 5);

   // Test `align_xsalloc_multi`.
   allocator.reset();
   auto [p_align_xsalloc_multi, p_align_xsalloc_multi_bytes] =
      allocator.align_xsalloc_multi<int4>(8u, 5u);
   cat::verify(p_align_xsalloc_multi_bytes == 24);
   cat::verify(cat::is_aligned(p_align_xsalloc_multi.data(), 8u));

   alloc_counter = 0;
   auto _ = allocator.align_xsalloc_multi<alloc_non_trivial>(8u, 5u);
   cat::verify(alloc_counter == 5);

   // Test `unalign_salloc_multi`.
   allocator.reset();
   auto [p_unalign_salloc_multi, p_unalign_salloc_multi_bytes] =
      allocator.unalign_salloc_multi<int1>(5u).value();
   cat::verify(p_unalign_salloc_multi_bytes == 5);

   alloc_counter = 0;
   auto _ = allocator.unalign_salloc_multi<alloc_non_trivial>(5u);
   cat::verify(alloc_counter == 5);

   // Test `unalign_xsalloc_multi`.
   allocator.reset();
   auto [p_unalign_xsalloc_multi, p_unalign_xsalloc_multi_bytes] =
      allocator.unalign_xsalloc_multi<int1>(5u);
   cat::verify(p_unalign_xsalloc_multi_bytes == 5);

   alloc_counter = 0;
   auto _ = allocator.unalign_xsalloc_multi<alloc_non_trivial>(5u);
   cat::verify(alloc_counter == 5);

   // Test `inline_salloc`.
   allocator.reset();
   auto [inline_salloc, inline_salloc_bytes] =
      allocator.inline_salloc<int4>(1).value();
   cat::verify(allocator.get(inline_salloc) == 1);
   cat::verify(inline_salloc_bytes == 64u);
   cat::verify(inline_salloc.is_inline());

   auto [inline_salloc_big, inline_salloc_bytes_big] =
      allocator.inline_salloc<alloc_huge_object>().value();
   cat::verify(inline_salloc_bytes_big == sizeof(alloc_huge_object));
   cat::verify(!inline_salloc_big.is_inline());

   // Test `inline_xsalloc`.
   allocator.reset();
   auto [inline_xsalloc, inline_xsalloc_bytes] =
      allocator.inline_xsalloc<int4>(1);
   cat::verify(inline_xsalloc_bytes == 64u);
   cat::verify(inline_xsalloc.is_inline());

   auto [inline_xsalloc_big, inline_xsalloc_bytes_big] =
      allocator.inline_xsalloc<alloc_huge_object>();
   cat::verify(inline_xsalloc_bytes_big == sizeof(alloc_huge_object));
   cat::verify(!inline_xsalloc_big.is_inline());

   // Test `inline_salloc_multi`.
   allocator.reset();
   auto [inline_salloc_multi, inline_salloc_multi_bytes] =
      allocator.inline_salloc_multi<int4>(5u).value();
   cat::verify(inline_salloc_multi_bytes == 64u);
   cat::verify(inline_salloc_multi.is_inline());

   auto [inline_salloc_multi_big, inline_salloc_multi_bytes_big] =
      allocator.inline_salloc_multi<alloc_huge_object>(5u).value();
   cat::verify(inline_salloc_multi_bytes_big == sizeof(alloc_huge_object) * 5);
   cat::verify(!inline_salloc_multi_big.is_inline());

   // Test `inline_xsalloc_multi`.
   allocator.reset();
   auto [inline_xsalloc_multi, inline_xsalloc_multi_bytes] =
      allocator.inline_xsalloc_multi<int4>(5u);
   cat::verify(inline_xsalloc_multi_bytes == 64u);
   cat::verify(inline_xsalloc_multi.is_inline());

   auto [inline_xsalloc_multi_big, inline_xsalloc_multi_bytes_big] =
      allocator.inline_xsalloc_multi<alloc_huge_object>(5u);
   cat::verify(inline_xsalloc_multi_bytes_big == sizeof(alloc_huge_object) * 5);
   cat::verify(!inline_xsalloc_multi_big.is_inline());

   // Test `inline_align_salloc`.
   allocator.reset();
   auto [inline_align_salloc, inline_align_salloc_bytes] =
      allocator.inline_align_salloc<int4>(8u, 1).value();
   cat::verify(allocator.get(inline_align_salloc) == 1);
   cat::verify(inline_align_salloc_bytes >= 64u);
   cat::verify(inline_align_salloc.is_inline());

   auto [inline_align_salloc_big, inline_align_salloc_bytes_big] =
      allocator.inline_align_salloc<alloc_huge_object>(8u).value();
   cat::verify(inline_align_salloc_bytes_big >= sizeof(alloc_huge_object));
   cat::verify(!inline_align_salloc_big.is_inline());

   // Test `inline_align_xsalloc`.
   allocator.reset();
   auto [inline_align_xsalloc, inline_align_xsalloc_bytes] =
      allocator.inline_align_xsalloc<int4>(8u, 1);
   cat::verify(allocator.get(inline_align_xsalloc) == 1);
   cat::verify(inline_align_xsalloc_bytes >= 64u);
   cat::verify(inline_align_xsalloc.is_inline());

   auto [inline_align_xsalloc_big, inline_align_xsalloc_bytes_big] =
      allocator.inline_align_xsalloc<alloc_huge_object>(8u);
   cat::verify(inline_align_xsalloc_bytes_big >= sizeof(alloc_huge_object));
   cat::verify(!inline_align_xsalloc_big.is_inline());

   // Test `inline_unalign_salloc`.
   allocator.reset();
   auto [inline_unalign_salloc, inline_unalign_salloc_bytes] =
      allocator.inline_unalign_salloc<int4>(1).value();
   cat::verify(allocator.get(inline_unalign_salloc) == 1);
   cat::verify(inline_unalign_salloc_bytes == 64u);
   cat::verify(inline_unalign_salloc.is_inline());

   auto [inline_unalign_salloc_big, inline_unalign_salloc_bytes_big] =
      allocator.inline_unalign_salloc<alloc_huge_object>().value();
   cat::verify(inline_unalign_salloc_bytes_big == sizeof(alloc_huge_object));
   cat::verify(!inline_unalign_salloc_big.is_inline());

   // Test `inline_unalign_xsalloc`.
   allocator.reset();
   auto [inline_unalign_xsalloc, inline_unalign_xsalloc_bytes] =
      allocator.inline_unalign_xsalloc<int4>(1);
   cat::verify(allocator.get(inline_unalign_xsalloc) == 1);
   cat::verify(inline_unalign_xsalloc_bytes == 64u);
   cat::verify(inline_unalign_xsalloc.is_inline());

   auto [inline_unalign_xsalloc_big, inline_unalign_xsalloc_bytes_big] =
      allocator.inline_unalign_xsalloc<alloc_huge_object>();
   cat::verify(inline_unalign_xsalloc_bytes_big == sizeof(alloc_huge_object));
   cat::verify(!inline_unalign_xsalloc_big.is_inline());

   // Test `inline_align_salloc_multi`.
   allocator.reset();
   auto [inline_align_salloc_multi, inline_align_salloc_multi_bytes] =
      allocator.inline_align_salloc_multi<int4>(8u, 5u).value();
   cat::verify(inline_align_salloc_multi_bytes >= 64u);
   cat::verify(inline_align_salloc_multi.is_inline());

   auto [inline_align_salloc_multi_big, inline_align_salloc_multi_bytes_big] =
      allocator.inline_align_salloc_multi<alloc_huge_object>(8u, 5u).value();
   cat::verify(
      inline_align_salloc_multi_bytes_big >= sizeof(alloc_huge_object)
   );
   cat::verify(!inline_align_salloc_multi_big.is_inline());

   // Test `inline_align_xsalloc_multi`.
   allocator.reset();
   auto [inline_align_xsalloc_multi, inline_align_xsalloc_multi_bytes] =
      allocator.inline_align_xsalloc_multi<int4>(8u, 5u);
   cat::verify(inline_align_xsalloc_multi_bytes >= 64u);
   cat::verify(inline_align_xsalloc_multi.is_inline());

   auto [inline_align_xsalloc_multi_big, inline_align_xsalloc_multi_bytes_big] =
      allocator.inline_align_xsalloc_multi<alloc_huge_object>(8u, 5u);
   cat::verify(
      inline_align_xsalloc_multi_bytes_big >= sizeof(alloc_huge_object)
   );
   cat::verify(!inline_align_xsalloc_multi_big.is_inline());

   // Test `inline_unalign_salloc_multi`.
   allocator.reset();
   auto [inline_unalign_salloc_multi, inline_unalign_salloc_multi_bytes] =
      allocator.inline_unalign_salloc_multi<int4>(5u).value();
   cat::verify(inline_unalign_salloc_multi_bytes == 64u);
   cat::verify(inline_unalign_salloc_multi.is_inline());

   auto
      [inline_unalign_salloc_multi_big, inline_unalign_salloc_multi_bytes_big] =
         allocator.inline_unalign_salloc_multi<alloc_huge_object>(5u).value();
   cat::verify(
      inline_unalign_salloc_multi_bytes_big == sizeof(alloc_huge_object) * 5
   );
   cat::verify(!inline_unalign_salloc_multi_big.is_inline());

   // Test `inline_unalign_xsalloc_multi`.
   allocator.reset();
   auto [inline_unalign_xsalloc_multi, inline_unalign_xsalloc_multi_bytes] =
      allocator.inline_unalign_xsalloc_multi<int4>(5u);
   cat::verify(inline_unalign_xsalloc_multi_bytes == 64u);
   cat::verify(inline_unalign_xsalloc_multi.is_inline());

   auto
      [inline_unalign_xsalloc_multi_big,
       inline_unalign_xsalloc_multi_bytes_big] =
         allocator.inline_unalign_xsalloc_multi<alloc_huge_object>(5u);
   cat::verify(
      inline_unalign_xsalloc_multi_bytes_big == sizeof(alloc_huge_object) * 5
   );
   cat::verify(!inline_unalign_xsalloc_multi_big.is_inline());

   // Test `xcalloc`.
   auto* _ = allocator.xcalloc<int4>();

   // Test `align_xcalloc`.
   auto* _ = allocator.align_xcalloc<int4>(8u);

   // Test `unalign_xcalloc`.
   auto* _ = allocator.unalign_xcalloc<int1>();

   // Test `inline_calloc`.
   auto _ = allocator.inline_calloc<int4>().value();

   // Test `inline_xcalloc`.
   auto _ = allocator.inline_xcalloc<int4>();

   // Test `inline_align_calloc`.
   auto _ = allocator.inline_align_calloc<int4>(8u).value();

   // Test `inline_align_xcalloc`.
   auto _ = allocator.inline_align_xcalloc<int4>(8u);

   // Test `inline_unalign_calloc`.
   auto _ = allocator.inline_unalign_calloc<int4>().value();

   // Test `inline_unalign_xcalloc`.
   auto _ = allocator.inline_unalign_xcalloc<int4>();

   // Test `xscalloc`.
   auto* _ = allocator.xscalloc<int4>().first();

   // Test `align_xscalloc`.
   auto* _ = allocator.align_xscalloc<int4>(8u).first();

   // Test `unalign_xscalloc`.
   auto* _ = allocator.unalign_xscalloc<int1>().first();

   // Test `inline_scalloc`.
   auto _ = allocator.inline_scalloc<int4>().value().first();

   // Test `inline_xscalloc`.
   auto _ = allocator.inline_xscalloc<int4>().first();

   // Test `inline_align_scalloc`.
   auto _ = allocator.inline_align_scalloc<int4>(8u).value().first();

   // Test `inline_align_xscalloc`.
   auto _ = allocator.inline_align_xscalloc<int4>(8u).first();

   // Test `inline_unalign_scalloc`.
   auto _ = allocator.inline_unalign_scalloc<int4>().value().first();

   // Test `inline_unalign_xscalloc`.
   auto _ = allocator.inline_unalign_xscalloc<int4>().first();

   allocator.reset();

   // Test `realloc`.
   auto* p_realloc_1 = allocator.alloc<int4>(1).value();
   auto* p_realloc_2 = allocator.alloc<int4>(2u).value();
   cat::verify(*p_realloc_1 == 1);
   cat::verify(*p_realloc_2 == 2);
   p_realloc_1 = allocator.realloc(p_realloc_2).value();
   cat::verify(*p_realloc_1 == 2);

   // Test `realloc_to`.
   p_alloc = allocator.realloc_to(allocator, p_alloc).value();

   // Test `xrealloc`.
   p_alloc = allocator.xrealloc(p_alloc);

   // Test `xrealloc_to`.
   p_alloc = allocator.xrealloc_to(allocator, p_alloc);

   // Test `align_realloc`.
   p_alloc = allocator.align_realloc(p_alloc, 8u).value();

   // Test `align_realloc_to`.
   p_alloc = allocator.align_realloc_to(allocator, p_alloc, 8u).value();

   // Test `align_xrealloc`.
   p_alloc = allocator.align_xrealloc(p_alloc, 8u);

   // Test `align_xrealloc_to`.
   p_alloc = allocator.align_xrealloc_to(allocator, p_alloc, 8u);

   // Test `unalign_realloc`.
   p_alloc = allocator.unalign_realloc(p_alloc).value();

   // Test `unalign_realloc_to`.
   p_alloc = allocator.unalign_realloc_to(allocator, p_alloc).value();

   // Test `unalign_xrealloc`.
   p_alloc = allocator.unalign_xrealloc(p_alloc);

   // Test `unalign_xrealloc_to`.
   p_alloc = allocator.unalign_xrealloc_to(allocator, p_alloc);

   // Test `realloc_multi`.
   p_alloc = allocator.alloc<int4>().value();
   p_alloc = allocator.realloc_multi(p_alloc, 5, 10u).value().data();
   allocator.free(p_alloc);

   // Test `realloc_multi_to`.
   p_alloc =
      allocator.realloc_multi_to(allocator, p_alloc, 5, 10u).value().data();

   // Test `xrealloc_multi`.
   auto xrealloc_multi_handle = allocator.xrealloc_multi(p_alloc, 5, 10u);
   p_alloc = xrealloc_multi_handle.data();

   // Test `xrealloc_multi_to`.
   auto xrealloc_multi_to_handle =
      allocator.xrealloc_multi_to(allocator, p_alloc, 5, 10u);
   p_alloc = xrealloc_multi_to_handle.data();

   // Test `align_realloc_multi`.
   p_alloc = allocator.align_realloc_multi(p_alloc, 8u, 5, 10u).value().data();

   // Test `align_realloc_multi_to`.
   p_alloc = allocator.align_realloc_multi_to(allocator, p_alloc, 8u, 5, 10u)
                .value()
                .data();

   // Test `align_xrealloc_multi`.
   auto align_xrealloc_multi_handle =
      allocator.align_xrealloc_multi(p_alloc, 8u, 5, 10u);
   p_alloc = align_xrealloc_multi_handle.data();

   // Test `align_xrealloc_multi_to`.
   auto align_xrealloc_multi_to_handle =
      allocator.align_xrealloc_multi_to(allocator, p_alloc, 8u, 5, 10u);
   p_alloc = align_xrealloc_multi_to_handle.data();

   // Test `unalign_realloc_multi`.
   p_alloc = allocator.unalign_realloc_multi(p_alloc, 5, 10u).value().data();

   // Test `unalign_realloc_multi_to`.
   p_alloc = allocator.unalign_realloc_multi_to(allocator, p_alloc, 5, 10u)
                .value()
                .data();

   // Test `unalign_xrealloc_multi`.
   auto unalign_xrealloc_multi_handle =
      allocator.unalign_xrealloc_multi(p_alloc, 5, 10u);
   p_alloc = unalign_xrealloc_multi_handle.data();

   // Test `unalign_xrealloc_multi_to`.
   auto unalign_xrealloc_multi_to_handle =
      allocator.unalign_xrealloc_multi_to(allocator, p_alloc, 5, 10u);
   p_alloc = unalign_xrealloc_multi_to_handle.data();

   // The allocator runs out of memory around here.
   allocator.reset();
   // This poisons `p_alloc`, so reallocate it.
   p_alloc = allocator.alloc<int4>(1).value();

   // Test `recalloc`.
   p_alloc = allocator.recalloc(p_alloc).value();

   // Test `recalloc_to`.
   p_alloc = allocator.recalloc_to(allocator, p_alloc).value();

   // Test `xrecalloc`.
   p_alloc = allocator.xrecalloc(p_alloc);

   // Test `xrecalloc_to`.
   p_alloc = allocator.xrecalloc_to(allocator, p_alloc);

   // Test `align_recalloc`.
   p_alloc = allocator.align_recalloc(p_alloc, 8u).value();

   // Test `align_recalloc_to`.
   p_alloc = allocator.align_recalloc_to(allocator, p_alloc, 8u).value();

   // Test `align_xrecalloc`.
   p_alloc = allocator.align_xrecalloc(p_alloc, 8u);

   // Test `align_xrecalloc_to`.
   p_alloc = allocator.align_xrecalloc_to(allocator, p_alloc, 8u);

   // Test `unalign_recalloc`.
   p_alloc = allocator.unalign_recalloc(p_alloc).value();

   // Test `unalign_recalloc_to`.
   p_alloc = allocator.unalign_recalloc_to(allocator, p_alloc).value();

   // Test `unalign_xrecalloc`.
   p_alloc = allocator.unalign_xrecalloc(p_alloc);

   // Test `unalign_xrecalloc_to`.
   p_alloc = allocator.unalign_xrecalloc_to(allocator, p_alloc);

   // Test `recalloc_multi`.
   p_alloc = allocator.recalloc_multi(p_alloc, 5, 10u).value().data();

   // Test `recalloc_multi_to`.
   p_alloc =
      allocator.recalloc_multi_to(allocator, p_alloc, 5, 10u).value().data();

   // Test `xrecalloc_multi`.
   auto xrecalloc_multi_handle = allocator.xrecalloc_multi(p_alloc, 5, 10u);
   p_alloc = xrecalloc_multi_handle.data();

   // Test `xrecalloc_multi_to`.
   auto xrecalloc_multi_to_handle =
      allocator.xrecalloc_multi_to(allocator, p_alloc, 5, 10u);
   p_alloc = xrecalloc_multi_to_handle.data();

   // Test `align_recalloc_multi`.
   p_alloc = allocator.align_recalloc_multi(p_alloc, 8u, 5, 10u).value().data();

   // Test `align_recalloc_multi_to`.
   p_alloc = allocator.align_recalloc_multi_to(allocator, p_alloc, 8u, 5, 10u)
                .value()
                .data();

   // Test `align_xrecalloc_multi`.
   auto align_xrecalloc_multi_handle =
      allocator.align_xrecalloc_multi(p_alloc, 8u, 5, 10u);
   p_alloc = align_xrecalloc_multi_handle.data();

   // Test `align_xrecalloc_multi_to`.
   auto align_xrecalloc_multi_to_handle =
      allocator.align_xrecalloc_multi_to(allocator, p_alloc, 8u, 5, 10u);
   p_alloc = align_xrecalloc_multi_to_handle.data();

   // Test `unalign_recalloc_multi`.
   p_alloc = allocator.unalign_recalloc_multi(p_alloc, 5, 10u).value().data();

   // Test `unalign_recalloc_multi_to`.
   p_alloc = allocator.unalign_recalloc_multi_to(allocator, p_alloc, 5, 10u)
                .value()
                .data();

   // Test `unalign_xrecalloc_multi`.
   auto unalign_xrecalloc_multi_handle =
      allocator.unalign_xrecalloc_multi(p_alloc, 5, 10u);
   p_alloc = unalign_xrecalloc_multi_handle.data();

   // Test `unalign_xrecalloc_multi_to`.
   auto unalign_xrecalloc_multi_to_handle =
      allocator.unalign_xrecalloc_multi_to(allocator, p_alloc, 5, 10u);
   p_alloc = unalign_xrecalloc_multi_to_handle.data();

   // Test `inline_realloc`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_realloc(inline_alloc);
   allocator.free(inline_alloc);

   // Test `inline_realloc_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_realloc_to(allocator, inline_alloc).value();
   allocator.free(inline_alloc);

   // Test `inline_xrealloc`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_xrealloc(inline_alloc);
   allocator.free(inline_alloc);

   // Test `inline_xrealloc_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_xrealloc_to(allocator, inline_alloc);
   allocator.free(inline_alloc);

   // Test `inline_align_realloc`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_align_realloc(inline_alloc, 8u).value();
   allocator.free(inline_alloc);

   // Test `inline_align_realloc_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ =
      allocator.inline_align_realloc_to(allocator, inline_alloc, 8u).value();
   allocator.free(inline_alloc);

   // Test `inline_align_xrealloc`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_align_xrealloc(inline_alloc, 8u);
   allocator.free(inline_alloc);

   // Test `inline_align_xrealloc_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_align_xrealloc_to(allocator, inline_alloc, 8u);
   allocator.free(inline_alloc);

   // Test `inline_unalign_realloc`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_unalign_realloc(inline_alloc).value();
   allocator.free(inline_alloc);

   // Test `inline_unalign_realloc_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ =
      allocator.inline_unalign_realloc_to(allocator, inline_alloc).value();
   allocator.free(inline_alloc);

   // Test `inline_unalign_xrealloc`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_unalign_xrealloc(inline_alloc);
   allocator.free(inline_alloc);

   // Test `inline_unalign_xrealloc_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_unalign_xrealloc_to(allocator, inline_alloc);
   allocator.free(inline_alloc);

   // Test `inline_align_realloc`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_align_realloc(inline_alloc, 8u).value();
   allocator.free(inline_alloc);

   // Test `inline_align_realloc_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ =
      allocator.inline_align_realloc_to(allocator, inline_alloc, 8u).value();
   allocator.free(inline_alloc);

   // Test `inline_align_xrealloc`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_align_xrealloc(inline_alloc, 8u);
   allocator.free(inline_alloc);

   // Test `inline_align_xrealloc_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_align_xrealloc_to(allocator, inline_alloc, 8u);
   allocator.free(inline_alloc);

   // Test `inline_unalign_realloc`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_unalign_realloc(inline_alloc).value();
   allocator.free(inline_alloc);

   // Test `inline_unalign_realloc_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ =
      allocator.inline_unalign_realloc_to(allocator, inline_alloc).value();
   allocator.free(inline_alloc);

   // Test `inline_unalign_xrealloc`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_unalign_xrealloc(inline_alloc);
   allocator.free(inline_alloc);

   // Test `inline_unalign_xrealloc_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_unalign_xrealloc_to(allocator, inline_alloc);
   allocator.free(inline_alloc);

   // Test `inline_realloc_multi`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_realloc_multi(inline_alloc, 10u).value();
   allocator.free(inline_alloc);

   // Test `inline_realloc_multi_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ =
      allocator.inline_realloc_multi_to(allocator, inline_alloc, 10u).value();
   allocator.free(inline_alloc);

   // Test `inline_xrealloc_multi`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_xrealloc_multi(inline_alloc, 10u);
   allocator.free(inline_alloc);

   // Test `inline_xrealloc_multi_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_xrealloc_multi_to(allocator, inline_alloc, 10u);
   allocator.free(inline_alloc);

   // Test `inline_align_realloc_multi`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_align_realloc_multi(inline_alloc, 8u, 10u).value();
   allocator.free(inline_alloc);

   // Test `inline_align_realloc_multi_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ =
      allocator.inline_align_realloc_multi_to(allocator, inline_alloc, 8u, 10u)
         .value();
   allocator.free(inline_alloc);

   // Test `inline_align_xrealloc_multi`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_align_xrealloc_multi(inline_alloc, 8u, 10u);
   allocator.free(inline_alloc);

   // Test `inline_align_xrealloc_multi_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_align_xrealloc_multi_to(
      allocator, inline_alloc, 8u, 10u
   );
   allocator.free(inline_alloc);

   // Test `inline_unalign_realloc_multi`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_unalign_realloc_multi(inline_alloc, 10u).value();
   allocator.free(inline_alloc);

   // Test `inline_unalign_realloc_multi_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ =
      allocator.inline_unalign_realloc_multi_to(allocator, inline_alloc, 10u)
         .value();
   allocator.free(inline_alloc);

   // Test `inline_unalign_xrealloc_multi`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_unalign_xrealloc_multi(inline_alloc, 10u);
   allocator.free(inline_alloc);

   // Test `inline_unalign_xrealloc_multi_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ =
      allocator.inline_unalign_xrealloc_multi_to(allocator, inline_alloc, 10u);
   allocator.free(inline_alloc);

   // Test `inline_align_realloc_multi`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_align_realloc_multi(inline_alloc, 8u, 10u).value();
   allocator.free(inline_alloc);

   // Test `inline_align_realloc_multi_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ =
      allocator.inline_align_realloc_multi_to(allocator, inline_alloc, 8u, 10u)
         .value();
   allocator.free(inline_alloc);

   // Test `inline_align_xrealloc_multi`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_align_xrealloc_multi(inline_alloc, 8u, 10u);
   allocator.free(inline_alloc);

   // Test `inline_align_xrealloc_multi_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_align_xrealloc_multi_to(
      allocator, inline_alloc, 8u, 10u
   );
   allocator.free(inline_alloc);

   // Test `inline_unalign_realloc_multi`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_unalign_realloc_multi(inline_alloc, 10u).value();
   allocator.free(inline_alloc);

   // Test `inline_unalign_realloc_multi_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ =
      allocator.inline_unalign_realloc_multi_to(allocator, inline_alloc, 10u)
         .value();
   allocator.free(inline_alloc);

   // Test `inline_unalign_xrealloc_multi`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_unalign_xrealloc_multi(inline_alloc, 10u);
   allocator.free(inline_alloc);

   // Test `inline_unalign_xrealloc_multi_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ =
      allocator.inline_unalign_xrealloc_multi_to(allocator, inline_alloc, 10u);
   allocator.free(inline_alloc);

   // The allocator runs out of memory around here.
   allocator.reset();

   // Test `inline_recalloc`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_recalloc(inline_alloc).value();
   allocator.free(inline_alloc);

   // Test `inline_recalloc_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_recalloc_to(allocator, inline_alloc).value();
   allocator.free(inline_alloc);

   // Test `inline_xrecalloc`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_xrecalloc(inline_alloc);
   allocator.free(inline_alloc);

   // Test `inline_xrecalloc_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_xrecalloc_to(allocator, inline_alloc);
   allocator.free(inline_alloc);

   // Test `inline_align_recalloc`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_align_recalloc(inline_alloc, 8u).value();
   allocator.free(inline_alloc);

   // Test `inline_align_recalloc_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ =
      allocator.inline_align_recalloc_to(allocator, inline_alloc, 8u).value();
   allocator.free(inline_alloc);

   // Test `inline_align_xrecalloc`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_align_xrecalloc(inline_alloc, 8u);
   allocator.free(inline_alloc);

   // Test `inline_align_xrecalloc_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_align_xrecalloc_to(allocator, inline_alloc, 8u);
   allocator.free(inline_alloc);

   // Test `inline_unalign_recalloc`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_unalign_recalloc(inline_alloc).value();
   allocator.free(inline_alloc);

   // Test `inline_unalign_recalloc_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ =
      allocator.inline_unalign_recalloc_to(allocator, inline_alloc).value();
   allocator.free(inline_alloc);

   // Test `inline_unalign_xrecalloc`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_unalign_xrecalloc(inline_alloc);
   allocator.free(inline_alloc);

   // Test `inline_unalign_xrecalloc_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_unalign_xrecalloc_to(allocator, inline_alloc);
   allocator.free(inline_alloc);

   // Test `inline_align_recalloc`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_align_recalloc(inline_alloc, 8u).value();
   allocator.free(inline_alloc);

   // Test `inline_align_recalloc_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ =
      allocator.inline_align_recalloc_to(allocator, inline_alloc, 8u).value();
   allocator.free(inline_alloc);

   // Test `inline_align_xrecalloc`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_align_xrecalloc(inline_alloc, 8u);
   allocator.free(inline_alloc);

   // Test `inline_align_xrecalloc_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_align_xrecalloc_to(allocator, inline_alloc, 8u);
   allocator.free(inline_alloc);

   // Test `inline_unalign_recalloc`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_unalign_recalloc(inline_alloc).value();
   allocator.free(inline_alloc);

   // Test `inline_unalign_recalloc_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ =
      allocator.inline_unalign_recalloc_to(allocator, inline_alloc).value();
   allocator.free(inline_alloc);

   // Test `inline_unalign_xrecalloc`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_unalign_xrecalloc(inline_alloc);
   allocator.free(inline_alloc);

   // Test `inline_unalign_xrecalloc_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_unalign_xrecalloc_to(allocator, inline_alloc);
   allocator.free(inline_alloc);

   // Test `inline_recalloc_multi`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_recalloc_multi(inline_alloc, 10u).value();
   allocator.free(inline_alloc);

   // Test `inline_recalloc_multi_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ =
      allocator.inline_recalloc_multi_to(allocator, inline_alloc, 10u).value();
   allocator.free(inline_alloc);

   // Test `inline_xrecalloc_multi`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_xrecalloc_multi(inline_alloc, 10u);
   allocator.free(inline_alloc);

   // Test `inline_xrecalloc_multi_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_xrecalloc_multi_to(allocator, inline_alloc, 10u);
   allocator.free(inline_alloc);

   // Test `inline_align_recalloc_multi`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ =
      allocator.inline_align_recalloc_multi(inline_alloc, 8u, 10u).value();
   allocator.free(inline_alloc);

   // Test `inline_align_recalloc_multi_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ =
      allocator.inline_align_recalloc_multi_to(allocator, inline_alloc, 8u, 10u)
         .value();
   allocator.free(inline_alloc);

   // Test `inline_align_xrecalloc_multi`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_align_xrecalloc_multi(inline_alloc, 8u, 10u);
   allocator.free(inline_alloc);

   // Test `inline_align_xrecalloc_multi_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_align_xrecalloc_multi_to(
      allocator, inline_alloc, 8u, 10u
   );
   allocator.free(inline_alloc);

   // Test `inline_unalign_recalloc_multi`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_unalign_recalloc_multi(inline_alloc, 10u).value();
   allocator.free(inline_alloc);

   // Test `inline_unalign_recalloc_multi_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ =
      allocator.inline_unalign_recalloc_multi_to(allocator, inline_alloc, 10u)
         .value();
   allocator.free(inline_alloc);

   // Test `inline_unalign_xrecalloc_multi`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_unalign_xrecalloc_multi(inline_alloc, 10u);
   allocator.free(inline_alloc);

   // Test `inline_unalign_xrecalloc_multi_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ =
      allocator.inline_unalign_xrecalloc_multi_to(allocator, inline_alloc, 10u);
   allocator.free(inline_alloc);

   // Test `inline_align_recalloc_multi`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ =
      allocator.inline_align_recalloc_multi(inline_alloc, 8u, 10u).value();
   allocator.free(inline_alloc);

   // Test `inline_align_recalloc_multi_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ =
      allocator.inline_align_recalloc_multi_to(allocator, inline_alloc, 8u, 10u)
         .value();
   allocator.free(inline_alloc);

   // Test `inline_align_xrecalloc_multi`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_align_xrecalloc_multi(inline_alloc, 8u, 10u);
   allocator.free(inline_alloc);

   // Test `inline_align_xrecalloc_multi_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_align_xrecalloc_multi_to(
      allocator, inline_alloc, 8u, 10u
   );
   allocator.free(inline_alloc);

   // Test `inline_unalign_recalloc_multi`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_unalign_recalloc_multi(inline_alloc, 10u).value();
   allocator.free(inline_alloc);

   // Test `inline_unalign_recalloc_multi_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ =
      allocator.inline_unalign_recalloc_multi_to(allocator, inline_alloc, 10u)
         .value();
   allocator.free(inline_alloc);

   // Test `inline_unalign_xrecalloc_multi`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_unalign_xrecalloc_multi(inline_alloc, 10u);
   allocator.free(inline_alloc);

   // Test `inline_unalign_xrecalloc_multi_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ =
      allocator.inline_unalign_xrecalloc_multi_to(allocator, inline_alloc, 10u);
   allocator.free(inline_alloc);

   // Test `resalloc`.
   p_alloc = allocator.alloc<int4>(0).value();
   p_alloc = allocator.resalloc(p_alloc).value().first();

   // Test `resalloc_to`.
   p_alloc = allocator.resalloc_to(allocator, p_alloc).value().first();

   // Test `xresalloc`.
   p_alloc = allocator.xresalloc(p_alloc).first();

   // Test `xresalloc_to`.
   p_alloc = allocator.xresalloc_to(allocator, p_alloc).first();

   // Test `align_resalloc`.
   p_alloc = allocator.align_resalloc(p_alloc, 8u).value().first();

   // Test `align_resalloc_to`.
   p_alloc =
      allocator.align_resalloc_to(allocator, p_alloc, 8u).value().first();

   // Test `align_xresalloc`.
   p_alloc = allocator.align_xresalloc(p_alloc, 8u).first();

   // Test `align_xresalloc_to`.
   p_alloc = allocator.align_xresalloc_to(allocator, p_alloc, 8u).first();

   // Test `unalign_resalloc`.
   p_alloc = allocator.unalign_resalloc(p_alloc).value().first();

   // Test `unalign_resalloc_to`.
   p_alloc = allocator.unalign_resalloc_to(allocator, p_alloc).value().first();

   // Test `unalign_xresalloc`.
   p_alloc = allocator.unalign_xresalloc(p_alloc).first();

   // Test `unalign_xresalloc_to`.
   p_alloc = allocator.unalign_xresalloc_to(allocator, p_alloc).first();

   // Test `resalloc_multi`.
   p_alloc = allocator.resalloc_multi(p_alloc, 5, 10u).value().first().data();

   // Test `resalloc_multi_to`.
   p_alloc = allocator.resalloc_multi_to(allocator, p_alloc, 5, 10u)
                .value()
                .first()
                .data();

   // Test `xresalloc_multi`.
   p_alloc = allocator.xresalloc_multi(p_alloc, 5, 10u).first().data();

   // Test `xresalloc_multi_to`.
   p_alloc =
      allocator.xresalloc_multi_to(allocator, p_alloc, 5, 10u).first().data();

   // Test `align_resalloc_multi`.
   p_alloc = allocator.align_resalloc_multi(p_alloc, 8u, 5, 10u)
                .value()
                .first()
                .data();

   // Test `align_resalloc_multi_to`.
   p_alloc = allocator.align_resalloc_multi_to(allocator, p_alloc, 8u, 5, 10u)
                .value()
                .first()
                .data();

   // Test `align_xresalloc_multi`.
   p_alloc =
      allocator.align_xresalloc_multi(p_alloc, 8u, 5, 10u).first().data();

   // Test `align_xresalloc_multi_to`.
   p_alloc = allocator.align_xresalloc_multi_to(allocator, p_alloc, 8u, 5, 10u)
                .first()
                .data();

   // Test `unalign_resalloc_multi`.
   p_alloc =
      allocator.unalign_resalloc_multi(p_alloc, 5, 10u).value().first().data();

   // Test `unalign_resalloc_multi_to`.
   p_alloc = allocator.unalign_resalloc_multi_to(allocator, p_alloc, 5, 10u)
                .value()
                .first()
                .data();

   // Test `unalign_xresalloc_multi`.
   p_alloc = allocator.unalign_xresalloc_multi(p_alloc, 5, 10u).first().data();

   // Test `unalign_xresalloc_multi_to`.
   p_alloc = allocator.unalign_xresalloc_multi_to(allocator, p_alloc, 5, 10u)
                .first()
                .data();

   // The allocator runs out of memory around here.
   allocator.reset();

   // Test `rescalloc`.
   p_alloc = allocator.alloc<int4>(0).value();
   p_alloc = allocator.rescalloc(p_alloc).value().first();

   // Test `rescalloc_to`.
   p_alloc = allocator.rescalloc_to(allocator, p_alloc).value().first();

   // Test `xrescalloc`.
   p_alloc = allocator.xrescalloc(p_alloc).first();

   // Test `xrescalloc_to`.
   p_alloc = allocator.xrescalloc_to(allocator, p_alloc).first();

   // Test `align_rescalloc`.
   p_alloc = allocator.align_rescalloc(p_alloc, 8u).value().first();

   // Test `align_rescalloc_to`.
   p_alloc =
      allocator.align_rescalloc_to(allocator, p_alloc, 8u).value().first();

   // Test `align_xrescalloc`.
   p_alloc = allocator.align_xrescalloc(p_alloc, 8u).first();

   // Test `align_xrescalloc_to`.
   p_alloc = allocator.align_xrescalloc_to(allocator, p_alloc, 8u).first();

   // Test `unalign_rescalloc`.
   p_alloc = allocator.unalign_rescalloc(p_alloc).value().first();

   // Test `unalign_rescalloc_to`.
   p_alloc = allocator.unalign_rescalloc_to(allocator, p_alloc).value().first();

   // Test `unalign_xrescalloc`.
   p_alloc = allocator.unalign_xrescalloc(p_alloc).first();

   // Test `unalign_xrescalloc_to`.
   p_alloc = allocator.unalign_xrescalloc_to(allocator, p_alloc).first();

   // Test `rescalloc_multi`.
   p_alloc = allocator.rescalloc_multi(p_alloc, 5, 10u).value().first().data();

   // Test `rescalloc_multi_to`.
   p_alloc = allocator.rescalloc_multi_to(allocator, p_alloc, 5, 10u)
                .value()
                .first()
                .data();

   // Test `xrescalloc_multi`.
   p_alloc = allocator.xrescalloc_multi(p_alloc, 5, 10u).first().data();

   // Test `xrescalloc_multi_to`.
   p_alloc =
      allocator.xrescalloc_multi_to(allocator, p_alloc, 5, 10u).first().data();

   // Test `align_rescalloc_multi`.
   p_alloc = allocator.align_rescalloc_multi(p_alloc, 8u, 5, 10u)
                .value()
                .first()
                .data();

   // Test `align_rescalloc_multi_to`.
   p_alloc = allocator.align_rescalloc_multi_to(allocator, p_alloc, 8u, 5, 10u)
                .value()
                .first()
                .data();

   // Test `align_xrescalloc_multi`.
   p_alloc =
      allocator.align_xrescalloc_multi(p_alloc, 8u, 5, 10u).first().data();

   // Test `align_xrescalloc_multi_to`.
   p_alloc = allocator.align_xrescalloc_multi_to(allocator, p_alloc, 8u, 5, 10u)
                .first()
                .data();

   // Test `unalign_rescalloc_multi`.
   p_alloc =
      allocator.unalign_rescalloc_multi(p_alloc, 5, 10u).value().first().data();

   // Test `unalign_rescalloc_multi_to`.
   p_alloc = allocator.unalign_rescalloc_multi_to(allocator, p_alloc, 5, 10u)
                .value()
                .first()
                .data();

   // Test `unalign_xrescalloc_multi`.
   p_alloc = allocator.unalign_xrescalloc_multi(p_alloc, 5, 10u).first().data();

   // Test `unalign_xrescalloc_multi_to`.
   p_alloc = allocator.unalign_xrescalloc_multi_to(allocator, p_alloc, 5, 10u)
                .first()
                .data();

   // Test `inline_resalloc`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_resalloc(inline_alloc);
   allocator.free(inline_alloc);

   // Test `inline_resalloc_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_resalloc_to(allocator, inline_alloc).value();
   allocator.free(inline_alloc);

   // Test `inline_xresalloc`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_xresalloc(inline_alloc);
   allocator.free(inline_alloc);

   // Test `inline_xresalloc_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_xresalloc_to(allocator, inline_alloc);
   allocator.free(inline_alloc);

   // Test `inline_align_resalloc`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_align_resalloc(inline_alloc, 8u).value();
   allocator.free(inline_alloc);

   // Test `inline_align_resalloc_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ =
      allocator.inline_align_resalloc_to(allocator, inline_alloc, 8u).value();
   allocator.free(inline_alloc);

   // Test `inline_align_xresalloc`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_align_xresalloc(inline_alloc, 8u);
   allocator.free(inline_alloc);

   // Test `inline_align_xresalloc_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_align_xresalloc_to(allocator, inline_alloc, 8u);
   allocator.free(inline_alloc);

   // Test `inline_unalign_resalloc`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_unalign_resalloc(inline_alloc).value();
   allocator.free(inline_alloc);

   // Test `inline_unalign_resalloc_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ =
      allocator.inline_unalign_resalloc_to(allocator, inline_alloc).value();
   allocator.free(inline_alloc);

   // Test `inline_unalign_xresalloc`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_unalign_xresalloc(inline_alloc);
   allocator.free(inline_alloc);

   // Test `inline_unalign_xresalloc_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_unalign_xresalloc_to(allocator, inline_alloc);
   allocator.free(inline_alloc);

   // Test `inline_align_resalloc`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_align_resalloc(inline_alloc, 8u).value();
   allocator.free(inline_alloc);

   // Test `inline_align_resalloc_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ =
      allocator.inline_align_resalloc_to(allocator, inline_alloc, 8u).value();
   allocator.free(inline_alloc);

   // Test `inline_align_xresalloc`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_align_xresalloc(inline_alloc, 8u);
   allocator.free(inline_alloc);

   // Test `inline_align_xresalloc_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_align_xresalloc_to(allocator, inline_alloc, 8u);
   allocator.free(inline_alloc);

   // Test `inline_unalign_resalloc`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_unalign_resalloc(inline_alloc).value();
   allocator.free(inline_alloc);

   // Test `inline_unalign_resalloc_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ =
      allocator.inline_unalign_resalloc_to(allocator, inline_alloc).value();
   allocator.free(inline_alloc);

   // Test `inline_unalign_xresalloc`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_unalign_xresalloc(inline_alloc);
   allocator.free(inline_alloc);

   // Test `inline_unalign_xresalloc_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_unalign_xresalloc_to(allocator, inline_alloc);
   allocator.free(inline_alloc);

   // Test `inline_resalloc_multi`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_resalloc_multi(inline_alloc, 10u).value();
   allocator.free(inline_alloc);

   // Test `inline_resalloc_multi_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ =
      allocator.inline_resalloc_multi_to(allocator, inline_alloc, 10u).value();
   allocator.free(inline_alloc);

   // Test `inline_xresalloc_multi`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_xresalloc_multi(inline_alloc, 10u);
   allocator.free(inline_alloc);

   // Test `inline_xresalloc_multi_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_xresalloc_multi_to(allocator, inline_alloc, 10u);
   allocator.free(inline_alloc);

   // Test `inline_align_resalloc_multi`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ =
      allocator.inline_align_resalloc_multi(inline_alloc, 8u, 10u).value();
   allocator.free(inline_alloc);

   // Test `inline_align_resalloc_multi_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ =
      allocator.inline_align_resalloc_multi_to(allocator, inline_alloc, 8u, 10u)
         .value();
   allocator.free(inline_alloc);

   // Test `inline_align_xresalloc_multi`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_align_xresalloc_multi(inline_alloc, 8u, 10u);
   allocator.free(inline_alloc);

   // Test `inline_align_xresalloc_multi_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_align_xresalloc_multi_to(
      allocator, inline_alloc, 8u, 10u
   );
   allocator.free(inline_alloc);

   // Test `inline_unalign_resalloc_multi`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_unalign_resalloc_multi(inline_alloc, 10u).value();
   allocator.free(inline_alloc);

   // Test `inline_unalign_resalloc_multi_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ =
      allocator.inline_unalign_resalloc_multi_to(allocator, inline_alloc, 10u)
         .value();
   allocator.free(inline_alloc);

   // Test `inline_unalign_xresalloc_multi`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_unalign_xresalloc_multi(inline_alloc, 10u);
   allocator.free(inline_alloc);

   // Test `inline_unalign_xresalloc_multi_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ =
      allocator.inline_unalign_xresalloc_multi_to(allocator, inline_alloc, 10u);
   allocator.free(inline_alloc);

   // Test `inline_align_resalloc_multi`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ =
      allocator.inline_align_resalloc_multi(inline_alloc, 8u, 10u).value();
   allocator.free(inline_alloc);

   // Test `inline_align_resalloc_multi_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ =
      allocator.inline_align_resalloc_multi_to(allocator, inline_alloc, 8u, 10u)
         .value();
   allocator.free(inline_alloc);

   // Test `inline_align_xresalloc_multi`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_align_xresalloc_multi(inline_alloc, 8u, 10u);
   allocator.free(inline_alloc);

   // Test `inline_align_xresalloc_multi_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_align_xresalloc_multi_to(
      allocator, inline_alloc, 8u, 10u
   );
   allocator.free(inline_alloc);

   // Test `inline_unalign_resalloc_multi`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_unalign_resalloc_multi(inline_alloc, 10u).value();
   allocator.free(inline_alloc);

   // Test `inline_unalign_resalloc_multi_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ =
      allocator.inline_unalign_resalloc_multi_to(allocator, inline_alloc, 10u)
         .value();
   allocator.free(inline_alloc);

   // Test `inline_unalign_xresalloc_multi`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_unalign_xresalloc_multi(inline_alloc, 10u);
   allocator.free(inline_alloc);

   // Test `inline_unalign_xresalloc_multi_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ =
      allocator.inline_unalign_xresalloc_multi_to(allocator, inline_alloc, 10u);
   allocator.free(inline_alloc);

   // The allocator runs out of memory around here.
   allocator.reset();

   // Test `inline_rescalloc`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_rescalloc(inline_alloc).value();
   allocator.free(inline_alloc);

   // Test `inline_rescalloc_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_rescalloc_to(allocator, inline_alloc).value();
   allocator.free(inline_alloc);

   // Test `inline_xrescalloc`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_xrescalloc(inline_alloc);
   allocator.free(inline_alloc);

   // Test `inline_xrescalloc_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_xrescalloc_to(allocator, inline_alloc);
   allocator.free(inline_alloc);

   // Test `inline_align_rescalloc`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_align_rescalloc(inline_alloc, 8u).value();
   allocator.free(inline_alloc);

   // Test `inline_align_rescalloc_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ =
      allocator.inline_align_rescalloc_to(allocator, inline_alloc, 8u).value();
   allocator.free(inline_alloc);

   // Test `inline_align_xrescalloc`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_align_xrescalloc(inline_alloc, 8u);
   allocator.free(inline_alloc);

   // Test `inline_align_xrescalloc_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_align_xrescalloc_to(allocator, inline_alloc, 8u);
   allocator.free(inline_alloc);

   // Test `inline_unalign_rescalloc`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_unalign_rescalloc(inline_alloc).value();
   allocator.free(inline_alloc);

   // Test `inline_unalign_rescalloc_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ =
      allocator.inline_unalign_rescalloc_to(allocator, inline_alloc).value();
   allocator.free(inline_alloc);

   // Test `inline_unalign_xrescalloc`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_unalign_xrescalloc(inline_alloc);
   allocator.free(inline_alloc);

   // Test `inline_unalign_xrescalloc_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_unalign_xrescalloc_to(allocator, inline_alloc);
   allocator.free(inline_alloc);

   // Test `inline_align_rescalloc`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_align_rescalloc(inline_alloc, 8u).value();
   allocator.free(inline_alloc);

   // Test `inline_align_rescalloc_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ =
      allocator.inline_align_rescalloc_to(allocator, inline_alloc, 8u).value();
   allocator.free(inline_alloc);

   // Test `inline_align_xrescalloc`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_align_xrescalloc(inline_alloc, 8u);
   allocator.free(inline_alloc);

   // Test `inline_align_xrescalloc_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_align_xrescalloc_to(allocator, inline_alloc, 8u);
   allocator.free(inline_alloc);

   // Test `inline_unalign_rescalloc`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_unalign_rescalloc(inline_alloc).value();
   allocator.free(inline_alloc);

   // Test `inline_unalign_rescalloc_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ =
      allocator.inline_unalign_rescalloc_to(allocator, inline_alloc).value();
   allocator.free(inline_alloc);

   // Test `inline_unalign_xrescalloc`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_unalign_xrescalloc(inline_alloc);
   allocator.free(inline_alloc);

   // Test `inline_unalign_xrescalloc_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_unalign_xrescalloc_to(allocator, inline_alloc);
   allocator.free(inline_alloc);

   // Test `inline_rescalloc_multi`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_rescalloc_multi(inline_alloc, 10u).value();
   allocator.free(inline_alloc);

   // Test `inline_rescalloc_multi_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ =
      allocator.inline_rescalloc_multi_to(allocator, inline_alloc, 10u).value();
   allocator.free(inline_alloc);

   // Test `inline_xrescalloc_multi`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_xrescalloc_multi(inline_alloc, 10u);
   allocator.free(inline_alloc);

   // Test `inline_xrescalloc_multi_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_xrescalloc_multi_to(allocator, inline_alloc, 10u);
   allocator.free(inline_alloc);

   // Test `inline_align_rescalloc_multi`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ =
      allocator.inline_align_rescalloc_multi(inline_alloc, 8u, 10u).value();
   allocator.free(inline_alloc);

   // Test `inline_align_rescalloc_multi_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ =
      allocator
         .inline_align_rescalloc_multi_to(allocator, inline_alloc, 8u, 10u)
         .value();
   allocator.free(inline_alloc);

   // Test `inline_align_xrescalloc_multi`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_align_xrescalloc_multi(inline_alloc, 8u, 10u);
   allocator.free(inline_alloc);

   // Test `inline_align_xrescalloc_multi_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_align_xrescalloc_multi_to(
      allocator, inline_alloc, 8u, 10u
   );
   allocator.free(inline_alloc);

   // Test `inline_unalign_rescalloc_multi`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_unalign_rescalloc_multi(inline_alloc, 10u).value();
   allocator.free(inline_alloc);

   // Test `inline_unalign_rescalloc_multi_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ =
      allocator.inline_unalign_rescalloc_multi_to(allocator, inline_alloc, 10u)
         .value();
   allocator.free(inline_alloc);

   // Test `inline_nalign_xrescalloc_multi`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_unalign_xrescalloc_multi(inline_alloc, 10u);
   allocator.free(inline_alloc);

   // Test `inline_unalign_xrescalloc_multi_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_unalign_xrescalloc_multi_to(
      allocator, inline_alloc, 10u
   );
   allocator.free(inline_alloc);

   // Test `inline_align_rescalloc_multi`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ =
      allocator.inline_align_rescalloc_multi(inline_alloc, 8u, 10u).value();
   allocator.free(inline_alloc);

   // Test `inline_align_rescalloc_multi_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ =
      allocator
         .inline_align_rescalloc_multi_to(allocator, inline_alloc, 8u, 10u)
         .value();
   allocator.free(inline_alloc);

   // Test `inline_align_xrescalloc_multi`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_align_xrescalloc_multi(inline_alloc, 8u, 10u);
   allocator.free(inline_alloc);

   // Test `inline_align_xrescalloc_multi_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_align_xrescalloc_multi_to(
      allocator, inline_alloc, 8u, 10u
   );
   allocator.free(inline_alloc);

   // Test `inline_unalign_rescalloc_multi`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_unalign_rescalloc_multi(inline_alloc, 10u).value();
   allocator.free(inline_alloc);

   // Test `inline_unalign_rescalloc_multi_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ =
      allocator.inline_unalign_rescalloc_multi_to(allocator, inline_alloc, 10u)
         .value();
   allocator.free(inline_alloc);

   // Test `inline_nalign_xrescalloc_multi`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_unalign_xrescalloc_multi(inline_alloc, 10u);
   allocator.free(inline_alloc);

   // Test `inline_unalign_xrescalloc_multi_to`.
   inline_alloc = allocator.inline_alloc<int4>().value();
   auto _ = allocator.inline_unalign_xrescalloc_multi_to(
      allocator, inline_alloc, 10u
   );
}

// Exercise every `inline_*` non-nalloc function with a non-default SBO size
// (`16u`) to prove that the `inline_size` template parameter is honored
// end-to-end. With a 16-byte buffer, an `int4` fits inline but
// `alloc_huge_object` (65 bytes) and a 5-element `int4` array (20 bytes) do
// not.
$test(alloc_inline_sbo16) {
   const_test_inline_alloc_sbo16();

   constexpr cat::idx sbo = 16u;

   pager.reset();
   cat::span page = pager.alloc_multi<cat::byte>(4_uki - 64u).or_exit();
   $defer {
      pager.free(page);
   };
   auto allocator = cat::make_linear_allocator(page);

   // Test `inline_alloc<T, sbo>`.
   auto h_alloc = allocator.inline_alloc<int4, sbo>(1).value();
   cat::verify(allocator.get(h_alloc) == 1);
   cat::verify(h_alloc.is_inline());
   auto h_alloc_big = allocator.inline_alloc<alloc_huge_object, sbo>().value();
   cat::verify(!h_alloc_big.is_inline());

   // Test `inline_xalloc<T, sbo>`.
   auto h_xalloc = allocator.inline_xalloc<int4, sbo>(2);
   cat::verify(allocator.get(h_xalloc) == 2);
   cat::verify(h_xalloc.is_inline());
   auto h_xalloc_big = allocator.inline_xalloc<alloc_huge_object, sbo>();
   cat::verify(!h_xalloc_big.is_inline());

   // Test `inline_alloc_multi<T, sbo>`.
   auto h_alloc_multi = allocator.inline_alloc_multi<int4, sbo>(2u).value();
   cat::verify(h_alloc_multi.size() == 2);
   cat::verify(h_alloc_multi.is_inline());
   auto h_alloc_multi_big = allocator.inline_alloc_multi<int4, sbo>(5u).value();
   cat::verify(!h_alloc_multi_big.is_inline());

   // Test `inline_xalloc_multi<T, sbo>`.
   auto h_xalloc_multi = allocator.inline_xalloc_multi<int4, sbo>(2u);
   cat::verify(h_xalloc_multi.size() == 2);
   cat::verify(h_xalloc_multi.is_inline());
   auto h_xalloc_multi_big = allocator.inline_xalloc_multi<int4, sbo>(5u);
   cat::verify(!h_xalloc_multi_big.is_inline());

   // Test `inline_align_alloc<T, sbo>`.
   auto h_align_alloc = allocator.inline_align_alloc<int4, sbo>(8u, 3).value();
   cat::verify(allocator.get(h_align_alloc) == 3);
   cat::verify(cat::is_aligned(&allocator.get(h_align_alloc), 8u));
   cat::verify(h_align_alloc.is_inline());
   auto h_align_alloc_big =
      allocator.inline_align_alloc<alloc_huge_object, sbo>(8u).value();
   cat::verify(!h_align_alloc_big.is_inline());

   // Test `inline_align_xalloc<T, sbo>`.
   auto h_align_xalloc = allocator.inline_align_xalloc<int4, sbo>(8u, 4);
   cat::verify(allocator.get(h_align_xalloc) == 4);
   cat::verify(cat::is_aligned(&allocator.get(h_align_xalloc), 8u));
   cat::verify(h_align_xalloc.is_inline());
   auto h_align_xalloc_big =
      allocator.inline_align_xalloc<alloc_huge_object, sbo>(8u);
   cat::verify(!h_align_xalloc_big.is_inline());

   // Test `inline_unalign_alloc<T, sbo>`.
   auto h_unalign_alloc = allocator.inline_unalign_alloc<int4, sbo>(5).value();
   cat::verify(allocator.get(h_unalign_alloc) == 5);
   cat::verify(h_unalign_alloc.is_inline());
   auto h_unalign_alloc_big =
      allocator.inline_unalign_alloc<alloc_huge_object, sbo>().value();
   cat::verify(!h_unalign_alloc_big.is_inline());

   // Test `inline_unalign_xalloc<T, sbo>`.
   auto h_unalign_xalloc = allocator.inline_unalign_xalloc<int4, sbo>(6);
   cat::verify(allocator.get(h_unalign_xalloc) == 6);
   cat::verify(h_unalign_xalloc.is_inline());
   auto h_unalign_xalloc_big =
      allocator.inline_unalign_xalloc<alloc_huge_object, sbo>();
   cat::verify(!h_unalign_xalloc_big.is_inline());

   allocator.reset();

   // Test `inline_align_alloc_multi<T, sbo>`.
   auto h_align_alloc_multi =
      allocator.inline_align_alloc_multi<int4, sbo>(8u, 2u).value();
   cat::verify(cat::is_aligned(allocator.get_ptr(h_align_alloc_multi), 8u));
   cat::verify(h_align_alloc_multi.is_inline());
   auto h_align_alloc_multi_big =
      allocator.inline_align_alloc_multi<int4, sbo>(8u, 5u).value();
   cat::verify(!h_align_alloc_multi_big.is_inline());

   // Test `inline_align_xalloc_multi<T, sbo>`.
   auto h_align_xalloc_multi =
      allocator.inline_align_xalloc_multi<int4, sbo>(8u, 2u);
   cat::verify(cat::is_aligned(allocator.get_ptr(h_align_xalloc_multi), 8u));
   cat::verify(h_align_xalloc_multi.is_inline());
   auto h_align_xalloc_multi_big =
      allocator.inline_align_xalloc_multi<int4, sbo>(8u, 5u);
   cat::verify(!h_align_xalloc_multi_big.is_inline());

   // Test `inline_unalign_alloc_multi<T, sbo>`.
   auto h_unalign_alloc_multi =
      allocator.inline_unalign_alloc_multi<int4, sbo>(2u).value();
   cat::verify(h_unalign_alloc_multi.is_inline());
   auto h_unalign_alloc_multi_big =
      allocator.inline_unalign_alloc_multi<int4, sbo>(5u).value();
   cat::verify(!h_unalign_alloc_multi_big.is_inline());

   // Test `inline_unalign_xalloc_multi<T, sbo>`.
   auto h_unalign_xalloc_multi =
      allocator.inline_unalign_xalloc_multi<int4, sbo>(2u);
   cat::verify(h_unalign_xalloc_multi.is_inline());
   auto h_unalign_xalloc_multi_big =
      allocator.inline_unalign_xalloc_multi<int4, sbo>(5u);
   cat::verify(!h_unalign_xalloc_multi_big.is_inline());

   allocator.reset();

   // Test `inline_salloc<T, sbo>`.
   auto [h_salloc, h_salloc_bytes] =
      allocator.inline_salloc<int4, sbo>(1).value();
   cat::verify(allocator.get(h_salloc) == 1);
   cat::verify(h_salloc_bytes == sbo);
   cat::verify(h_salloc.is_inline());
   auto [h_salloc_big, h_salloc_bytes_big] =
      allocator.inline_salloc<alloc_huge_object, sbo>().value();
   cat::verify(h_salloc_bytes_big == sizeof(alloc_huge_object));
   cat::verify(!h_salloc_big.is_inline());

   // Test `inline_xsalloc<T, sbo>`.
   auto [h_xsalloc, h_xsalloc_bytes] = allocator.inline_xsalloc<int4, sbo>(2);
   cat::verify(h_xsalloc_bytes == sbo);
   cat::verify(h_xsalloc.is_inline());
   auto [h_xsalloc_big, h_xsalloc_bytes_big] =
      allocator.inline_xsalloc<alloc_huge_object, sbo>();
   cat::verify(h_xsalloc_bytes_big == sizeof(alloc_huge_object));
   cat::verify(!h_xsalloc_big.is_inline());

   // Test `inline_salloc_multi<T, sbo>`. `int4` is 4-byte aligned, so the
   // overflow path may pad the byte count returned by the linear allocator.
   auto [h_salloc_multi, h_salloc_multi_bytes] =
      allocator.inline_salloc_multi<int4, sbo>(2u).value();
   cat::verify(h_salloc_multi_bytes == sbo);
   cat::verify(h_salloc_multi.is_inline());
   auto [h_salloc_multi_big, h_salloc_multi_bytes_big] =
      allocator.inline_salloc_multi<int4, sbo>(5u).value();
   cat::verify(h_salloc_multi_bytes_big >= sizeof(int4) * 5);
   cat::verify(!h_salloc_multi_big.is_inline());

   // Test `inline_xsalloc_multi<T, sbo>`.
   auto [h_xsalloc_multi, h_xsalloc_multi_bytes] =
      allocator.inline_xsalloc_multi<int4, sbo>(2u);
   cat::verify(h_xsalloc_multi_bytes == sbo);
   cat::verify(h_xsalloc_multi.is_inline());
   auto [h_xsalloc_multi_big, h_xsalloc_multi_bytes_big] =
      allocator.inline_xsalloc_multi<int4, sbo>(5u);
   cat::verify(h_xsalloc_multi_bytes_big >= sizeof(int4) * 5);
   cat::verify(!h_xsalloc_multi_big.is_inline());

   allocator.reset();

   // Test `inline_align_salloc<T, sbo>`.
   auto [h_align_salloc, h_align_salloc_bytes] =
      allocator.inline_align_salloc<int4, sbo>(8u, 7).value();
   cat::verify(allocator.get(h_align_salloc) == 7);
   cat::verify(h_align_salloc_bytes >= sbo);
   cat::verify(h_align_salloc.is_inline());
   auto [h_align_salloc_big, h_align_salloc_bytes_big] =
      allocator.inline_align_salloc<alloc_huge_object, sbo>(8u).value();
   cat::verify(h_align_salloc_bytes_big >= sizeof(alloc_huge_object));
   cat::verify(!h_align_salloc_big.is_inline());

   // Test `inline_align_xsalloc<T, sbo>`.
   auto [h_align_xsalloc, h_align_xsalloc_bytes] =
      allocator.inline_align_xsalloc<int4, sbo>(8u, 8);
   cat::verify(allocator.get(h_align_xsalloc) == 8);
   cat::verify(h_align_xsalloc_bytes >= sbo);
   cat::verify(h_align_xsalloc.is_inline());
   auto [h_align_xsalloc_big, h_align_xsalloc_bytes_big] =
      allocator.inline_align_xsalloc<alloc_huge_object, sbo>(8u);
   cat::verify(h_align_xsalloc_bytes_big >= sizeof(alloc_huge_object));
   cat::verify(!h_align_xsalloc_big.is_inline());

   // Test `inline_unalign_salloc<T, sbo>`.
   auto [h_unalign_salloc, h_unalign_salloc_bytes] =
      allocator.inline_unalign_salloc<int4, sbo>(9).value();
   cat::verify(allocator.get(h_unalign_salloc) == 9);
   cat::verify(h_unalign_salloc_bytes == sbo);
   cat::verify(h_unalign_salloc.is_inline());
   auto [h_unalign_salloc_big, h_unalign_salloc_bytes_big] =
      allocator.inline_unalign_salloc<alloc_huge_object, sbo>().value();
   cat::verify(h_unalign_salloc_bytes_big == sizeof(alloc_huge_object));
   cat::verify(!h_unalign_salloc_big.is_inline());

   // Test `inline_unalign_xsalloc<T, sbo>`.
   auto [h_unalign_xsalloc, h_unalign_xsalloc_bytes] =
      allocator.inline_unalign_xsalloc<int4, sbo>(10);
   cat::verify(allocator.get(h_unalign_xsalloc) == 10);
   cat::verify(h_unalign_xsalloc_bytes == sbo);
   cat::verify(h_unalign_xsalloc.is_inline());
   auto [h_unalign_xsalloc_big, h_unalign_xsalloc_bytes_big] =
      allocator.inline_unalign_xsalloc<alloc_huge_object, sbo>();
   cat::verify(h_unalign_xsalloc_bytes_big == sizeof(alloc_huge_object));
   cat::verify(!h_unalign_xsalloc_big.is_inline());

   allocator.reset();

   // Test `inline_align_salloc_multi<T, sbo>`.
   auto [h_align_salloc_multi, h_align_salloc_multi_bytes] =
      allocator.inline_align_salloc_multi<int4, sbo>(8u, 2u).value();
   cat::verify(h_align_salloc_multi_bytes >= sbo);
   cat::verify(h_align_salloc_multi.is_inline());
   auto [h_align_salloc_multi_big, h_align_salloc_multi_bytes_big] =
      allocator.inline_align_salloc_multi<int4, sbo>(8u, 5u).value();
   cat::verify(h_align_salloc_multi_bytes_big >= sizeof(int4) * 5);
   cat::verify(!h_align_salloc_multi_big.is_inline());

   // Test `inline_align_xsalloc_multi<T, sbo>`.
   auto [h_align_xsalloc_multi, h_align_xsalloc_multi_bytes] =
      allocator.inline_align_xsalloc_multi<int4, sbo>(8u, 2u);
   cat::verify(h_align_xsalloc_multi_bytes >= sbo);
   cat::verify(h_align_xsalloc_multi.is_inline());
   auto [h_align_xsalloc_multi_big, h_align_xsalloc_multi_bytes_big] =
      allocator.inline_align_xsalloc_multi<int4, sbo>(8u, 5u);
   cat::verify(h_align_xsalloc_multi_bytes_big >= sizeof(int4) * 5);
   cat::verify(!h_align_xsalloc_multi_big.is_inline());

   // Test `inline_unalign_salloc_multi<T, sbo>`.
   auto [h_unalign_salloc_multi, h_unalign_salloc_multi_bytes] =
      allocator.inline_unalign_salloc_multi<int4, sbo>(2u).value();
   cat::verify(h_unalign_salloc_multi_bytes == sbo);
   cat::verify(h_unalign_salloc_multi.is_inline());
   auto [h_unalign_salloc_multi_big, h_unalign_salloc_multi_bytes_big] =
      allocator.inline_unalign_salloc_multi<int4, sbo>(5u).value();
   cat::verify(h_unalign_salloc_multi_bytes_big == sizeof(int4) * 5);
   cat::verify(!h_unalign_salloc_multi_big.is_inline());

   // Test `inline_unalign_xsalloc_multi<T, sbo>`.
   auto [h_unalign_xsalloc_multi, h_unalign_xsalloc_multi_bytes] =
      allocator.inline_unalign_xsalloc_multi<int4, sbo>(2u);
   cat::verify(h_unalign_xsalloc_multi_bytes == sbo);
   cat::verify(h_unalign_xsalloc_multi.is_inline());
   auto [h_unalign_xsalloc_multi_big, h_unalign_xsalloc_multi_bytes_big] =
      allocator.inline_unalign_xsalloc_multi<int4, sbo>(5u);
   cat::verify(h_unalign_xsalloc_multi_bytes_big == sizeof(int4) * 5);
   cat::verify(!h_unalign_xsalloc_multi_big.is_inline());

   allocator.reset();

   // Test `inline_calloc<T, sbo>`.
   auto h_calloc = allocator.inline_calloc<int4, sbo>().value();
   cat::verify(h_calloc.is_inline());
   auto h_calloc_big =
      allocator.inline_calloc<alloc_huge_object, sbo>().value();
   cat::verify(!h_calloc_big.is_inline());

   // Test `inline_xcalloc<T, sbo>`.
   auto h_xcalloc = allocator.inline_xcalloc<int4, sbo>();
   cat::verify(h_xcalloc.is_inline());
   auto h_xcalloc_big = allocator.inline_xcalloc<alloc_huge_object, sbo>();
   cat::verify(!h_xcalloc_big.is_inline());

   // Test `inline_calloc_multi<T, sbo>`.
   auto h_calloc_multi = allocator.inline_calloc_multi<int4, sbo>(2u).value();
   cat::verify(h_calloc_multi.is_inline());
   auto h_calloc_multi_big =
      allocator.inline_calloc_multi<int4, sbo>(5u).value();
   cat::verify(!h_calloc_multi_big.is_inline());

   // Test `inline_xcalloc_multi<T, sbo>`.
   auto h_xcalloc_multi = allocator.inline_xcalloc_multi<int4, sbo>(2u);
   cat::verify(h_xcalloc_multi.is_inline());
   auto h_xcalloc_multi_big = allocator.inline_xcalloc_multi<int4, sbo>(5u);
   cat::verify(!h_xcalloc_multi_big.is_inline());

   allocator.reset();

   // Test `inline_align_calloc<T, sbo>`.
   auto h_align_calloc = allocator.inline_align_calloc<int4, sbo>(8u).value();
   cat::verify(cat::is_aligned(&allocator.get(h_align_calloc), 8u));
   cat::verify(h_align_calloc.is_inline());
   auto h_align_calloc_big =
      allocator.inline_align_calloc<alloc_huge_object, sbo>(8u).value();
   cat::verify(!h_align_calloc_big.is_inline());

   // Test `inline_align_xcalloc<T, sbo>`.
   auto h_align_xcalloc = allocator.inline_align_xcalloc<int4, sbo>(8u);
   cat::verify(cat::is_aligned(&allocator.get(h_align_xcalloc), 8u));
   cat::verify(h_align_xcalloc.is_inline());
   auto h_align_xcalloc_big =
      allocator.inline_align_xcalloc<alloc_huge_object, sbo>(8u);
   cat::verify(!h_align_xcalloc_big.is_inline());

   // Test `inline_unalign_calloc<T, sbo>`.
   auto h_unalign_calloc = allocator.inline_unalign_calloc<int4, sbo>().value();
   cat::verify(h_unalign_calloc.is_inline());
   auto h_unalign_calloc_big =
      allocator.inline_unalign_calloc<alloc_huge_object, sbo>().value();
   cat::verify(!h_unalign_calloc_big.is_inline());

   // Test `inline_unalign_xcalloc<T, sbo>`.
   auto h_unalign_xcalloc = allocator.inline_unalign_xcalloc<int4, sbo>();
   cat::verify(h_unalign_xcalloc.is_inline());
   auto h_unalign_xcalloc_big =
      allocator.inline_unalign_xcalloc<alloc_huge_object, sbo>();
   cat::verify(!h_unalign_xcalloc_big.is_inline());

   allocator.reset();

   // Test `inline_align_calloc_multi<T, sbo>`.
   auto h_align_calloc_multi =
      allocator.inline_align_calloc_multi<int4, sbo>(8u, 2u).value();
   cat::verify(cat::is_aligned(allocator.get_ptr(h_align_calloc_multi), 8u));
   cat::verify(h_align_calloc_multi.is_inline());
   auto h_align_calloc_multi_big =
      allocator.inline_align_calloc_multi<int4, sbo>(8u, 5u).value();
   cat::verify(!h_align_calloc_multi_big.is_inline());

   // Test `inline_align_xcalloc_multi<T, sbo>`.
   auto h_align_xcalloc_multi =
      allocator.inline_align_xcalloc_multi<int4, sbo>(8u, 2u);
   cat::verify(cat::is_aligned(allocator.get_ptr(h_align_xcalloc_multi), 8u));
   cat::verify(h_align_xcalloc_multi.is_inline());
   auto h_align_xcalloc_multi_big =
      allocator.inline_align_xcalloc_multi<int4, sbo>(8u, 5u);
   cat::verify(!h_align_xcalloc_multi_big.is_inline());

   // Test `inline_unalign_calloc_multi<T, sbo>`.
   auto h_unalign_calloc_multi =
      allocator.inline_unalign_calloc_multi<int4, sbo>(2u).value();
   cat::verify(h_unalign_calloc_multi.is_inline());
   auto h_unalign_calloc_multi_big =
      allocator.inline_unalign_calloc_multi<int4, sbo>(5u).value();
   cat::verify(!h_unalign_calloc_multi_big.is_inline());

   // Test `inline_unalign_xcalloc_multi<T, sbo>`.
   auto h_unalign_xcalloc_multi =
      allocator.inline_unalign_xcalloc_multi<int4, sbo>(2u);
   cat::verify(h_unalign_xcalloc_multi.is_inline());
   auto h_unalign_xcalloc_multi_big =
      allocator.inline_unalign_xcalloc_multi<int4, sbo>(5u);
   cat::verify(!h_unalign_xcalloc_multi_big.is_inline());

   allocator.reset();

   // Test `inline_scalloc<T, sbo>`.
   auto [h_scalloc, h_scalloc_bytes] =
      allocator.inline_scalloc<int4, sbo>().value();
   cat::verify(h_scalloc_bytes == sbo);
   cat::verify(h_scalloc.is_inline());
   auto [h_scalloc_big, h_scalloc_bytes_big] =
      allocator.inline_scalloc<alloc_huge_object, sbo>().value();
   cat::verify(h_scalloc_bytes_big == sizeof(alloc_huge_object));
   cat::verify(!h_scalloc_big.is_inline());

   // Test `inline_xscalloc<T, sbo>`.
   auto [h_xscalloc, h_xscalloc_bytes] = allocator.inline_xscalloc<int4, sbo>();
   cat::verify(h_xscalloc_bytes == sbo);
   cat::verify(h_xscalloc.is_inline());
   auto [h_xscalloc_big, h_xscalloc_bytes_big] =
      allocator.inline_xscalloc<alloc_huge_object, sbo>();
   cat::verify(h_xscalloc_bytes_big == sizeof(alloc_huge_object));
   cat::verify(!h_xscalloc_big.is_inline());

   // Test `inline_scalloc_multi<T, sbo>`. `int4` is 4-byte aligned, so the
   // overflow path may pad the byte count returned by the linear allocator.
   auto [h_scalloc_multi, h_scalloc_multi_bytes] =
      allocator.inline_scalloc_multi<int4, sbo>(2u).value();
   cat::verify(h_scalloc_multi_bytes == sbo);
   cat::verify(h_scalloc_multi.is_inline());
   auto [h_scalloc_multi_big, h_scalloc_multi_bytes_big] =
      allocator.inline_scalloc_multi<int4, sbo>(5u).value();
   cat::verify(h_scalloc_multi_bytes_big >= sizeof(int4) * 5);
   cat::verify(!h_scalloc_multi_big.is_inline());

   // Test `inline_xscalloc_multi<T, sbo>`.
   auto [h_xscalloc_multi, h_xscalloc_multi_bytes] =
      allocator.inline_xscalloc_multi<int4, sbo>(2u);
   cat::verify(h_xscalloc_multi_bytes == sbo);
   cat::verify(h_xscalloc_multi.is_inline());
   auto [h_xscalloc_multi_big, h_xscalloc_multi_bytes_big] =
      allocator.inline_xscalloc_multi<int4, sbo>(5u);
   cat::verify(h_xscalloc_multi_bytes_big >= sizeof(int4) * 5);
   cat::verify(!h_xscalloc_multi_big.is_inline());

   allocator.reset();

   // Test `inline_align_scalloc<T, sbo>`.
   auto [h_align_scalloc, h_align_scalloc_bytes] =
      allocator.inline_align_scalloc<int4, sbo>(8u).value();
   cat::verify(h_align_scalloc_bytes >= sbo);
   cat::verify(h_align_scalloc.is_inline());
   auto [h_align_scalloc_big, h_align_scalloc_bytes_big] =
      allocator.inline_align_scalloc<alloc_huge_object, sbo>(8u).value();
   cat::verify(h_align_scalloc_bytes_big >= sizeof(alloc_huge_object));
   cat::verify(!h_align_scalloc_big.is_inline());

   // Test `inline_align_xscalloc<T, sbo>`.
   auto [h_align_xscalloc, h_align_xscalloc_bytes] =
      allocator.inline_align_xscalloc<int4, sbo>(8u);
   cat::verify(h_align_xscalloc_bytes >= sbo);
   cat::verify(h_align_xscalloc.is_inline());
   auto [h_align_xscalloc_big, h_align_xscalloc_bytes_big] =
      allocator.inline_align_xscalloc<alloc_huge_object, sbo>(8u);
   cat::verify(h_align_xscalloc_bytes_big >= sizeof(alloc_huge_object));
   cat::verify(!h_align_xscalloc_big.is_inline());

   // Test `inline_unalign_scalloc<T, sbo>`.
   auto [h_unalign_scalloc, h_unalign_scalloc_bytes] =
      allocator.inline_unalign_scalloc<int4, sbo>().value();
   cat::verify(h_unalign_scalloc_bytes == sbo);
   cat::verify(h_unalign_scalloc.is_inline());
   auto [h_unalign_scalloc_big, h_unalign_scalloc_bytes_big] =
      allocator.inline_unalign_scalloc<alloc_huge_object, sbo>().value();
   cat::verify(h_unalign_scalloc_bytes_big == sizeof(alloc_huge_object));
   cat::verify(!h_unalign_scalloc_big.is_inline());

   // Test `inline_unalign_xscalloc<T, sbo>`.
   auto [h_unalign_xscalloc, h_unalign_xscalloc_bytes] =
      allocator.inline_unalign_xscalloc<int4, sbo>();
   cat::verify(h_unalign_xscalloc_bytes == sbo);
   cat::verify(h_unalign_xscalloc.is_inline());
   auto [h_unalign_xscalloc_big, h_unalign_xscalloc_bytes_big] =
      allocator.inline_unalign_xscalloc<alloc_huge_object, sbo>();
   cat::verify(h_unalign_xscalloc_bytes_big == sizeof(alloc_huge_object));
   cat::verify(!h_unalign_xscalloc_big.is_inline());

   allocator.reset();

   // Test `inline_align_scalloc_multi<T, sbo>`.
   auto [h_align_scalloc_multi, h_align_scalloc_multi_bytes] =
      allocator.inline_align_scalloc_multi<int4, sbo>(8u, 2u).value();
   cat::verify(h_align_scalloc_multi_bytes >= sbo);
   cat::verify(h_align_scalloc_multi.is_inline());
   auto [h_align_scalloc_multi_big, h_align_scalloc_multi_bytes_big] =
      allocator.inline_align_scalloc_multi<int4, sbo>(8u, 5u).value();
   cat::verify(h_align_scalloc_multi_bytes_big >= sizeof(int4) * 5);
   cat::verify(!h_align_scalloc_multi_big.is_inline());

   // Test `inline_align_xscalloc_multi<T, sbo>`.
   auto [h_align_xscalloc_multi, h_align_xscalloc_multi_bytes] =
      allocator.inline_align_xscalloc_multi<int4, sbo>(8u, 2u);
   cat::verify(h_align_xscalloc_multi_bytes >= sbo);
   cat::verify(h_align_xscalloc_multi.is_inline());
   auto [h_align_xscalloc_multi_big, h_align_xscalloc_multi_bytes_big] =
      allocator.inline_align_xscalloc_multi<int4, sbo>(8u, 5u);
   cat::verify(h_align_xscalloc_multi_bytes_big >= sizeof(int4) * 5);
   cat::verify(!h_align_xscalloc_multi_big.is_inline());

   // Test `inline_unalign_scalloc_multi<T, sbo>`.
   auto [h_unalign_scalloc_multi, h_unalign_scalloc_multi_bytes] =
      allocator.inline_unalign_scalloc_multi<int4, sbo>(2u).value();
   cat::verify(h_unalign_scalloc_multi_bytes == sbo);
   cat::verify(h_unalign_scalloc_multi.is_inline());
   auto [h_unalign_scalloc_multi_big, h_unalign_scalloc_multi_bytes_big] =
      allocator.inline_unalign_scalloc_multi<int4, sbo>(5u).value();
   cat::verify(h_unalign_scalloc_multi_bytes_big == sizeof(int4) * 5);
   cat::verify(!h_unalign_scalloc_multi_big.is_inline());

   // Test `inline_unalign_xscalloc_multi<T, sbo>`.
   auto [h_unalign_xscalloc_multi, h_unalign_xscalloc_multi_bytes] =
      allocator.inline_unalign_xscalloc_multi<int4, sbo>(2u);
   cat::verify(h_unalign_xscalloc_multi_bytes == sbo);
   cat::verify(h_unalign_xscalloc_multi.is_inline());
   auto [h_unalign_xscalloc_multi_big, h_unalign_xscalloc_multi_bytes_big] =
      allocator.inline_unalign_xscalloc_multi<int4, sbo>(5u);
   cat::verify(h_unalign_xscalloc_multi_bytes_big == sizeof(int4) * 5);
   cat::verify(!h_unalign_xscalloc_multi_big.is_inline());

   allocator.reset();

   // The realloc / recalloc / resalloc / rescalloc family infers `inline_size`
   // from the input handle. Each call frees its input, so reseed before every
   // entry point.
   auto seed = [&] {
      return allocator.inline_alloc<int4, sbo>().value();
   };

   // Test `inline_realloc<sbo>` and `inline_realloc_to<sbo>`.
   {
      auto h_seed = seed();
      auto h = allocator.inline_realloc(h_seed).value();
      cat::verify(h.is_inline());
   }
   {
      auto h_seed = seed();
      auto h = allocator.inline_realloc_to(allocator, h_seed).value();
      cat::verify(h.is_inline());
   }

   // Test `inline_xrealloc<sbo>` and `inline_xrealloc_to<sbo>`.
   {
      auto h_seed = seed();
      auto h = allocator.inline_xrealloc(h_seed);
      cat::verify(h.is_inline());
   }
   {
      auto h_seed = seed();
      auto h = allocator.inline_xrealloc_to(allocator, h_seed);
      cat::verify(h.is_inline());
   }

   // Test `inline_align_realloc<sbo>` and `inline_align_realloc_to<sbo>`.
   {
      auto h_seed = seed();
      auto h = allocator.inline_align_realloc(h_seed, 8u).value();
      cat::verify(h.is_inline());
   }
   {
      auto h_seed = seed();
      auto h = allocator.inline_align_realloc_to(allocator, h_seed, 8u).value();
      cat::verify(h.is_inline());
   }

   // Test `inline_align_xrealloc<sbo>` and `inline_align_xrealloc_to<sbo>`.
   {
      auto h_seed = seed();
      auto h = allocator.inline_align_xrealloc(h_seed, 8u);
      cat::verify(h.is_inline());
   }
   {
      auto h_seed = seed();
      auto h = allocator.inline_align_xrealloc_to(allocator, h_seed, 8u);
      cat::verify(h.is_inline());
   }

   // Test `inline_unalign_realloc<sbo>` and `inline_unalign_realloc_to<sbo>`.
   {
      auto h_seed = seed();
      auto h = allocator.inline_unalign_realloc(h_seed).value();
      cat::verify(h.is_inline());
   }
   {
      auto h_seed = seed();
      auto h = allocator.inline_unalign_realloc_to(allocator, h_seed).value();
      cat::verify(h.is_inline());
   }

   // Test `inline_unalign_xrealloc<sbo>` and `inline_unalign_xrealloc_to<sbo>`.
   {
      auto h_seed = seed();
      auto h = allocator.inline_unalign_xrealloc(h_seed);
      cat::verify(h.is_inline());
   }
   {
      auto h_seed = seed();
      auto h = allocator.inline_unalign_xrealloc_to(allocator, h_seed);
      cat::verify(h.is_inline());
   }

   // Test `inline_realloc_multi<sbo>` and `inline_realloc_multi_to<sbo>`.
   {
      auto h_seed = seed();
      auto h = allocator.inline_realloc_multi(h_seed, 2u).value();
      cat::verify(h.is_inline());
   }
   {
      auto h_seed = seed();
      auto h = allocator.inline_realloc_multi_to(allocator, h_seed, 2u).value();
      cat::verify(h.is_inline());
   }

   // Test `inline_xrealloc_multi<sbo>` and `inline_xrealloc_multi_to<sbo>`.
   {
      auto h_seed = seed();
      auto h = allocator.inline_xrealloc_multi(h_seed, 2u);
      cat::verify(h.is_inline());
   }
   {
      auto h_seed = seed();
      auto h = allocator.inline_xrealloc_multi_to(allocator, h_seed, 2u);
      cat::verify(h.is_inline());
   }

   // Test `inline_align_realloc_multi<sbo>` and
   // `inline_align_realloc_multi_to<sbo>`.
   {
      auto h_seed = seed();
      auto h = allocator.inline_align_realloc_multi(h_seed, 8u, 2u).value();
      cat::verify(h.is_inline());
   }
   {
      auto h_seed = seed();
      auto h =
         allocator.inline_align_realloc_multi_to(allocator, h_seed, 8u, 2u)
            .value();
      cat::verify(h.is_inline());
   }

   // Test `inline_align_xrealloc_multi<sbo>` and
   // `inline_align_xrealloc_multi_to<sbo>`.
   {
      auto h_seed = seed();
      auto h = allocator.inline_align_xrealloc_multi(h_seed, 8u, 2u);
      cat::verify(h.is_inline());
   }
   {
      auto h_seed = seed();
      auto h =
         allocator.inline_align_xrealloc_multi_to(allocator, h_seed, 8u, 2u);
      cat::verify(h.is_inline());
   }

   // Test `inline_unalign_realloc_multi<sbo>` and
   // `inline_unalign_realloc_multi_to<sbo>`.
   {
      auto h_seed = seed();
      auto h = allocator.inline_unalign_realloc_multi(h_seed, 2u).value();
      cat::verify(h.is_inline());
   }
   {
      auto h_seed = seed();
      auto h = allocator.inline_unalign_realloc_multi_to(allocator, h_seed, 2u)
                  .value();
      cat::verify(h.is_inline());
   }

   // Test `inline_unalign_xrealloc_multi<sbo>` and
   // `inline_unalign_xrealloc_multi_to<sbo>`.
   {
      auto h_seed = seed();
      auto h = allocator.inline_unalign_xrealloc_multi(h_seed, 2u);
      cat::verify(h.is_inline());
   }
   {
      auto h_seed = seed();
      auto h =
         allocator.inline_unalign_xrealloc_multi_to(allocator, h_seed, 2u);
      cat::verify(h.is_inline());
   }

   allocator.reset();

   // Test `inline_recalloc<sbo>` and `inline_recalloc_to<sbo>`.
   {
      auto h_seed = seed();
      auto h = allocator.inline_recalloc(h_seed).value();
      cat::verify(h.is_inline());
   }
   {
      auto h_seed = seed();
      auto h = allocator.inline_recalloc_to(allocator, h_seed).value();
      cat::verify(h.is_inline());
   }

   // Test `inline_xrecalloc<sbo>` and `inline_xrecalloc_to<sbo>`.
   {
      auto h_seed = seed();
      auto h = allocator.inline_xrecalloc(h_seed);
      cat::verify(h.is_inline());
   }
   {
      auto h_seed = seed();
      auto h = allocator.inline_xrecalloc_to(allocator, h_seed);
      cat::verify(h.is_inline());
   }

   // Test `inline_align_recalloc<sbo>` and `inline_align_recalloc_to<sbo>`.
   {
      auto h_seed = seed();
      auto h = allocator.inline_align_recalloc(h_seed, 8u).value();
      cat::verify(h.is_inline());
   }
   {
      auto h_seed = seed();
      auto h =
         allocator.inline_align_recalloc_to(allocator, h_seed, 8u).value();
      cat::verify(h.is_inline());
   }

   // Test `inline_align_xrecalloc<sbo>` and `inline_align_xrecalloc_to<sbo>`.
   {
      auto h_seed = seed();
      auto h = allocator.inline_align_xrecalloc(h_seed, 8u);
      cat::verify(h.is_inline());
   }
   {
      auto h_seed = seed();
      auto h = allocator.inline_align_xrecalloc_to(allocator, h_seed, 8u);
      cat::verify(h.is_inline());
   }

   // Test `inline_unalign_recalloc<sbo>` and
   // `inline_unalign_recalloc_to<sbo>`.
   {
      auto h_seed = seed();
      auto h = allocator.inline_unalign_recalloc(h_seed).value();
      cat::verify(h.is_inline());
   }
   {
      auto h_seed = seed();
      auto h = allocator.inline_unalign_recalloc_to(allocator, h_seed).value();
      cat::verify(h.is_inline());
   }

   // Test `inline_unalign_xrecalloc<sbo>` and
   // `inline_unalign_xrecalloc_to<sbo>`.
   {
      auto h_seed = seed();
      auto h = allocator.inline_unalign_xrecalloc(h_seed);
      cat::verify(h.is_inline());
   }
   {
      auto h_seed = seed();
      auto h = allocator.inline_unalign_xrecalloc_to(allocator, h_seed);
      cat::verify(h.is_inline());
   }

   // Test `inline_recalloc_multi<sbo>` and `inline_recalloc_multi_to<sbo>`.
   {
      auto h_seed = seed();
      auto h = allocator.inline_recalloc_multi(h_seed, 2u).value();
      cat::verify(h.is_inline());
   }
   {
      auto h_seed = seed();
      auto h =
         allocator.inline_recalloc_multi_to(allocator, h_seed, 2u).value();
      cat::verify(h.is_inline());
   }

   // Test `inline_xrecalloc_multi<sbo>` and `inline_xrecalloc_multi_to<sbo>`.
   {
      auto h_seed = seed();
      auto h = allocator.inline_xrecalloc_multi(h_seed, 2u);
      cat::verify(h.is_inline());
   }
   {
      auto h_seed = seed();
      auto h = allocator.inline_xrecalloc_multi_to(allocator, h_seed, 2u);
      cat::verify(h.is_inline());
   }

   // Test `inline_align_recalloc_multi<sbo>` and
   // `inline_align_recalloc_multi_to<sbo>`.
   {
      auto h_seed = seed();
      auto h = allocator.inline_align_recalloc_multi(h_seed, 8u, 2u).value();
      cat::verify(h.is_inline());
   }
   {
      auto h_seed = seed();
      auto h =
         allocator.inline_align_recalloc_multi_to(allocator, h_seed, 8u, 2u)
            .value();
      cat::verify(h.is_inline());
   }

   // Test `inline_align_xrecalloc_multi<sbo>` and
   // `inline_align_xrecalloc_multi_to<sbo>`.
   {
      auto h_seed = seed();
      auto h = allocator.inline_align_xrecalloc_multi(h_seed, 8u, 2u);
      cat::verify(h.is_inline());
   }
   {
      auto h_seed = seed();
      auto h =
         allocator.inline_align_xrecalloc_multi_to(allocator, h_seed, 8u, 2u);
      cat::verify(h.is_inline());
   }

   // Test `inline_unalign_recalloc_multi<sbo>` and
   // `inline_unalign_recalloc_multi_to<sbo>`.
   {
      auto h_seed = seed();
      auto h = allocator.inline_unalign_recalloc_multi(h_seed, 2u).value();
      cat::verify(h.is_inline());
   }
   {
      auto h_seed = seed();
      auto h = allocator.inline_unalign_recalloc_multi_to(allocator, h_seed, 2u)
                  .value();
      cat::verify(h.is_inline());
   }

   // Test `inline_unalign_xrecalloc_multi<sbo>` and
   // `inline_unalign_xrecalloc_multi_to<sbo>`.
   {
      auto h_seed = seed();
      auto h = allocator.inline_unalign_xrecalloc_multi(h_seed, 2u);
      cat::verify(h.is_inline());
   }
   {
      auto h_seed = seed();
      auto h =
         allocator.inline_unalign_xrecalloc_multi_to(allocator, h_seed, 2u);
      cat::verify(h.is_inline());
   }

   allocator.reset();

   // Test `inline_resalloc<sbo>` and `inline_resalloc_to<sbo>`.
   {
      auto h_seed = seed();
      auto [h, bytes] = allocator.inline_resalloc(h_seed).value();
      cat::verify(h.is_inline());
      cat::verify(bytes == sbo);
   }
   {
      auto h_seed = seed();
      auto [h, bytes] = allocator.inline_resalloc_to(allocator, h_seed).value();
      cat::verify(h.is_inline());
      cat::verify(bytes == sbo);
   }

   // Test `inline_xresalloc<sbo>` and `inline_xresalloc_to<sbo>`.
   {
      auto h_seed = seed();
      auto [h, bytes] = allocator.inline_xresalloc(h_seed);
      cat::verify(h.is_inline());
      cat::verify(bytes == sbo);
   }
   {
      auto h_seed = seed();
      auto [h, bytes] = allocator.inline_xresalloc_to(allocator, h_seed);
      cat::verify(h.is_inline());
      cat::verify(bytes == sbo);
   }

   // Test `inline_align_resalloc<sbo>` and `inline_align_resalloc_to<sbo>`.
   {
      auto h_seed = seed();
      auto [h, bytes] = allocator.inline_align_resalloc(h_seed, 8u).value();
      cat::verify(h.is_inline());
      cat::verify(bytes >= sbo);
   }
   {
      auto h_seed = seed();
      auto [h, bytes] =
         allocator.inline_align_resalloc_to(allocator, h_seed, 8u).value();
      cat::verify(h.is_inline());
      cat::verify(bytes >= sbo);
   }

   // Test `inline_align_xresalloc<sbo>` and `inline_align_xresalloc_to<sbo>`.
   {
      auto h_seed = seed();
      auto [h, bytes] = allocator.inline_align_xresalloc(h_seed, 8u);
      cat::verify(h.is_inline());
      cat::verify(bytes >= sbo);
   }
   {
      auto h_seed = seed();
      auto [h, bytes] =
         allocator.inline_align_xresalloc_to(allocator, h_seed, 8u);
      cat::verify(h.is_inline());
      cat::verify(bytes >= sbo);
   }

   // Test `inline_unalign_resalloc<sbo>` and
   // `inline_unalign_resalloc_to<sbo>`.
   {
      auto h_seed = seed();
      auto [h, bytes] = allocator.inline_unalign_resalloc(h_seed).value();
      cat::verify(h.is_inline());
      cat::verify(bytes == sbo);
   }
   {
      auto h_seed = seed();
      auto [h, bytes] =
         allocator.inline_unalign_resalloc_to(allocator, h_seed).value();
      cat::verify(h.is_inline());
      cat::verify(bytes == sbo);
   }

   // Test `inline_unalign_xresalloc<sbo>` and
   // `inline_unalign_xresalloc_to<sbo>`.
   {
      auto h_seed = seed();
      auto [h, bytes] = allocator.inline_unalign_xresalloc(h_seed);
      cat::verify(h.is_inline());
      cat::verify(bytes == sbo);
   }
   {
      auto h_seed = seed();
      auto [h, bytes] =
         allocator.inline_unalign_xresalloc_to(allocator, h_seed);
      cat::verify(h.is_inline());
      cat::verify(bytes == sbo);
   }

   // Test `inline_resalloc_multi<sbo>` and `inline_resalloc_multi_to<sbo>`.
   {
      auto h_seed = seed();
      auto [h, bytes] = allocator.inline_resalloc_multi(h_seed, 2u).value();
      cat::verify(h.is_inline());
      cat::verify(bytes == sbo);
   }
   {
      auto h_seed = seed();
      auto [h, bytes] =
         allocator.inline_resalloc_multi_to(allocator, h_seed, 2u).value();
      cat::verify(h.is_inline());
      cat::verify(bytes == sbo);
   }

   // Test `inline_xresalloc_multi<sbo>` and `inline_xresalloc_multi_to<sbo>`.
   {
      auto h_seed = seed();
      auto [h, bytes] = allocator.inline_xresalloc_multi(h_seed, 2u);
      cat::verify(h.is_inline());
      cat::verify(bytes == sbo);
   }
   {
      auto h_seed = seed();
      auto [h, bytes] =
         allocator.inline_xresalloc_multi_to(allocator, h_seed, 2u);
      cat::verify(h.is_inline());
      cat::verify(bytes == sbo);
   }

   // Test `inline_align_resalloc_multi<sbo>` and
   // `inline_align_resalloc_multi_to<sbo>`.
   {
      auto h_seed = seed();
      auto [h, bytes] =
         allocator.inline_align_resalloc_multi(h_seed, 8u, 2u).value();
      cat::verify(h.is_inline());
      cat::verify(bytes >= sbo);
   }
   {
      auto h_seed = seed();
      auto [h, bytes] =
         allocator.inline_align_resalloc_multi_to(allocator, h_seed, 8u, 2u)
            .value();
      cat::verify(h.is_inline());
      cat::verify(bytes >= sbo);
   }

   // Test `inline_align_xresalloc_multi<sbo>` and
   // `inline_align_xresalloc_multi_to<sbo>`.
   {
      auto h_seed = seed();
      auto [h, bytes] = allocator.inline_align_xresalloc_multi(h_seed, 8u, 2u);
      cat::verify(h.is_inline());
      cat::verify(bytes >= sbo);
   }
   {
      auto h_seed = seed();
      auto [h, bytes] =
         allocator.inline_align_xresalloc_multi_to(allocator, h_seed, 8u, 2u);
      cat::verify(h.is_inline());
      cat::verify(bytes >= sbo);
   }

   // Test `inline_unalign_resalloc_multi<sbo>` and
   // `inline_unalign_resalloc_multi_to<sbo>`.
   {
      auto h_seed = seed();
      auto [h, bytes] =
         allocator.inline_unalign_resalloc_multi(h_seed, 2u).value();
      cat::verify(h.is_inline());
      cat::verify(bytes == sbo);
   }
   {
      auto h_seed = seed();
      auto [h, bytes] =
         allocator.inline_unalign_resalloc_multi_to(allocator, h_seed, 2u)
            .value();
      cat::verify(h.is_inline());
      cat::verify(bytes == sbo);
   }

   // Test `inline_unalign_xresalloc_multi<sbo>` and
   // `inline_unalign_xresalloc_multi_to<sbo>`.
   {
      auto h_seed = seed();
      auto [h, bytes] = allocator.inline_unalign_xresalloc_multi(h_seed, 2u);
      cat::verify(h.is_inline());
      cat::verify(bytes == sbo);
   }
   {
      auto h_seed = seed();
      auto [h, bytes] =
         allocator.inline_unalign_xresalloc_multi_to(allocator, h_seed, 2u);
      cat::verify(h.is_inline());
      cat::verify(bytes == sbo);
   }

   allocator.reset();

   // Test `inline_rescalloc<sbo>` and `inline_rescalloc_to<sbo>`.
   {
      auto h_seed = seed();
      auto [h, bytes] = allocator.inline_rescalloc(h_seed).value();
      cat::verify(h.is_inline());
      cat::verify(bytes == sbo);
   }
   {
      auto h_seed = seed();
      auto [h, bytes] =
         allocator.inline_rescalloc_to(allocator, h_seed).value();
      cat::verify(h.is_inline());
      cat::verify(bytes == sbo);
   }

   // Test `inline_xrescalloc<sbo>` and `inline_xrescalloc_to<sbo>`.
   {
      auto h_seed = seed();
      auto [h, bytes] = allocator.inline_xrescalloc(h_seed);
      cat::verify(h.is_inline());
      cat::verify(bytes == sbo);
   }
   {
      auto h_seed = seed();
      auto [h, bytes] = allocator.inline_xrescalloc_to(allocator, h_seed);
      cat::verify(h.is_inline());
      cat::verify(bytes == sbo);
   }

   // Test `inline_align_rescalloc<sbo>` and `inline_align_rescalloc_to<sbo>`.
   {
      auto h_seed = seed();
      auto [h, bytes] = allocator.inline_align_rescalloc(h_seed, 8u).value();
      cat::verify(h.is_inline());
      cat::verify(bytes >= sbo);
   }
   {
      auto h_seed = seed();
      auto [h, bytes] =
         allocator.inline_align_rescalloc_to(allocator, h_seed, 8u).value();
      cat::verify(h.is_inline());
      cat::verify(bytes >= sbo);
   }

   // Test `inline_align_xrescalloc<sbo>` and
   // `inline_align_xrescalloc_to<sbo>`.
   {
      auto h_seed = seed();
      auto [h, bytes] = allocator.inline_align_xrescalloc(h_seed, 8u);
      cat::verify(h.is_inline());
      cat::verify(bytes >= sbo);
   }
   {
      auto h_seed = seed();
      auto [h, bytes] =
         allocator.inline_align_xrescalloc_to(allocator, h_seed, 8u);
      cat::verify(h.is_inline());
      cat::verify(bytes >= sbo);
   }

   // Test `inline_unalign_rescalloc<sbo>` and
   // `inline_unalign_rescalloc_to<sbo>`.
   {
      auto h_seed = seed();
      auto [h, bytes] = allocator.inline_unalign_rescalloc(h_seed).value();
      cat::verify(h.is_inline());
      cat::verify(bytes == sbo);
   }
   {
      auto h_seed = seed();
      auto [h, bytes] =
         allocator.inline_unalign_rescalloc_to(allocator, h_seed).value();
      cat::verify(h.is_inline());
      cat::verify(bytes == sbo);
   }

   // Test `inline_unalign_xrescalloc<sbo>` and
   // `inline_unalign_xrescalloc_to<sbo>`.
   {
      auto h_seed = seed();
      auto [h, bytes] = allocator.inline_unalign_xrescalloc(h_seed);
      cat::verify(h.is_inline());
      cat::verify(bytes == sbo);
   }
   {
      auto h_seed = seed();
      auto [h, bytes] =
         allocator.inline_unalign_xrescalloc_to(allocator, h_seed);
      cat::verify(h.is_inline());
      cat::verify(bytes == sbo);
   }

   // Test `inline_rescalloc_multi<sbo>` and `inline_rescalloc_multi_to<sbo>`.
   {
      auto h_seed = seed();
      auto [h, bytes] = allocator.inline_rescalloc_multi(h_seed, 2u).value();
      cat::verify(h.is_inline());
      cat::verify(bytes == sbo);
   }
   {
      auto h_seed = seed();
      auto [h, bytes] =
         allocator.inline_rescalloc_multi_to(allocator, h_seed, 2u).value();
      cat::verify(h.is_inline());
      cat::verify(bytes == sbo);
   }

   // Test `inline_xrescalloc_multi<sbo>` and `inline_xrescalloc_multi_to<sbo>`.
   {
      auto h_seed = seed();
      auto [h, bytes] = allocator.inline_xrescalloc_multi(h_seed, 2u);
      cat::verify(h.is_inline());
      cat::verify(bytes == sbo);
   }
   {
      auto h_seed = seed();
      auto [h, bytes] =
         allocator.inline_xrescalloc_multi_to(allocator, h_seed, 2u);
      cat::verify(h.is_inline());
      cat::verify(bytes == sbo);
   }

   // Test `inline_align_rescalloc_multi<sbo>` and
   // `inline_align_rescalloc_multi_to<sbo>`.
   {
      auto h_seed = seed();
      auto [h, bytes] =
         allocator.inline_align_rescalloc_multi(h_seed, 8u, 2u).value();
      cat::verify(h.is_inline());
      cat::verify(bytes >= sbo);
   }
   {
      auto h_seed = seed();
      auto [h, bytes] =
         allocator.inline_align_rescalloc_multi_to(allocator, h_seed, 8u, 2u)
            .value();
      cat::verify(h.is_inline());
      cat::verify(bytes >= sbo);
   }

   // Test `inline_align_xrescalloc_multi<sbo>` and
   // `inline_align_xrescalloc_multi_to<sbo>`.
   {
      auto h_seed = seed();
      auto [h, bytes] = allocator.inline_align_xrescalloc_multi(h_seed, 8u, 2u);
      cat::verify(h.is_inline());
      cat::verify(bytes >= sbo);
   }
   {
      auto h_seed = seed();
      auto [h, bytes] =
         allocator.inline_align_xrescalloc_multi_to(allocator, h_seed, 8u, 2u);
      cat::verify(h.is_inline());
      cat::verify(bytes >= sbo);
   }

   // Test `inline_unalign_rescalloc_multi<sbo>` and
   // `inline_unalign_rescalloc_multi_to<sbo>`.
   {
      auto h_seed = seed();
      auto [h, bytes] =
         allocator.inline_unalign_rescalloc_multi(h_seed, 2u).value();
      cat::verify(h.is_inline());
      cat::verify(bytes == sbo);
   }
   {
      auto h_seed = seed();
      auto [h, bytes] =
         allocator.inline_unalign_rescalloc_multi_to(allocator, h_seed, 2u)
            .value();
      cat::verify(h.is_inline());
      cat::verify(bytes == sbo);
   }

   // Test `inline_unalign_xrescalloc_multi<sbo>` and
   // `inline_unalign_xrescalloc_multi_to<sbo>`.
   {
      auto h_seed = seed();
      auto [h, bytes] = allocator.inline_unalign_xrescalloc_multi(h_seed, 2u);
      cat::verify(h.is_inline());
      cat::verify(bytes == sbo);
   }
   {
      auto h_seed = seed();
      auto [h, bytes] =
         allocator.inline_unalign_xrescalloc_multi_to(allocator, h_seed, 2u);
      cat::verify(h.is_inline());
      cat::verify(bytes == sbo);
   }
}
