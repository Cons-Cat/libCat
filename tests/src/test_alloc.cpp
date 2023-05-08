#include <cat/bit>
#include <cat/linear_allocator>
#include <cat/page_allocator>

#include "../unit_tests.hpp"

struct alloc_huge_object {
    [[maybe_unused]] uint1 storage[cat::inline_buffer_size.raw + 1];
};

int4 alloc_counter = 0;

struct alloc_non_trivial {
    char storage;

    alloc_non_trivial() {
        ++alloc_counter;
    }
};

struct alloc_non_trivial_huge_object {
    [[maybe_unused]] uint1 storage[cat::inline_buffer_size.raw];

    alloc_non_trivial_huge_object() {
        ++alloc_counter;
    }
};

consteval void const_test() {
    cat::page_allocator allocator;

    int4* p_alloc = allocator.alloc<int4>(1).value();
    allocator.free(p_alloc);

    int4* p_xalloc = allocator.xalloc<int4>(1);
    allocator.free(p_xalloc);

    cat::span<int4> alloc_multi = allocator.alloc_multi<int4>(5u).value();

    alloc_multi =
        allocator.realloc_multi(alloc_multi.data(), alloc_multi.size(), 10u)
            .value();

    allocator.free_multi(alloc_multi.data(), alloc_multi.size());

    cat::span<int4> xalloc_multi = allocator.xalloc_multi<int4>(5u);
    allocator.free_multi(xalloc_multi.data(), xalloc_multi.size());
}

TEST(test_alloc) {
#ifndef __clang__
    // This does not compile with Clang, currently.
    const_test();
#endif

    // Initialize an allocator.
    cat::page_allocator pager;
    pager.reset();
    // Page the kernel for a linear allocator to test with.
    cat::span page = pager.alloc_multi<cat::byte>(4_uki - 64u).or_exit();
    defer(pager.free(page.data());)
    auto allocator =
        cat::linear_allocator::backed(pager, page.size()).or_exit();

    // Test `alloc`.
    _ = allocator.alloc<int4>().value();
    auto* p_alloc = allocator.alloc<int4>(1).value();
    cat::verify(*p_alloc == 1);

    // Test `xalloc`.
    _ = allocator.xalloc<int4>();
    auto* p_xalloc = allocator.xalloc<int4>(1);
    cat::verify(*p_xalloc == 1);

    // Test `alloc_multi`.
    cat::span<int4> alloc_multi = allocator.alloc_multi<int4>(5u).value();
    cat::verify(alloc_multi.size() == 5);

    // Test `xalloc_multi`.
    alloc_multi = allocator.xalloc_multi<int4>(5u);
    cat::verify(alloc_multi.size() == 5);

    // Test `align_alloc`.
    _ = allocator.align_alloc<int4>(8u).value();
    auto* p_align_alloc = allocator.align_alloc<int4>(8u, 1).value();
    cat::verify(*p_align_alloc == 1);
    cat::verify(cat::is_aligned(p_align_alloc, 8u));

    // Test `align_xalloc`.
    _ = allocator.align_xalloc<int4>(8u);
    auto* p_align_xalloc = allocator.align_xalloc<int4>(8u, 1);
    cat::verify(*p_align_xalloc == 1);
    cat::verify(cat::is_aligned(p_align_xalloc, 8u));

    // Test `unalign_alloc`.
    _ = allocator.unalign_alloc<int4>(8u).value();
    auto* p_unalign_alloc = allocator.unalign_alloc<int4>(1).value();
    cat::verify(*p_unalign_alloc == 1);

    // Test `unalign_xalloc`.
    _ = allocator.unalign_xalloc<int4>(8u);
    auto* p_unalign_xalloc = allocator.unalign_xalloc<int4>(1);
    cat::verify(*p_unalign_xalloc == 1);

    // Test `align_alloc_multi`.
    auto* p_align_alloc_multi =
        allocator.align_alloc_multi<int4>(8u, 5u).value().data();
    cat::verify(cat::is_aligned(p_align_alloc_multi, 8u));
    alloc_counter = 0;
    _ = allocator.align_alloc_multi<alloc_non_trivial>(8u, 5u);
    cat::verify(alloc_counter == 5);

    // Test `align_xalloc_multi`.
    _ = allocator.align_xalloc_multi<int4>(8u, 5u);
    alloc_counter = 0;
    _ = allocator.align_xalloc_multi<alloc_non_trivial>(8u, 5u);
    cat::verify(alloc_counter == 5);

    // Test `unalign_alloc_multi`.
    _ = allocator.unalign_alloc_multi<int1>(5u)
            .value();  // `int4` is 4-byte aligned.
    alloc_counter = 0;
    _ = allocator.unalign_alloc_multi<alloc_non_trivial>(5u);
    cat::verify(alloc_counter == 5);

    // Test `unalign_xalloc_multi`.
    _ = allocator.unalign_xalloc_multi<int1>(5u);  // `int4` is 4-byte aligned.
    alloc_counter = 0;
    _ = allocator.unalign_xalloc_multi<alloc_non_trivial>(5u);
    cat::verify(alloc_counter == 5);

    // Test `inline_alloc`.
    _ = allocator.inline_alloc<int4>().value();
    auto inline_alloc = allocator.inline_alloc<int4>(1).value();
    cat::verify(allocator.get(inline_alloc) == 1);
    cat::verify(inline_alloc.is_inline());
    alloc_counter = 0;
    _ = allocator.inline_alloc<alloc_non_trivial>();
    cat::verify(alloc_counter == 1);

    // `alloc_huge_object` is larger than the inline buffer.
    auto inline_alloc_2 = allocator.inline_alloc<alloc_huge_object>().value();
    cat::verify(!inline_alloc_2.is_inline());

    alloc_counter = 0;
    _ = allocator.inline_alloc<alloc_non_trivial_huge_object>();
    cat::verify(alloc_counter == 1);

    // Test `inline_xalloc`.
    _ = allocator.inline_xalloc<int4>();
    auto inline_xalloc = allocator.inline_xalloc<int4>(1);
    cat::verify(allocator.get(inline_xalloc) == 1);

    // Test `inline_alloc_multi`.
    auto inline_alloc_multi = allocator.inline_alloc_multi<int4>(5u).value();
    cat::verify(inline_alloc_multi.size() == 5);
    alloc_counter = 0;
    _ = allocator.inline_alloc_multi<alloc_non_trivial>(5u);
    cat::verify(alloc_counter == 5);

    // Test `inline_xalloc_multi`.
    auto inline_xalloc_multi = allocator.inline_xalloc_multi<int4>(5u);
    cat::verify(inline_xalloc_multi.size() == 5);
    alloc_counter = 0;
    _ = allocator.inline_xalloc_multi<alloc_non_trivial>(5u);
    cat::verify(alloc_counter == 5);

    // Test `inline_align_alloc`.
    _ = allocator.inline_align_alloc<int4>(8u).value();
    auto inline_align_alloc = allocator.inline_align_alloc<int4>(8u, 1).value();
    cat::verify(allocator.get(inline_align_alloc) == 1);
    cat::verify(cat::is_aligned(&allocator.get(inline_align_alloc), 8u));
    cat::verify(inline_align_alloc.is_inline());

    // Test `inline_unalign_alloc`.
    _ = allocator.inline_unalign_alloc<int4>(8u).value();
    auto inline_unalign_alloc = allocator.inline_unalign_alloc<int4>(1).value();
    cat::verify(allocator.get(inline_unalign_alloc) == 1);
    cat::verify(inline_unalign_alloc.is_inline());

    // Test `inline_unalign_xalloc`.
    _ = allocator.inline_unalign_xalloc<int4>(8u);
    auto inline_unalign_xalloc = allocator.inline_unalign_xalloc<int4>(1);
    cat::verify(allocator.get(inline_unalign_xalloc) == 1);
    cat::verify(inline_unalign_xalloc.is_inline());

    allocator.reset();

    // Test `inline_align_alloc_multi`.
    auto inline_align_alloc_multi =
        allocator.inline_align_alloc_multi<int4>(8u, 5u).value();
    cat::verify(cat::is_aligned(allocator.p_get(inline_align_alloc_multi), 8u));
    cat::verify(inline_align_alloc_multi.is_inline());

    auto inline_align_alloc_multi_big =
        allocator.inline_align_alloc_multi<int4>(8u, 64u).value();
    cat::verify(!inline_align_alloc_multi_big.is_inline());

    // Test `inline_align_xalloc_multi`.
    auto inline_align_xalloc_multi =
        allocator.inline_align_xalloc_multi<int4>(8u, 5u);
    cat::verify(
        cat::is_aligned(allocator.p_get(inline_align_xalloc_multi), 8u));
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
    cat::verify(inline_nalloc == cat::inline_buffer_size);
    iword inline_nalloc_big =
        allocator.inline_nalloc<alloc_huge_object>().value();
    cat::verify(inline_nalloc_big == 257);

    // Test `inline_xnalloc`.
    allocator.reset();
    iword inline_xnalloc = allocator.inline_xnalloc<int4>();
    cat::verify(inline_xnalloc == cat::inline_buffer_size);
    iword inline_xnalloc_big = allocator.inline_xnalloc<alloc_huge_object>();
    cat::verify(inline_xnalloc_big == 257);

    // Test `inline_nalloc_multi`.
    allocator.reset();
    iword inline_nalloc_multi = allocator.inline_nalloc_multi<int4>(5u).value();
    cat::verify(inline_nalloc_multi == cat::inline_buffer_size);
    iword inline_nalloc_multi_big =
        allocator.inline_nalloc_multi<alloc_huge_object>(2u).value();
    cat::verify(inline_nalloc_multi_big == (257 * 2));

    // Test `inline_xnalloc_multi`.
    allocator.reset();
    iword inline_xnalloc_multi = allocator.inline_xnalloc_multi<int4>(5u);
    cat::verify(inline_xnalloc_multi == cat::inline_buffer_size);
    iword inline_xnalloc_multi_big =
        allocator.inline_xnalloc_multi<alloc_huge_object>(2u);
    cat::verify(inline_xnalloc_multi_big == (257 * 2));

    // Test `inline_align_nalloc`.
    allocator.reset();
    iword inline_align_nalloc = allocator.inline_align_nalloc<int4>(4u).value();
    cat::verify(inline_align_nalloc == cat::inline_buffer_size);
    iword inline_align_nalloc_big =
        allocator.inline_align_nalloc<alloc_huge_object>(1u).value();
    cat::verify(inline_align_nalloc_big == 257);

    // Test `inline_align_xnalloc`.
    allocator.reset();
    iword inline_align_xnalloc = allocator.inline_align_xnalloc<int4>(4u);
    cat::verify(inline_align_xnalloc == cat::inline_buffer_size);
    iword inline_align_xnalloc_big =
        allocator.inline_align_xnalloc<alloc_huge_object>(1u);
    cat::verify(inline_align_xnalloc_big == 257);

    // Test `inline_unalign_nalloc`.
    allocator.reset();
    iword inline_unalign_nalloc =
        allocator.inline_unalign_nalloc<int4>().value();
    cat::verify(inline_unalign_nalloc == cat::inline_buffer_size);
    iword inline_unalign_nalloc_big =
        allocator.inline_unalign_nalloc<alloc_huge_object>().value();
    cat::verify(inline_unalign_nalloc_big == 257);

    // Test `inline_unalign_xnalloc`.
    allocator.reset();
    iword inline_unalign_xnalloc = allocator.inline_unalign_xnalloc<int4>();
    cat::verify(inline_unalign_xnalloc == cat::inline_buffer_size);
    iword inline_unalign_xnalloc_big =
        allocator.inline_unalign_xnalloc<alloc_huge_object>();
    cat::verify(inline_unalign_xnalloc_big == 257);

    // Test `inline_align_nalloc_multi`.
    allocator.reset();
    iword inline_align_nalloc_multi =
        allocator.inline_align_nalloc_multi<int4>(4u, 5u).value();
    cat::verify(inline_align_nalloc_multi == cat::inline_buffer_size);
    iword inline_align_nalloc_multi_big =
        allocator.inline_align_nalloc_multi<alloc_huge_object>(1u, 2u).value();
    cat::verify(inline_align_nalloc_multi_big == (257 * 2));

    // Test `inline_align_xnalloc_multi`.
    allocator.reset();
    iword inline_align_xnalloc_multi =
        allocator.inline_align_xnalloc_multi<int4>(4u, 5u);
    cat::verify(inline_align_xnalloc_multi == cat::inline_buffer_size);
    iword inline_align_xnalloc_multi_big =
        allocator.inline_align_xnalloc_multi<alloc_huge_object>(1u, 2u);
    cat::verify(inline_align_xnalloc_multi_big == (257 * 2));

    // Test `inline_unalign_nalloc_multi`.
    allocator.reset();
    iword inline_unalign_nalloc_multi =
        allocator.inline_unalign_nalloc_multi<int4>(5u).value();
    cat::verify(inline_unalign_nalloc_multi == cat::inline_buffer_size);
    iword inline_unalign_nalloc_multi_big =
        allocator.inline_unalign_nalloc_multi<alloc_huge_object>(2u).value();
    cat::verify(inline_unalign_nalloc_multi_big == (257 * 2));

    // Test `inline_unalign_xnalloc_multi`.
    allocator.reset();
    iword inline_unalign_xnalloc_multi =
        allocator.inline_unalign_xnalloc_multi<int4>(5u);
    cat::verify(inline_unalign_xnalloc_multi == cat::inline_buffer_size);
    iword inline_unalign_xnalloc_multi_big =
        allocator.inline_unalign_xnalloc_multi<alloc_huge_object>(2u);
    cat::verify(inline_unalign_xnalloc_multi_big == (257 * 2));

    alloc_counter = 0;
    _ = allocator.salloc_multi<alloc_non_trivial>(5u);
    cat::verify(alloc_counter == 5);

    alloc_counter = 0;
    _ = allocator.xsalloc_multi<alloc_non_trivial>(5u);
    cat::verify(alloc_counter == 5);

    // Test `align_salloc`.
    _ = allocator.align_salloc<int4>(8u);
    allocator.reset();
    auto [p_align_salloc, p_align_salloc_bytes] =
        allocator.align_salloc<int4>(8u, 1).value();
    cat::verify(*p_align_salloc == 1);
    cat::verify(p_align_salloc_bytes == 8);
    cat::verify(cat::is_aligned(p_align_salloc, 8u));

    alloc_counter = 0;
    _ = allocator.align_salloc<alloc_non_trivial>(8u);
    cat::verify(alloc_counter == 1);

    // Test `align_xsalloc`.
    _ = allocator.align_xsalloc<int4>(8u);
    allocator.reset();
    auto [p_align_xsalloc, p_align_xsalloc_bytes] =
        allocator.align_xsalloc<int4>(8u, 1);
    cat::verify(*p_align_xsalloc == 1);
    cat::verify(p_align_xsalloc_bytes == 8);
    cat::verify(cat::is_aligned(p_align_xsalloc, 8u));

    alloc_counter = 0;
    _ = allocator.align_xsalloc<alloc_non_trivial>(8u);
    cat::verify(alloc_counter == 1);

    // Test `unalign_salloc`.
    _ = allocator.unalign_salloc<int1>();
    allocator.reset();
    auto [p_unalign_salloc, p_unalign_salloc_bytes] =
        allocator.unalign_salloc<int1>(1).value();
    cat::verify(*p_unalign_salloc == 1);
    cat::verify(p_unalign_salloc_bytes == 1);

    alloc_counter = 0;
    _ = allocator.unalign_salloc<alloc_non_trivial>();
    cat::verify(alloc_counter == 1);

    // Test `unalign_xsalloc`.
    _ = allocator.unalign_xsalloc<int1>();
    allocator.reset();
    auto [p_unalign_xsalloc, p_unalign_xsalloc_bytes] =
        allocator.unalign_xsalloc<int1>(1);
    cat::verify(*p_unalign_xsalloc == 1);
    cat::verify(p_unalign_xsalloc_bytes == 1);

    alloc_counter = 0;
    _ = allocator.unalign_xsalloc<alloc_non_trivial>();
    cat::verify(alloc_counter == 1);

    // Test `align_salloc_multi`.
    allocator.reset();
    auto [p_align_salloc_multi, p_align_salloc_multi_bytes] =
        allocator.align_salloc_multi<int4>(8u, 5u).value();
    cat::verify(p_align_salloc_multi_bytes == 24);
    cat::verify(cat::is_aligned(p_align_salloc_multi.data(), 8u));

    alloc_counter = 0;
    _ = allocator.align_salloc_multi<alloc_non_trivial>(8u, 5u);
    cat::verify(alloc_counter == 5);

    // Test `align_xsalloc_multi`.
    allocator.reset();
    auto [p_align_xsalloc_multi, p_align_xsalloc_multi_bytes] =
        allocator.align_xsalloc_multi<int4>(8u, 5u);
    cat::verify(p_align_xsalloc_multi_bytes == 24);
    cat::verify(cat::is_aligned(p_align_xsalloc_multi.data(), 8u));

    alloc_counter = 0;
    _ = allocator.align_xsalloc_multi<alloc_non_trivial>(8u, 5u);
    cat::verify(alloc_counter == 5);

    // Test `unalign_salloc_multi`.
    allocator.reset();
    auto [p_unalign_salloc_multi, p_unalign_salloc_multi_bytes] =
        allocator.unalign_salloc_multi<int1>(5u).value();
    cat::verify(p_unalign_salloc_multi_bytes == 5);

    alloc_counter = 0;
    _ = allocator.unalign_salloc_multi<alloc_non_trivial>(5u);
    cat::verify(alloc_counter == 5);

    // Test `unalign_xsalloc_multi`.
    allocator.reset();
    auto [p_unalign_xsalloc_multi, p_unalign_xsalloc_multi_bytes] =
        allocator.unalign_xsalloc_multi<int1>(5u);
    cat::verify(p_unalign_xsalloc_multi_bytes == 5);

    alloc_counter = 0;
    _ = allocator.unalign_xsalloc_multi<alloc_non_trivial>(5u);
    cat::verify(alloc_counter == 5);

    // Test `inline_salloc`.
    allocator.reset();
    auto [inline_salloc, inline_salloc_bytes] =
        allocator.inline_salloc<int4>(1).value();
    cat::verify(allocator.get(inline_salloc) == 1);
    cat::verify(inline_salloc_bytes == cat::inline_buffer_size);
    cat::verify(inline_salloc.is_inline());

    auto [inline_salloc_big, inline_salloc_bytes_big] =
        allocator.inline_salloc<alloc_huge_object>().value();
    cat::verify(inline_salloc_bytes_big == ssizeof(alloc_huge_object));
    cat::verify(!inline_salloc_big.is_inline());

    // Test `inline_xsalloc`.
    allocator.reset();
    auto [inline_xsalloc, inline_xsalloc_bytes] =
        allocator.inline_xsalloc<int4>(1);
    cat::verify(inline_xsalloc_bytes == cat::inline_buffer_size);
    cat::verify(inline_xsalloc.is_inline());

    auto [inline_xsalloc_big, inline_xsalloc_bytes_big] =
        allocator.inline_xsalloc<alloc_huge_object>();
    cat::verify(inline_xsalloc_bytes_big == ssizeof(alloc_huge_object));
    cat::verify(!inline_xsalloc_big.is_inline());

    // Test `inline_salloc_multi`.
    allocator.reset();
    auto [inline_salloc_multi, inline_salloc_multi_bytes] =
        allocator.inline_salloc_multi<int4>(5u).value();
    cat::verify(inline_salloc_multi_bytes == cat::inline_buffer_size);
    cat::verify(inline_salloc_multi.is_inline());

    auto [inline_salloc_multi_big, inline_salloc_multi_bytes_big] =
        allocator.inline_salloc_multi<alloc_huge_object>(5u).value();
    cat::verify(inline_salloc_multi_bytes_big ==
                ssizeof(alloc_huge_object) * 5);
    cat::verify(!inline_salloc_multi_big.is_inline());

    // Test `inline_xsalloc_multi`.
    allocator.reset();
    auto [inline_xsalloc_multi, inline_xsalloc_multi_bytes] =
        allocator.inline_xsalloc_multi<int4>(5u);
    cat::verify(inline_xsalloc_multi_bytes == cat::inline_buffer_size);
    cat::verify(inline_xsalloc_multi.is_inline());

    auto [inline_xsalloc_multi_big, inline_xsalloc_multi_bytes_big] =
        allocator.inline_xsalloc_multi<alloc_huge_object>(5u);
    cat::verify(inline_xsalloc_multi_bytes_big ==
                ssizeof(alloc_huge_object) * 5);
    cat::verify(!inline_xsalloc_multi_big.is_inline());

    // Test `inline_align_salloc`.
    allocator.reset();
    auto [inline_align_salloc, inline_align_salloc_bytes] =
        allocator.inline_align_salloc<int4>(8u, 1).value();
    cat::verify(allocator.get(inline_align_salloc) == 1);
    cat::verify(inline_align_salloc_bytes >= cat::inline_buffer_size);
    cat::verify(inline_align_salloc.is_inline());

    auto [inline_align_salloc_big, inline_align_salloc_bytes_big] =
        allocator.inline_align_salloc<alloc_huge_object>(8u).value();
    cat::verify(inline_align_salloc_bytes_big >= ssizeof(alloc_huge_object));
    cat::verify(!inline_align_salloc_big.is_inline());

    // Test `inline_align_xsalloc`.
    allocator.reset();
    auto [inline_align_xsalloc, inline_align_xsalloc_bytes] =
        allocator.inline_align_xsalloc<int4>(8u, 1);
    cat::verify(allocator.get(inline_align_xsalloc) == 1);
    cat::verify(inline_align_xsalloc_bytes >= cat::inline_buffer_size);
    cat::verify(inline_align_xsalloc.is_inline());

    auto [inline_align_xsalloc_big, inline_align_xsalloc_bytes_big] =
        allocator.inline_align_xsalloc<alloc_huge_object>(8u);
    cat::verify(inline_align_xsalloc_bytes_big >= ssizeof(alloc_huge_object));
    cat::verify(!inline_align_xsalloc_big.is_inline());

    // Test `inline_unalign_salloc`.
    allocator.reset();
    auto [inline_unalign_salloc, inline_unalign_salloc_bytes] =
        allocator.inline_unalign_salloc<int4>(1).value();
    cat::verify(allocator.get(inline_unalign_salloc) == 1);
    cat::verify(inline_unalign_salloc_bytes == cat::inline_buffer_size);
    cat::verify(inline_unalign_salloc.is_inline());

    auto [inline_unalign_salloc_big, inline_unalign_salloc_bytes_big] =
        allocator.inline_unalign_salloc<alloc_huge_object>().value();
    cat::verify(inline_unalign_salloc_bytes_big == ssizeof(alloc_huge_object));
    cat::verify(!inline_unalign_salloc_big.is_inline());

    // Test `inline_unalign_xsalloc`.
    allocator.reset();
    auto [inline_unalign_xsalloc, inline_unalign_xsalloc_bytes] =
        allocator.inline_unalign_xsalloc<int4>(1);
    cat::verify(allocator.get(inline_unalign_xsalloc) == 1);
    cat::verify(inline_unalign_xsalloc_bytes == cat::inline_buffer_size);
    cat::verify(inline_unalign_xsalloc.is_inline());

    auto [inline_unalign_xsalloc_big, inline_unalign_xsalloc_bytes_big] =
        allocator.inline_unalign_xsalloc<alloc_huge_object>();
    cat::verify(inline_unalign_xsalloc_bytes_big == ssizeof(alloc_huge_object));
    cat::verify(!inline_unalign_xsalloc_big.is_inline());

    // Test `inline_align_salloc_multi`.
    allocator.reset();
    auto [inline_align_salloc_multi, inline_align_salloc_multi_bytes] =
        allocator.inline_align_salloc_multi<int4>(8u, 5u).value();
    cat::verify(inline_align_salloc_multi_bytes >= cat::inline_buffer_size);
    cat::verify(inline_align_salloc_multi.is_inline());

    auto [inline_align_salloc_multi_big, inline_align_salloc_multi_bytes_big] =
        allocator.inline_align_salloc_multi<alloc_huge_object>(8u, 5u).value();
    cat::verify(inline_align_salloc_multi_bytes_big >=
                ssizeof(alloc_huge_object));
    cat::verify(!inline_align_salloc_multi_big.is_inline());

    // Test `inline_align_xsalloc_multi`.
    allocator.reset();
    auto [inline_align_xsalloc_multi, inline_align_xsalloc_multi_bytes] =
        allocator.inline_align_xsalloc_multi<int4>(8u, 5u);
    cat::verify(inline_align_xsalloc_multi_bytes >= cat::inline_buffer_size);
    cat::verify(inline_align_xsalloc_multi.is_inline());

    auto [inline_align_xsalloc_multi_big,
          inline_align_xsalloc_multi_bytes_big] =
        allocator.inline_align_xsalloc_multi<alloc_huge_object>(8u, 5u);
    cat::verify(inline_align_xsalloc_multi_bytes_big >=
                ssizeof(alloc_huge_object));
    cat::verify(!inline_align_xsalloc_multi_big.is_inline());

    // Test `inline_unalign_salloc_multi`.
    allocator.reset();
    auto [inline_unalign_salloc_multi, inline_unalign_salloc_multi_bytes] =
        allocator.inline_unalign_salloc_multi<int4>(5u).value();
    cat::verify(inline_unalign_salloc_multi_bytes == cat::inline_buffer_size);
    cat::verify(inline_unalign_salloc_multi.is_inline());

    auto [inline_unalign_salloc_multi_big,
          inline_unalign_salloc_multi_bytes_big] =
        allocator.inline_unalign_salloc_multi<alloc_huge_object>(5u).value();
    cat::verify(inline_unalign_salloc_multi_bytes_big ==
                ssizeof(alloc_huge_object) * 5);
    cat::verify(!inline_unalign_salloc_multi_big.is_inline());

    // Test `inline_unalign_xsalloc_multi`.
    allocator.reset();
    auto [inline_unalign_xsalloc_multi, inline_unalign_xsalloc_multi_bytes] =
        allocator.inline_unalign_xsalloc_multi<int4>(5u);
    cat::verify(inline_unalign_xsalloc_multi_bytes == cat::inline_buffer_size);
    cat::verify(inline_unalign_xsalloc_multi.is_inline());

    auto [inline_unalign_xsalloc_multi_big,
          inline_unalign_xsalloc_multi_bytes_big] =
        allocator.inline_unalign_xsalloc_multi<alloc_huge_object>(5u);
    cat::verify(inline_unalign_xsalloc_multi_bytes_big ==
                ssizeof(alloc_huge_object) * 5);
    cat::verify(!inline_unalign_xsalloc_multi_big.is_inline());

    // Test `xcalloc`.
    _ = allocator.xcalloc<int4>();
    _ = allocator.xcalloc<int4>(1);

    // Test `align_xcalloc`.
    _ = allocator.align_xcalloc<int4>(8u);
    _ = allocator.align_xcalloc<int4>(8u, 1);

    // Test `unalign_xcalloc`.
    _ = allocator.unalign_xcalloc<int1>();
    _ = allocator.unalign_xcalloc<int1>(1);

    // Test `inline_calloc`.
    _ = allocator.inline_calloc<int4>().value();
    _ = allocator.inline_calloc<int4>(1).value();

    // Test `inline_xcalloc`.
    _ = allocator.inline_xcalloc<int4>();
    _ = allocator.inline_xcalloc<int4>(1);

    // Test `inline_align_calloc`.
    _ = allocator.inline_align_calloc<int4>(8u).value();
    _ = allocator.inline_align_calloc<int4>(8u, 1).value();

    // Test `inline_align_xcalloc`.
    _ = allocator.inline_align_xcalloc<int4>(8u);
    _ = allocator.inline_align_xcalloc<int4>(8u, 1);

    // Test `inline_unalign_calloc`.
    _ = allocator.inline_unalign_calloc<int4>().value();
    _ = allocator.inline_unalign_calloc<int4>(1).value();

    // Test `inline_unalign_xcalloc`.
    _ = allocator.inline_unalign_xcalloc<int4>();
    _ = allocator.inline_unalign_xcalloc<int4>(1);

    // Test `xscalloc`.
    _ = allocator.xscalloc<int4>().first();
    _ = allocator.xscalloc<int4>(1).first();

    // Test `align_xscalloc`.
    _ = allocator.align_xscalloc<int4>(8u).first();
    _ = allocator.align_xscalloc<int4>(8u, 1).first();

    // Test `unalign_xscalloc`.
    _ = allocator.unalign_xscalloc<int1>().first();
    _ = allocator.unalign_xscalloc<int1>(1).first();

    // Test `inline_scalloc`.
    _ = allocator.inline_scalloc<int4>().value().first();
    _ = allocator.inline_scalloc<int4>(1).value().first();

    // Test `inline_xscalloc`.
    _ = allocator.inline_xscalloc<int4>().first();
    _ = allocator.inline_xscalloc<int4>(1).first();

    // Test `inline_align_scalloc`.
    _ = allocator.inline_align_scalloc<int4>(8u).value().first();
    _ = allocator.inline_align_scalloc<int4>(8u, 1).value().first();

    // Test `inline_align_xscalloc`.
    _ = allocator.inline_align_xscalloc<int4>(8u).first();
    _ = allocator.inline_align_xscalloc<int4>(8u, 1).first();

    // Test `inline_unalign_scalloc`.
    _ = allocator.inline_unalign_scalloc<int4>().value().first();
    _ = allocator.inline_unalign_scalloc<int4>(1).value().first();

    // Test `inline_unalign_xscalloc`.
    _ = allocator.inline_unalign_xscalloc<int4>().first();
    _ = allocator.inline_unalign_xscalloc<int4>(1).first();

    allocator.reset();

    // Test `realloc`.
    auto* p_realloc_1 = allocator.alloc<int4>(1).value();
    auto* p_realloc_2 = allocator.alloc<int4>(2u).value();
    cat::verify(*p_realloc_1 == 1);
    cat::verify(*p_realloc_2 == 2);
    p_realloc_1 = allocator.realloc(p_realloc_2).value();
    cat::verify(*p_realloc_1 == 2);

    // Test `realloc_to`
    p_alloc = allocator.realloc_to(allocator, p_alloc).value();

    // Test `xrealloc`
    p_alloc = allocator.xrealloc(p_alloc);

    // Test `xrealloc_to`
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

    // Test `realloc_multi_to`
    p_alloc =
        allocator.realloc_multi_to(allocator, p_alloc, 5, 10u).value().data();

    // Test `xrealloc_multi`
    p_alloc = allocator.xrealloc_multi(p_alloc, 5, 10u).data();

    // Test `xrealloc_multi_to`
    p_alloc = allocator.xrealloc_multi_to(allocator, p_alloc, 5, 10u).data();

    // Test `align_realloc_multi`.
    p_alloc = allocator.align_realloc_multi(p_alloc, 8u, 5, 10u).value().data();

    // Test `align_realloc_multi_to`.
    p_alloc = allocator.align_realloc_multi_to(allocator, p_alloc, 8u, 5, 10u)
                  .value()
                  .data();

    // Test `align_xrealloc_multi`.
    p_alloc = allocator.align_xrealloc_multi(p_alloc, 8u, 5, 10u).data();

    // Test `align_xrealloc_multi_to`.
    p_alloc = allocator.align_xrealloc_multi_to(allocator, p_alloc, 8u, 5, 10u)
                  .data();

    // Test `unalign_realloc_multi`.
    p_alloc = allocator.unalign_realloc_multi(p_alloc, 5, 10u).value().data();

    // Test `unalign_realloc_multi_to`.
    p_alloc = allocator.unalign_realloc_multi_to(allocator, p_alloc, 5, 10u)
                  .value()
                  .data();

    // Test `unalign_xrealloc_multi`.
    p_alloc = allocator.unalign_xrealloc_multi(p_alloc, 5, 10u).data();

    // Test `unalign_xrealloc_multi_to`.
    p_alloc =
        allocator.unalign_xrealloc_multi_to(allocator, p_alloc, 5, 10u).data();

    // The allocator runs out of memory around here.
    allocator.reset();
    // This poisons `p_alloc`, so reallocate it.
    p_alloc = allocator.alloc<int4>(1).value();

    // Test `recalloc`.
    p_alloc = allocator.recalloc(p_alloc).value();

    // Test `recalloc_to`
    p_alloc = allocator.recalloc_to(allocator, p_alloc).value();

    // Test `xrecalloc`
    p_alloc = allocator.xrecalloc(p_alloc);

    // Test `xrecalloc_to`
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

    // Test `recalloc_multi_to`
    p_alloc =
        allocator.recalloc_multi_to(allocator, p_alloc, 5, 10u).value().data();

    // Test `xrecalloc_multi`
    p_alloc = allocator.xrecalloc_multi(p_alloc, 5, 10u).data();

    // Test `xrecalloc_multi_to`
    p_alloc = allocator.xrecalloc_multi_to(allocator, p_alloc, 5, 10u).data();

    // Test `align_recalloc_multi`.
    p_alloc =
        allocator.align_recalloc_multi(p_alloc, 8u, 5, 10u).value().data();

    // Test `align_recalloc_multi_to`.
    p_alloc = allocator.align_recalloc_multi_to(allocator, p_alloc, 8u, 5, 10u)
                  .value()
                  .data();

    // Test `align_xrecalloc_multi`.
    p_alloc = allocator.align_xrecalloc_multi(p_alloc, 8u, 5, 10u).data();

    // Test `align_xrecalloc_multi_to`.
    p_alloc = allocator.align_xrecalloc_multi_to(allocator, p_alloc, 8u, 5, 10u)
                  .data();

    // Test `unalign_recalloc_multi`.
    p_alloc = allocator.unalign_recalloc_multi(p_alloc, 5, 10u).value().data();

    // Test `unalign_recalloc_multi_to`.
    p_alloc = allocator.unalign_recalloc_multi_to(allocator, p_alloc, 5, 10u)
                  .value()
                  .data();

    // Test `unalign_xrecalloc_multi`.
    p_alloc = allocator.unalign_xrecalloc_multi(p_alloc, 5, 10u).data();

    // Test `unalign_xrecalloc_multi_to`.
    p_alloc =
        allocator.unalign_xrecalloc_multi_to(allocator, p_alloc, 5, 10u).data();

    // Test `inline_realloc`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_realloc(inline_alloc);
    allocator.free(inline_alloc);

    // Test `inline_realloc_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_realloc_to(allocator, inline_alloc).value();
    allocator.free(inline_alloc);

    // Test `inline_xrealloc`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_xrealloc(inline_alloc);
    allocator.free(inline_alloc);

    // Test `inline_xrealloc_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_xrealloc_to(allocator, inline_alloc);
    allocator.free(inline_alloc);

    // Test `inline_align_realloc`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_align_realloc(inline_alloc, 8u).value();
    allocator.free(inline_alloc);

    // Test `inline_align_realloc_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_align_realloc_to(allocator, inline_alloc, 8u).value();
    allocator.free(inline_alloc);

    // Test `inline_align_xrealloc`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_align_xrealloc(inline_alloc, 8u);
    allocator.free(inline_alloc);

    // Test `inline_align_xrealloc_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_align_xrealloc_to(allocator, inline_alloc, 8u);
    allocator.free(inline_alloc);

    // Test `inline_unalign_realloc`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_unalign_realloc(inline_alloc).value();
    allocator.free(inline_alloc);

    // Test `inline_unalign_realloc_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_unalign_realloc_to(allocator, inline_alloc).value();
    allocator.free(inline_alloc);

    // Test `inline_unalign_xrealloc`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_unalign_xrealloc(inline_alloc);
    allocator.free(inline_alloc);

    // Test `inline_unalign_xrealloc_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_unalign_xrealloc_to(allocator, inline_alloc);
    allocator.free(inline_alloc);

    // Test `inline_align_realloc`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_align_realloc(inline_alloc, 8u).value();
    allocator.free(inline_alloc);

    // Test `inline_align_realloc_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_align_realloc_to(allocator, inline_alloc, 8u).value();
    allocator.free(inline_alloc);

    // Test `inline_align_xrealloc`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_align_xrealloc(inline_alloc, 8u);
    allocator.free(inline_alloc);

    // Test `inline_align_xrealloc_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_align_xrealloc_to(allocator, inline_alloc, 8u);
    allocator.free(inline_alloc);

    // Test `inline_unalign_realloc`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_unalign_realloc(inline_alloc).value();
    allocator.free(inline_alloc);

    // Test `inline_unalign_realloc_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_unalign_realloc_to(allocator, inline_alloc).value();
    allocator.free(inline_alloc);

    // Test `inline_unalign_xrealloc`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_unalign_xrealloc(inline_alloc);
    allocator.free(inline_alloc);

    // Test `inline_unalign_xrealloc_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_unalign_xrealloc_to(allocator, inline_alloc);
    allocator.free(inline_alloc);

    // Test `inline_realloc_multi`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_realloc_multi(inline_alloc, 10u).value();
    allocator.free(inline_alloc);

    // Test `inline_realloc_multi_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_realloc_multi_to(allocator, inline_alloc, 10u).value();
    allocator.free(inline_alloc);

    // Test `inline_xrealloc_multi`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_xrealloc_multi(inline_alloc, 10u);
    allocator.free(inline_alloc);

    // Test `inline_xrealloc_multi_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_xrealloc_multi_to(allocator, inline_alloc, 10u);
    allocator.free(inline_alloc);

    // Test `inline_align_realloc_multi`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_align_realloc_multi(inline_alloc, 8u, 10u).value();
    allocator.free(inline_alloc);

    // Test `inline_align_realloc_multi_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator
            .inline_align_realloc_multi_to(allocator, inline_alloc, 8u, 10u)
            .value();
    allocator.free(inline_alloc);

    // Test `inline_align_xrealloc_multi`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_align_xrealloc_multi(inline_alloc, 8u, 10u);
    allocator.free(inline_alloc);

    // Test `inline_align_xrealloc_multi_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_align_xrealloc_multi_to(allocator, inline_alloc, 8u,
                                                 10u);
    allocator.free(inline_alloc);

    // Test `inline_unalign_realloc_multi`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_unalign_realloc_multi(inline_alloc, 10u).value();
    allocator.free(inline_alloc);

    // Test `inline_unalign_realloc_multi_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_unalign_realloc_multi_to(allocator, inline_alloc, 10u)
            .value();
    allocator.free(inline_alloc);

    // Test `inline_unalign_xrealloc_multi`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_unalign_xrealloc_multi(inline_alloc, 10u);
    allocator.free(inline_alloc);

    // Test `inline_unalign_xrealloc_multi_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_unalign_xrealloc_multi_to(allocator, inline_alloc,
                                                   10u);
    allocator.free(inline_alloc);

    // Test `inline_align_realloc_multi`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_align_realloc_multi(inline_alloc, 8u, 10u).value();
    allocator.free(inline_alloc);

    // Test `inline_align_realloc_multi_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator
            .inline_align_realloc_multi_to(allocator, inline_alloc, 8u, 10u)
            .value();
    allocator.free(inline_alloc);

    // Test `inline_align_xrealloc_multi`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_align_xrealloc_multi(inline_alloc, 8u, 10u);
    allocator.free(inline_alloc);

    // Test `inline_align_xrealloc_multi_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_align_xrealloc_multi_to(allocator, inline_alloc, 8u,
                                                 10u);
    allocator.free(inline_alloc);

    // Test `inline_unalign_realloc_multi`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_unalign_realloc_multi(inline_alloc, 10u).value();
    allocator.free(inline_alloc);

    // Test `inline_unalign_realloc_multi_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_unalign_realloc_multi_to(allocator, inline_alloc, 10u)
            .value();
    allocator.free(inline_alloc);

    // Test `inline_unalign_xrealloc_multi`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_unalign_xrealloc_multi(inline_alloc, 10u);
    allocator.free(inline_alloc);

    // Test `inline_unalign_xrealloc_multi_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_unalign_xrealloc_multi_to(allocator, inline_alloc,
                                                   10u);
    allocator.free(inline_alloc);

    // The allocator runs out of memory around here.
    allocator.reset();

    // Test `inline_recalloc`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_recalloc(inline_alloc).value();
    allocator.free(inline_alloc);

    // Test `inline_recalloc_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_recalloc_to(allocator, inline_alloc).value();
    allocator.free(inline_alloc);

    // Test `inline_xrecalloc`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_xrecalloc(inline_alloc);
    allocator.free(inline_alloc);

    // Test `inline_xrecalloc_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_xrecalloc_to(allocator, inline_alloc);
    allocator.free(inline_alloc);

    // Test `inline_align_recalloc`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_align_recalloc(inline_alloc, 8u).value();
    allocator.free(inline_alloc);

    // Test `inline_align_recalloc_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_align_recalloc_to(allocator, inline_alloc, 8u).value();
    allocator.free(inline_alloc);

    // Test `inline_align_xrecalloc`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_align_xrecalloc(inline_alloc, 8u);
    allocator.free(inline_alloc);

    // Test `inline_align_xrecalloc_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_align_xrecalloc_to(allocator, inline_alloc, 8u);
    allocator.free(inline_alloc);

    // Test `inline_unalign_recalloc`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_unalign_recalloc(inline_alloc).value();
    allocator.free(inline_alloc);

    // Test `inline_unalign_recalloc_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_unalign_recalloc_to(allocator, inline_alloc).value();
    allocator.free(inline_alloc);

    // Test `inline_unalign_xrecalloc`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_unalign_xrecalloc(inline_alloc);
    allocator.free(inline_alloc);

    // Test `inline_unalign_xrecalloc_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_unalign_xrecalloc_to(allocator, inline_alloc);
    allocator.free(inline_alloc);

    // Test `inline_align_recalloc`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_align_recalloc(inline_alloc, 8u).value();
    allocator.free(inline_alloc);

    // Test `inline_align_recalloc_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_align_recalloc_to(allocator, inline_alloc, 8u).value();
    allocator.free(inline_alloc);

    // Test `inline_align_xrecalloc`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_align_xrecalloc(inline_alloc, 8u);
    allocator.free(inline_alloc);

    // Test `inline_align_xrecalloc_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_align_xrecalloc_to(allocator, inline_alloc, 8u);
    allocator.free(inline_alloc);

    // Test `inline_unalign_recalloc`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_unalign_recalloc(inline_alloc).value();
    allocator.free(inline_alloc);

    // Test `inline_unalign_recalloc_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_unalign_recalloc_to(allocator, inline_alloc).value();
    allocator.free(inline_alloc);

    // Test `inline_unalign_xrecalloc`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_unalign_xrecalloc(inline_alloc);
    allocator.free(inline_alloc);

    // Test `inline_unalign_xrecalloc_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_unalign_xrecalloc_to(allocator, inline_alloc);
    allocator.free(inline_alloc);

    // Test `inline_recalloc_multi`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_recalloc_multi(inline_alloc, 10u).value();
    allocator.free(inline_alloc);

    // Test `inline_recalloc_multi_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_recalloc_multi_to(allocator, inline_alloc, 10u)
            .value();
    allocator.free(inline_alloc);

    // Test `inline_xrecalloc_multi`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_xrecalloc_multi(inline_alloc, 10u);
    allocator.free(inline_alloc);

    // Test `inline_xrecalloc_multi_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_xrecalloc_multi_to(allocator, inline_alloc, 10u);
    allocator.free(inline_alloc);

    // Test `inline_align_recalloc_multi`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_align_recalloc_multi(inline_alloc, 8u, 10u).value();
    allocator.free(inline_alloc);

    // Test `inline_align_recalloc_multi_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator
            .inline_align_recalloc_multi_to(allocator, inline_alloc, 8u, 10u)
            .value();
    allocator.free(inline_alloc);

    // Test `inline_align_xrecalloc_multi`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_align_xrecalloc_multi(inline_alloc, 8u, 10u);
    allocator.free(inline_alloc);

    // Test `inline_align_xrecalloc_multi_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_align_xrecalloc_multi_to(allocator, inline_alloc, 8u,
                                                  10u);
    allocator.free(inline_alloc);

    // Test `inline_unalign_recalloc_multi`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_unalign_recalloc_multi(inline_alloc, 10u).value();
    allocator.free(inline_alloc);

    // Test `inline_unalign_recalloc_multi_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_unalign_recalloc_multi_to(allocator, inline_alloc, 10u)
            .value();
    allocator.free(inline_alloc);

    // Test `inline_unalign_xrecalloc_multi`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_unalign_xrecalloc_multi(inline_alloc, 10u);
    allocator.free(inline_alloc);

    // Test `inline_unalign_xrecalloc_multi_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_unalign_xrecalloc_multi_to(allocator, inline_alloc,
                                                    10u);
    allocator.free(inline_alloc);

    // Test `inline_align_recalloc_multi`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_align_recalloc_multi(inline_alloc, 8u, 10u).value();
    allocator.free(inline_alloc);

    // Test `inline_align_recalloc_multi_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator
            .inline_align_recalloc_multi_to(allocator, inline_alloc, 8u, 10u)
            .value();
    allocator.free(inline_alloc);

    // Test `inline_align_xrecalloc_multi`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_align_xrecalloc_multi(inline_alloc, 8u, 10u);
    allocator.free(inline_alloc);

    // Test `inline_align_xrecalloc_multi_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_align_xrecalloc_multi_to(allocator, inline_alloc, 8u,
                                                  10u);
    allocator.free(inline_alloc);

    // Test `inline_unalign_recalloc_multi`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_unalign_recalloc_multi(inline_alloc, 10u).value();
    allocator.free(inline_alloc);

    // Test `inline_unalign_recalloc_multi_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_unalign_recalloc_multi_to(allocator, inline_alloc, 10u)
            .value();
    allocator.free(inline_alloc);

    // Test `inline_unalign_xrecalloc_multi`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_unalign_xrecalloc_multi(inline_alloc, 10u);
    allocator.free(inline_alloc);

    // Test `inline_unalign_xrecalloc_multi_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_unalign_xrecalloc_multi_to(allocator, inline_alloc,
                                                    10u);
    allocator.free(inline_alloc);

    // Test `resalloc`.
    p_alloc = allocator.alloc<int4>(0).value();
    p_alloc = allocator.resalloc(p_alloc).value().first();

    // Test `resalloc_to`
    p_alloc = allocator.resalloc_to(allocator, p_alloc).value().first();

    // Test `xresalloc`
    p_alloc = allocator.xresalloc(p_alloc).first();

    // Test `xresalloc_to`
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

    // Test `resalloc_multi_to`
    p_alloc = allocator.resalloc_multi_to(allocator, p_alloc, 5, 10u)
                  .value()
                  .first()
                  .data();

    // Test `xresalloc_multi`
    p_alloc = allocator.xresalloc_multi(p_alloc, 5, 10u).first().data();

    // Test `xresalloc_multi_to`
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
    p_alloc = allocator.unalign_resalloc_multi(p_alloc, 5, 10u)
                  .value()
                  .first()
                  .data();

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

    // Test `rescalloc_to`
    p_alloc = allocator.rescalloc_to(allocator, p_alloc).value().first();

    // Test `xrescalloc`
    p_alloc = allocator.xrescalloc(p_alloc).first();

    // Test `xrescalloc_to`
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
    p_alloc =
        allocator.unalign_rescalloc_to(allocator, p_alloc).value().first();

    // Test `unalign_xrescalloc`.
    p_alloc = allocator.unalign_xrescalloc(p_alloc).first();

    // Test `unalign_xrescalloc_to`.
    p_alloc = allocator.unalign_xrescalloc_to(allocator, p_alloc).first();

    // Test `rescalloc_multi`.
    p_alloc = allocator.rescalloc_multi(p_alloc, 5, 10u).value().first().data();

    // Test `rescalloc_multi_to`
    p_alloc = allocator.rescalloc_multi_to(allocator, p_alloc, 5, 10u)
                  .value()
                  .first()
                  .data();

    // Test `xrescalloc_multi`
    p_alloc = allocator.xrescalloc_multi(p_alloc, 5, 10u).first().data();

    // Test `xrescalloc_multi_to`
    p_alloc = allocator.xrescalloc_multi_to(allocator, p_alloc, 5, 10u)
                  .first()
                  .data();

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
    p_alloc =
        allocator.align_xrescalloc_multi_to(allocator, p_alloc, 8u, 5, 10u)
            .first()
            .data();

    // Test `unalign_rescalloc_multi`.
    p_alloc = allocator.unalign_rescalloc_multi(p_alloc, 5, 10u)
                  .value()
                  .first()
                  .data();

    // Test `unalign_rescalloc_multi_to`.
    p_alloc = allocator.unalign_rescalloc_multi_to(allocator, p_alloc, 5, 10u)
                  .value()
                  .first()
                  .data();

    // Test `unalign_xrescalloc_multi`.
    p_alloc =
        allocator.unalign_xrescalloc_multi(p_alloc, 5, 10u).first().data();

    // Test `unalign_xrescalloc_multi_to`.
    p_alloc = allocator.unalign_xrescalloc_multi_to(allocator, p_alloc, 5, 10u)
                  .first()
                  .data();

    // Test `inline_resalloc`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_resalloc(inline_alloc);
    allocator.free(inline_alloc);

    // Test `inline_resalloc_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_resalloc_to(allocator, inline_alloc).value();
    allocator.free(inline_alloc);

    // Test `inline_xresalloc`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_xresalloc(inline_alloc);
    allocator.free(inline_alloc);

    // Test `inline_xresalloc_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_xresalloc_to(allocator, inline_alloc);
    allocator.free(inline_alloc);

    // Test `inline_align_resalloc`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_align_resalloc(inline_alloc, 8u).value();
    allocator.free(inline_alloc);

    // Test `inline_align_resalloc_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_align_resalloc_to(allocator, inline_alloc, 8u).value();
    allocator.free(inline_alloc);

    // Test `inline_align_xresalloc`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_align_xresalloc(inline_alloc, 8u);
    allocator.free(inline_alloc);

    // Test `inline_align_xresalloc_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_align_xresalloc_to(allocator, inline_alloc, 8u);
    allocator.free(inline_alloc);

    // Test `inline_unalign_resalloc`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_unalign_resalloc(inline_alloc).value();
    allocator.free(inline_alloc);

    // Test `inline_unalign_resalloc_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_unalign_resalloc_to(allocator, inline_alloc).value();
    allocator.free(inline_alloc);

    // Test `inline_unalign_xresalloc`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_unalign_xresalloc(inline_alloc);
    allocator.free(inline_alloc);

    // Test `inline_unalign_xresalloc_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_unalign_xresalloc_to(allocator, inline_alloc);
    allocator.free(inline_alloc);

    // Test `inline_align_resalloc`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_align_resalloc(inline_alloc, 8u).value();
    allocator.free(inline_alloc);

    // Test `inline_align_resalloc_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_align_resalloc_to(allocator, inline_alloc, 8u).value();
    allocator.free(inline_alloc);

    // Test `inline_align_xresalloc`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_align_xresalloc(inline_alloc, 8u);
    allocator.free(inline_alloc);

    // Test `inline_align_xresalloc_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_align_xresalloc_to(allocator, inline_alloc, 8u);
    allocator.free(inline_alloc);

    // Test `inline_unalign_resalloc`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_unalign_resalloc(inline_alloc).value();
    allocator.free(inline_alloc);

    // Test `inline_unalign_resalloc_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_unalign_resalloc_to(allocator, inline_alloc).value();
    allocator.free(inline_alloc);

    // Test `inline_unalign_xresalloc`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_unalign_xresalloc(inline_alloc);
    allocator.free(inline_alloc);

    // Test `inline_unalign_xresalloc_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_unalign_xresalloc_to(allocator, inline_alloc);
    allocator.free(inline_alloc);

    // Test `inline_resalloc_multi`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_resalloc_multi(inline_alloc, 10u).value();
    allocator.free(inline_alloc);

    // Test `inline_resalloc_multi_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_resalloc_multi_to(allocator, inline_alloc, 10u)
            .value();
    allocator.free(inline_alloc);

    // Test `inline_xresalloc_multi`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_xresalloc_multi(inline_alloc, 10u);
    allocator.free(inline_alloc);

    // Test `inline_xresalloc_multi_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_xresalloc_multi_to(allocator, inline_alloc, 10u);
    allocator.free(inline_alloc);

    // Test `inline_align_resalloc_multi`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_align_resalloc_multi(inline_alloc, 8u, 10u).value();
    allocator.free(inline_alloc);

    // Test `inline_align_resalloc_multi_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator
            .inline_align_resalloc_multi_to(allocator, inline_alloc, 8u, 10u)
            .value();
    allocator.free(inline_alloc);

    // Test `inline_align_xresalloc_multi`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_align_xresalloc_multi(inline_alloc, 8u, 10u);
    allocator.free(inline_alloc);

    // Test `inline_align_xresalloc_multi_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_align_xresalloc_multi_to(allocator, inline_alloc, 8u,
                                                  10u);
    allocator.free(inline_alloc);

    // Test `inline_unalign_resalloc_multi`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_unalign_resalloc_multi(inline_alloc, 10u).value();
    allocator.free(inline_alloc);

    // Test `inline_unalign_resalloc_multi_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_unalign_resalloc_multi_to(allocator, inline_alloc, 10u)
            .value();
    allocator.free(inline_alloc);

    // Test `inline_unalign_xresalloc_multi`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_unalign_xresalloc_multi(inline_alloc, 10u);
    allocator.free(inline_alloc);

    // Test `inline_unalign_xresalloc_multi_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_unalign_xresalloc_multi_to(allocator, inline_alloc,
                                                    10u);
    allocator.free(inline_alloc);

    // Test `inline_align_resalloc_multi`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_align_resalloc_multi(inline_alloc, 8u, 10u).value();
    allocator.free(inline_alloc);

    // Test `inline_align_resalloc_multi_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator
            .inline_align_resalloc_multi_to(allocator, inline_alloc, 8u, 10u)
            .value();
    allocator.free(inline_alloc);

    // Test `inline_align_xresalloc_multi`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_align_xresalloc_multi(inline_alloc, 8u, 10u);
    allocator.free(inline_alloc);

    // Test `inline_align_xresalloc_multi_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_align_xresalloc_multi_to(allocator, inline_alloc, 8u,
                                                  10u);
    allocator.free(inline_alloc);

    // Test `inline_unalign_resalloc_multi`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_unalign_resalloc_multi(inline_alloc, 10u).value();
    allocator.free(inline_alloc);

    // Test `inline_unalign_resalloc_multi_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_unalign_resalloc_multi_to(allocator, inline_alloc, 10u)
            .value();
    allocator.free(inline_alloc);

    // Test `inline_unalign_xresalloc_multi`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_unalign_xresalloc_multi(inline_alloc, 10u);
    allocator.free(inline_alloc);

    // Test `inline_unalign_xresalloc_multi_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_unalign_xresalloc_multi_to(allocator, inline_alloc,
                                                    10u);
    allocator.free(inline_alloc);

    // The allocator runs out of memory around here.
    allocator.reset();

    // Test `inline_rescalloc`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_rescalloc(inline_alloc).value();
    allocator.free(inline_alloc);

    // Test `inline_rescalloc_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_rescalloc_to(allocator, inline_alloc).value();
    allocator.free(inline_alloc);

    // Test `inline_xrescalloc`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_xrescalloc(inline_alloc);
    allocator.free(inline_alloc);

    // Test `inline_xrescalloc_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_xrescalloc_to(allocator, inline_alloc);
    allocator.free(inline_alloc);

    // Test `inline_align_rescalloc`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_align_rescalloc(inline_alloc, 8u).value();
    allocator.free(inline_alloc);

    // Test `inline_align_rescalloc_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_align_rescalloc_to(allocator, inline_alloc, 8u)
            .value();
    allocator.free(inline_alloc);

    // Test `inline_align_xrescalloc`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_align_xrescalloc(inline_alloc, 8u);
    allocator.free(inline_alloc);

    // Test `inline_align_xrescalloc_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_align_xrescalloc_to(allocator, inline_alloc, 8u);
    allocator.free(inline_alloc);

    // Test `inline_unalign_rescalloc`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_unalign_rescalloc(inline_alloc).value();
    allocator.free(inline_alloc);

    // Test `inline_unalign_rescalloc_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_unalign_rescalloc_to(allocator, inline_alloc).value();
    allocator.free(inline_alloc);

    // Test `inline_unalign_xrescalloc`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_unalign_xrescalloc(inline_alloc);
    allocator.free(inline_alloc);

    // Test `inline_unalign_xrescalloc_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_unalign_xrescalloc_to(allocator, inline_alloc);
    allocator.free(inline_alloc);

    // Test `inline_align_rescalloc`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_align_rescalloc(inline_alloc, 8u).value();
    allocator.free(inline_alloc);

    // Test `inline_align_rescalloc_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_align_rescalloc_to(allocator, inline_alloc, 8u)
            .value();
    allocator.free(inline_alloc);

    // Test `inline_align_xrescalloc`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_align_xrescalloc(inline_alloc, 8u);
    allocator.free(inline_alloc);

    // Test `inline_align_xrescalloc_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_align_xrescalloc_to(allocator, inline_alloc, 8u);
    allocator.free(inline_alloc);

    // Test `inline_unalign_rescalloc`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_unalign_rescalloc(inline_alloc).value();
    allocator.free(inline_alloc);

    // Test `inline_unalign_rescalloc_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_unalign_rescalloc_to(allocator, inline_alloc).value();
    allocator.free(inline_alloc);

    // Test `inline_unalign_xrescalloc`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_unalign_xrescalloc(inline_alloc);
    allocator.free(inline_alloc);

    // Test `inline_unalign_xrescalloc_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_unalign_xrescalloc_to(allocator, inline_alloc);
    allocator.free(inline_alloc);

    // Test `inline_rescalloc_multi`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_rescalloc_multi(inline_alloc, 10u).value();
    allocator.free(inline_alloc);

    // Test `inline_rescalloc_multi_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_rescalloc_multi_to(allocator, inline_alloc, 10u)
            .value();
    allocator.free(inline_alloc);

    // Test `inline_xrescalloc_multi`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_xrescalloc_multi(inline_alloc, 10u);
    allocator.free(inline_alloc);

    // Test `inline_xrescalloc_multi_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_xrescalloc_multi_to(allocator, inline_alloc, 10u);
    allocator.free(inline_alloc);

    // Test `inline_align_rescalloc_multi`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_align_rescalloc_multi(inline_alloc, 8u, 10u).value();
    allocator.free(inline_alloc);

    // Test `inline_align_rescalloc_multi_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator
            .inline_align_rescalloc_multi_to(allocator, inline_alloc, 8u, 10u)
            .value();
    allocator.free(inline_alloc);

    // Test `inline_align_xrescalloc_multi`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_align_xrescalloc_multi(inline_alloc, 8u, 10u);
    allocator.free(inline_alloc);

    // Test `inline_align_xrescalloc_multi_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_align_xrescalloc_multi_to(allocator, inline_alloc, 8u,
                                                   10u);
    allocator.free(inline_alloc);

    // Test `inline_unalign_rescalloc_multi`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_unalign_rescalloc_multi(inline_alloc, 10u).value();
    allocator.free(inline_alloc);

    // Test `inline_unalign_rescalloc_multi_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator
            .inline_unalign_rescalloc_multi_to(allocator, inline_alloc, 10u)
            .value();
    allocator.free(inline_alloc);

    // Test `inline_nalign_xrescalloc_multi`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_unalign_xrescalloc_multi(inline_alloc, 10u);
    allocator.free(inline_alloc);

    // Test `inline_unalign_xrescalloc_multi_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_unalign_xrescalloc_multi_to(allocator, inline_alloc,
                                                     10u);
    allocator.free(inline_alloc);

    // Test `inline_align_rescalloc_multi`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_align_rescalloc_multi(inline_alloc, 8u, 10u).value();
    allocator.free(inline_alloc);

    // Test `inline_align_rescalloc_multi_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator
            .inline_align_rescalloc_multi_to(allocator, inline_alloc, 8u, 10u)
            .value();
    allocator.free(inline_alloc);

    // Test `inline_align_xrescalloc_multi`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_align_xrescalloc_multi(inline_alloc, 8u, 10u);
    allocator.free(inline_alloc);

    // Test `inline_align_xrescalloc_multi_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_align_xrescalloc_multi_to(allocator, inline_alloc, 8u,
                                                   10u);
    allocator.free(inline_alloc);

    // Test `inline_unalign_rescalloc_multi`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_unalign_rescalloc_multi(inline_alloc, 10u).value();
    allocator.free(inline_alloc);

    // Test `inline_unalign_rescalloc_multi_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator
            .inline_unalign_rescalloc_multi_to(allocator, inline_alloc, 10u)
            .value();
    allocator.free(inline_alloc);

    // Test `inline_nalign_xrescalloc_multi`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_unalign_xrescalloc_multi(inline_alloc, 10u);
    allocator.free(inline_alloc);

    // Test `inline_unalign_xrescalloc_multi_to`.
    inline_alloc = allocator.inline_alloc<int4>().value();
    _ = allocator.inline_unalign_xrescalloc_multi_to(allocator, inline_alloc,
                                                     10u);
}
