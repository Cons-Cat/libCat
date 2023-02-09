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

    cat::span<int4> alloc_multi = allocator.alloc_multi<int4>(5).value();

    alloc_multi =
        allocator.realloc_multi(alloc_multi.data(), alloc_multi.size(), 10)
            .value();

    allocator.free_multi(alloc_multi.data(), alloc_multi.size());

    cat::span<int4> xalloc_multi = allocator.xalloc_multi<int4>(5);
    allocator.free_multi(xalloc_multi.data(), xalloc_multi.size());
}

TEST(test_alloc) {
    const_test();

    // Initialize an allocator.
    cat::page_allocator pager;
    pager.reset();
    // Page the kernel for a linear allocator to test with.
    auto page = pager.opq_alloc_multi<cat::byte>(4_ki - 64).or_exit();
    defer(pager.free(page);)
    auto allocator = cat::linear_allocator::backed_handle(pager, page);

    // Test `opq_alloc`.
    _ = allocator.opq_alloc<int4>().value();
    auto opq_alloc = allocator.opq_alloc<int4>(1).value();
    cat::verify(allocator.get(opq_alloc) == 1);
    alloc_counter = 0;
    _ = allocator.opq_alloc<alloc_non_trivial>();
    cat::verify(alloc_counter == 1);

    // Test `opq_xalloc`.
    _ = allocator.opq_xalloc<int4>();
    auto opq_xalloc = allocator.opq_xalloc<int4>(1);
    cat::verify(allocator.get(opq_xalloc) == 1);

    // Test `alloc`.
    _ = allocator.alloc<int4>().value();
    auto p_alloc = allocator.alloc<int4>(1).value();
    cat::verify(*p_alloc == 1);

    // Test `xalloc`.
    _ = allocator.xalloc<int4>();
    auto p_xalloc = allocator.xalloc<int4>(1);
    cat::verify(*p_xalloc == 1);

    // Test `alloc_multi`.
    auto alloc_multi = allocator.opq_alloc_multi<int4>(5).value();
    cat::verify(alloc_multi.size() == 5);
    cat::verify(alloc_multi.raw_size() == 20);
    alloc_counter = 0;
    _ = allocator.opq_alloc_multi<alloc_non_trivial>(5);
    cat::verify(alloc_counter == 5);

    // Test `xalloc_multi`.
    auto xalloc_multi = allocator.opq_xalloc_multi<int4>(5);
    cat::verify(xalloc_multi.size() == 5);
    cat::verify(xalloc_multi.raw_size() == 20);

    // Test `alloc_multi`.
    _ = allocator.alloc_multi<int4>(5).value();

    // Test `xalloc_multi`.
    _ = allocator.xalloc_multi<int4>(5);

    // Test `opq_align_alloc`.
    _ = allocator.opq_align_alloc<int4>(8u).value();
    auto align_alloc = allocator.opq_align_alloc<int4>(8u, 1).value();
    cat::verify(allocator.get(align_alloc) == 1);
    cat::verify(cat::is_aligned(&allocator.get(align_alloc), 8u));

    // Test `opq_align_xalloc`.
    _ = allocator.opq_align_xalloc<int4>(8u);
    auto align_xalloc = allocator.opq_align_xalloc<int4>(8u, 1);
    cat::verify(allocator.get(align_xalloc) == 1);
    cat::verify(cat::is_aligned(&allocator.get(align_xalloc), 8u));

    // Test `align_alloc`.
    _ = allocator.align_alloc<int4>(8u).value();
    auto p_align_alloc = allocator.align_alloc<int4>(8u, 1).value();
    cat::verify(*p_align_alloc == 1);
    cat::verify(cat::is_aligned(p_align_alloc, 8u));

    // Test `align_xalloc`.
    _ = allocator.align_xalloc<int4>(8u);
    auto p_align_xalloc = allocator.align_xalloc<int4>(8u, 1);
    cat::verify(*p_align_xalloc == 1);
    cat::verify(cat::is_aligned(p_align_xalloc, 8u));

    // Test `opq_unalign_alloc`.
    _ = allocator.opq_unalign_alloc<int4>().value();
    auto unalign_alloc = allocator.opq_unalign_alloc<int4>(1).value();
    cat::verify(allocator.get(unalign_alloc) == 1);

    // Test `opq_unalign_xalloc`.
    _ = allocator.opq_unalign_xalloc<int4>(8u);
    auto unalign_xalloc = allocator.opq_unalign_xalloc<int4>(1);
    cat::verify(allocator.get(unalign_xalloc) == 1);

    // Test `unalign_alloc`.
    _ = allocator.unalign_alloc<int4>(8u).value();
    auto p_unalign_alloc = allocator.unalign_alloc<int4>(1).value();
    cat::verify(*p_unalign_alloc == 1);

    // Test `unalign_xalloc`.
    _ = allocator.unalign_xalloc<int4>(8u);
    auto p_unalign_xalloc = allocator.unalign_xalloc<int4>(1);
    cat::verify(*p_unalign_xalloc == 1);

    // Test `align_alloc_multi`.
    auto align_alloc_multi =
        allocator.opq_align_alloc_multi<int4>(8u, 5).value();
    cat::verify(align_alloc_multi.size() == 5);
    cat::verify(align_alloc_multi.raw_size() == 20);
    cat::verify(cat::is_aligned(allocator.p_get(align_alloc_multi), 8u));
    alloc_counter = 0;
    _ = allocator.opq_align_alloc_multi<alloc_non_trivial>(8u, 5);
    cat::verify(alloc_counter == 5);

    // Test `align_xalloc_multi`.
    auto align_xalloc_multi = allocator.opq_align_xalloc_multi<int4>(8u, 5);
    cat::verify(align_xalloc_multi.size() == 5);
    cat::verify(align_xalloc_multi.raw_size() == 20);
    cat::verify(cat::is_aligned(allocator.p_get(align_xalloc_multi), 8u));
    alloc_counter = 0;
    _ = allocator.opq_align_xalloc_multi<alloc_non_trivial>(8u, 5);

    cat::verify(alloc_counter == 5);

    // Test `align_alloc_multi`.
    auto p_align_alloc_multi =
        allocator.align_alloc_multi<int4>(8u, 5).value().data();
    cat::verify(cat::is_aligned(p_align_alloc_multi, 8u));
    alloc_counter = 0;
    _ = allocator.align_alloc_multi<alloc_non_trivial>(8u, 5);
    cat::verify(alloc_counter == 5);

    // Test `align_xalloc_multi`.
    _ = allocator.align_xalloc_multi<int4>(8u, 5);
    alloc_counter = 0;
    _ = allocator.align_xalloc_multi<alloc_non_trivial>(8u, 5);
    cat::verify(alloc_counter == 5);

    // Test `unalign_alloc_multi`.
    auto unalign_alloc_multi =
        allocator.opq_unalign_alloc_multi<int4>(5).value();
    cat::verify(unalign_alloc_multi.size() == 5);
    cat::verify(unalign_alloc_multi.raw_size() == 20);
    alloc_counter = 0;
    _ = allocator.opq_unalign_alloc_multi<alloc_non_trivial>(5);
    cat::verify(alloc_counter == 5);

    // Test `unalign_xalloc_multi`.
    auto unalign_xalloc_multi = allocator.opq_unalign_xalloc_multi<int1>(5);
    cat::verify(unalign_xalloc_multi.size() == 5);
    cat::verify(unalign_xalloc_multi.raw_size() == 5);
    alloc_counter = 0;
    _ = allocator.opq_unalign_xalloc_multi<alloc_non_trivial>(5);
    cat::verify(alloc_counter == 5);

    // Test `unalign_alloc_multi`.
    _ = allocator.unalign_alloc_multi<int1>(5)
            .value();  // `int4` is 4-byte aligned.
    alloc_counter = 0;
    _ = allocator.unalign_alloc_multi<alloc_non_trivial>(5);
    cat::verify(alloc_counter == 5);

    // Test `unalign_xalloc_multi`.
    _ = allocator.unalign_xalloc_multi<int1>(5);  // `int4` is 4-byte aligned.
    alloc_counter = 0;
    _ = allocator.unalign_xalloc_multi<alloc_non_trivial>(5);
    cat::verify(alloc_counter == 5);

    // Test `opq_inline_alloc`.
    _ = allocator.opq_inline_alloc<int4>().value();
    auto inline_alloc = allocator.opq_inline_alloc<int4>(1).value();
    cat::verify(allocator.get(inline_alloc) == 1);
    cat::verify(inline_alloc.is_inline());
    alloc_counter = 0;
    _ = allocator.opq_inline_alloc<alloc_non_trivial>();
    cat::verify(alloc_counter == 1);

    // `alloc_huge_object` is larger than the inline buffer.
    auto inline_alloc_2 =
        allocator.opq_inline_alloc<alloc_huge_object>().value();
    cat::verify(!inline_alloc_2.is_inline());

    alloc_counter = 0;
    _ = allocator.opq_inline_alloc<alloc_non_trivial_huge_object>();
    cat::verify(alloc_counter == 1);

    // Test `opq_inline_xalloc`.
    _ = allocator.opq_inline_xalloc<int4>();
    auto inline_xalloc = allocator.opq_inline_xalloc<int4>(1);
    cat::verify(allocator.get(inline_xalloc) == 1);

    // Test `inline_alloc_multi`.
    auto inline_alloc_multi = allocator.opq_inline_alloc_multi<int4>(5).value();
    cat::verify(inline_alloc_multi.size() == 5);
    alloc_counter = 0;
    _ = allocator.opq_inline_alloc_multi<alloc_non_trivial>(5);
    cat::verify(alloc_counter == 5);

    // Test `inline_xalloc_multi`.
    auto inline_xalloc_multi = allocator.opq_inline_xalloc_multi<int4>(5);
    cat::verify(inline_xalloc_multi.size() == 5);
    alloc_counter = 0;
    _ = allocator.opq_inline_xalloc_multi<alloc_non_trivial>(5);
    cat::verify(alloc_counter == 5);

    // Test `opq_inline_align_alloc`.
    _ = allocator.opq_inline_align_alloc<int4>(8u).value();
    auto inline_align_alloc =
        allocator.opq_inline_align_alloc<int4>(8u, 1).value();
    cat::verify(allocator.get(inline_align_alloc) == 1);
    cat::verify(cat::is_aligned(&allocator.get(inline_align_alloc), 8u));
    cat::verify(inline_align_alloc.is_inline());

    // Test `opq_inline_unalign_alloc`.
    _ = allocator.opq_inline_unalign_alloc<int4>(8u).value();
    auto inline_unalign_alloc =
        allocator.opq_inline_unalign_alloc<int4>(1).value();
    cat::verify(allocator.get(inline_unalign_alloc) == 1);
    cat::verify(inline_unalign_alloc.is_inline());

    // Test `opq_inline_unalign_xalloc`.
    _ = allocator.opq_inline_unalign_xalloc<int4>(8u);
    auto inline_unalign_xalloc = allocator.opq_inline_unalign_xalloc<int4>(1);
    cat::verify(allocator.get(inline_unalign_xalloc) == 1);
    cat::verify(inline_unalign_xalloc.is_inline());

    allocator.reset();

    // Test `inline_align_alloc_multi`.
    auto inline_align_alloc_multi =
        allocator.opq_inline_align_alloc_multi<int4>(8u, 5).value();
    cat::verify(cat::is_aligned(allocator.p_get(inline_align_alloc_multi), 8u));
    cat::verify(inline_align_alloc_multi.is_inline());

    auto inline_align_alloc_multi_big =
        allocator.opq_inline_align_alloc_multi<int4>(8u, 64).value();
    cat::verify(!inline_align_alloc_multi_big.is_inline());

    // Test `inline_align_xalloc_multi`.
    auto inline_align_xalloc_multi =
        allocator.opq_inline_align_xalloc_multi<int4>(8u, 5);
    cat::verify(
        cat::is_aligned(allocator.p_get(inline_align_xalloc_multi), 8u));
    cat::verify(inline_align_xalloc_multi.is_inline());

    // Test `inline_unalign_alloc_multi`.
    auto inline_unalign_alloc_multi =
        allocator.opq_inline_unalign_alloc_multi<int4>(5).value();
    cat::verify(inline_unalign_alloc_multi.is_inline());

    auto inline_unalign_alloc_multi_big =
        allocator.opq_inline_unalign_alloc_multi<int4>(64).value();
    cat::verify(!inline_unalign_alloc_multi_big.is_inline());

    // Test `inline_unalign_xalloc_multi`.
    auto inline_unalign_xalloc_multi =
        allocator.opq_inline_unalign_xalloc_multi<int4>(5);
    cat::verify(inline_unalign_xalloc_multi.is_inline());

    auto inline_unalign_xalloc_multi_big =
        allocator.opq_inline_unalign_xalloc_multi<int4>(64);
    cat::verify(!inline_unalign_xalloc_multi_big.is_inline());

    // Always reset the allocator so that there are no alignment requirements
    // interfering with `opq_nalloc` tests. Specific allocator tests such as
    // `test_linear_allocator.opq_cpp` check that in greater cat::detail.

    // Test `opq_nalloc`.
    allocator.reset();
    iword nalloc = allocator.opq_nalloc<int4>().value();
    cat::verify(nalloc == ssizeof(int4));

    // Test `opq_xnalloc`.
    allocator.reset();
    iword xnalloc = allocator.opq_xnalloc<int4>();
    cat::verify(xnalloc == ssizeof(int4));

    // Test `nalloc_multi`.
    allocator.reset();
    iword nalloc_multi = allocator.opq_nalloc_multi<int4>(5).value();
    cat::verify(nalloc_multi == (ssizeof(int4) * 5));

    // Test `xnalloc_multi`.
    allocator.reset();
    iword xnalloc_multi = allocator.opq_xnalloc_multi<int4>(5);
    cat::verify(xnalloc_multi == (ssizeof(int4) * 5));

    // Test `opq_align_nalloc`.
    allocator.reset();
    iword align_nalloc = allocator.opq_align_nalloc<int4>(4u).value();
    cat::verify(align_nalloc == ssizeof(int4));

    // Test `opq_align_xnalloc`.
    allocator.reset();
    iword align_xnalloc = allocator.opq_align_xnalloc<int4>(4u);
    cat::verify(align_xnalloc == ssizeof(int4));

    // Test `align_nalloc_multi`.
    allocator.reset();
    iword align_nalloc_multi =
        allocator.opq_align_nalloc_multi<int4>(4u, 5).value();
    cat::verify(align_nalloc_multi == (ssizeof(int4) * 5));

    // Test `align_xnalloc_multi`.
    allocator.reset();
    iword align_xnalloc_multi = allocator.opq_align_xnalloc_multi<int4>(4u, 5);
    cat::verify(align_xnalloc_multi == (ssizeof(int4) * 5));

    // Test `opq_unalign_nalloc`.
    allocator.reset();
    iword unalign_nalloc = allocator.opq_unalign_nalloc<int4>().value();
    cat::verify(unalign_nalloc == ssizeof(int4));

    // Test `opq_unalign_xnalloc`.
    allocator.reset();
    iword unalign_xnalloc = allocator.opq_unalign_xnalloc<int4>();
    cat::verify(unalign_xnalloc == ssizeof(int4));

    // Test `unalign_nalloc_multi`.
    allocator.reset();
    iword unalign_nalloc_multi =
        allocator.opq_unalign_nalloc_multi<int4>(5).value();
    cat::verify(unalign_nalloc_multi == (ssizeof(int4) * 5));

    // Test `unalign_xnalloc_multi`.
    allocator.reset();
    iword unalign_xnalloc_multi = allocator.opq_unalign_xnalloc_multi<int4>(5);
    cat::verify(unalign_xnalloc_multi == (ssizeof(int4) * 5));

    // Test `opq_inline_nalloc`.
    allocator.reset();
    iword inline_nalloc = allocator.opq_inline_nalloc<int4>().value();
    cat::verify(inline_nalloc == cat::inline_buffer_size);
    iword inline_nalloc_big =
        allocator.opq_inline_nalloc<alloc_huge_object>().value();
    cat::verify(inline_nalloc_big == 257);

    // Test `opq_inline_xnalloc`.
    allocator.reset();
    iword inline_xnalloc = allocator.opq_inline_xnalloc<int4>();
    cat::verify(inline_xnalloc == cat::inline_buffer_size);
    iword inline_xnalloc_big =
        allocator.opq_inline_xnalloc<alloc_huge_object>();
    cat::verify(inline_xnalloc_big == 257);

    // Test `inline_nalloc_multi`.
    allocator.reset();
    iword inline_nalloc_multi =
        allocator.opq_inline_nalloc_multi<int4>(5).value();
    cat::verify(inline_nalloc_multi == cat::inline_buffer_size);
    iword inline_nalloc_multi_big =
        allocator.opq_inline_nalloc_multi<alloc_huge_object>(2).value();
    cat::verify(inline_nalloc_multi_big == (257 * 2));

    // Test `inline_xnalloc_multi`.
    allocator.reset();
    iword inline_xnalloc_multi = allocator.opq_inline_xnalloc_multi<int4>(5);
    cat::verify(inline_xnalloc_multi == cat::inline_buffer_size);
    iword inline_xnalloc_multi_big =
        allocator.opq_inline_xnalloc_multi<alloc_huge_object>(2);
    cat::verify(inline_xnalloc_multi_big == (257 * 2));

    // Test `opq_inline_align_nalloc`.
    allocator.reset();
    iword inline_align_nalloc =
        allocator.opq_inline_align_nalloc<int4>(4u).value();
    cat::verify(inline_align_nalloc == cat::inline_buffer_size);
    iword inline_align_nalloc_big =
        allocator.opq_inline_align_nalloc<alloc_huge_object>(1u).value();
    cat::verify(inline_align_nalloc_big == 257);

    // Test `opq_inline_align_xnalloc`.
    allocator.reset();
    iword inline_align_xnalloc = allocator.opq_inline_align_xnalloc<int4>(4u);
    cat::verify(inline_align_xnalloc == cat::inline_buffer_size);
    iword inline_align_xnalloc_big =
        allocator.opq_inline_align_xnalloc<alloc_huge_object>(1u);
    cat::verify(inline_align_xnalloc_big == 257);

    // Test `opq_inline_unalign_nalloc`.
    allocator.reset();
    iword inline_unalign_nalloc =
        allocator.opq_inline_unalign_nalloc<int4>().value();
    cat::verify(inline_unalign_nalloc == cat::inline_buffer_size);
    iword inline_unalign_nalloc_big =
        allocator.opq_inline_unalign_nalloc<alloc_huge_object>().value();
    cat::verify(inline_unalign_nalloc_big == 257);

    // Test `opq_inline_unalign_xnalloc`.
    allocator.reset();
    iword inline_unalign_xnalloc = allocator.opq_inline_unalign_xnalloc<int4>();
    cat::verify(inline_unalign_xnalloc == cat::inline_buffer_size);
    iword inline_unalign_xnalloc_big =
        allocator.opq_inline_unalign_xnalloc<alloc_huge_object>();
    cat::verify(inline_unalign_xnalloc_big == 257);

    // Test `inline_align_nalloc_multi`.
    allocator.reset();
    iword inline_align_nalloc_multi =
        allocator.opq_inline_align_nalloc_multi<int4>(4u, 5).value();
    cat::verify(inline_align_nalloc_multi == cat::inline_buffer_size);
    iword inline_align_nalloc_multi_big =
        allocator.opq_inline_align_nalloc_multi<alloc_huge_object>(1u, 2)
            .value();
    cat::verify(inline_align_nalloc_multi_big == (257 * 2));

    // Test `inline_align_xnalloc_multi`.
    allocator.reset();
    iword inline_align_xnalloc_multi =
        allocator.opq_inline_align_xnalloc_multi<int4>(4u, 5);
    cat::verify(inline_align_xnalloc_multi == cat::inline_buffer_size);
    iword inline_align_xnalloc_multi_big =
        allocator.opq_inline_align_xnalloc_multi<alloc_huge_object>(1u, 2);
    cat::verify(inline_align_xnalloc_multi_big == (257 * 2));

    // Test `inline_unalign_nalloc_multi`.
    allocator.reset();
    iword inline_unalign_nalloc_multi =
        allocator.opq_inline_unalign_nalloc_multi<int4>(5).value();
    cat::verify(inline_unalign_nalloc_multi == cat::inline_buffer_size);
    iword inline_unalign_nalloc_multi_big =
        allocator.opq_inline_unalign_nalloc_multi<alloc_huge_object>(2).value();
    cat::verify(inline_unalign_nalloc_multi_big == (257 * 2));

    // Test `inline_unalign_xnalloc_multi`.
    allocator.reset();
    iword inline_unalign_xnalloc_multi =
        allocator.opq_inline_unalign_xnalloc_multi<int4>(5);
    cat::verify(inline_unalign_xnalloc_multi == cat::inline_buffer_size);
    iword inline_unalign_xnalloc_multi_big =
        allocator.opq_inline_unalign_xnalloc_multi<alloc_huge_object>(2);
    cat::verify(inline_unalign_xnalloc_multi_big == (257 * 2));

    // Test `opq_salloc`.
    _ = allocator.opq_salloc<int4>().value();
    allocator.reset();
    _ = allocator.opq_alloc<cat::byte>();  // Offset linear allocator by 1 byte.
    auto [salloc, salloc_bytes] = allocator.opq_salloc<int4>(1).value();
    cat::verify(allocator.get(salloc) == 1);
    cat::verify(salloc_bytes == 7);
    alloc_counter = 0;
    _ = allocator.opq_salloc<alloc_non_trivial>();
    cat::verify(alloc_counter == 1);

    // Test `opq_xsalloc`.
    _ = allocator.opq_xsalloc<int4>();
    allocator.reset();
    _ = allocator.opq_alloc<cat::byte>();  // Offset linear allocator by 1 byte.
    auto [xsalloc, xsalloc_bytes] = allocator.opq_xsalloc<int4>(1);
    cat::verify(allocator.get(xsalloc) == 1);
    cat::verify(xsalloc_bytes == 7);
    alloc_counter = 0;
    _ = allocator.opq_xsalloc<alloc_non_trivial>();
    cat::verify(alloc_counter == 1);

    // Test `salloc`.
    _ = allocator.salloc<int4>().value();
    allocator.reset();
    _ = allocator.opq_alloc<cat::byte>();  // Offset linear allocator by 1 byte.
    auto [p_salloc, p_salloc_bytes] = allocator.salloc<int4>(1).value();
    cat::verify(*p_salloc == 1);
    cat::verify(p_salloc_bytes == 7);
    alloc_counter = 0;
    _ = allocator.salloc<alloc_non_trivial>();
    cat::verify(alloc_counter == 1);

    // Test `xsalloc`.
    _ = allocator.xsalloc<int4>();
    allocator.reset();
    _ = allocator.opq_alloc<cat::byte>();  // Offset linear allocator by 1 byte.
    auto [p_xsalloc, p_xsalloc_bytes] = allocator.xsalloc<int4>(1);
    cat::verify(*p_xsalloc == 1);
    cat::verify(p_xsalloc_bytes == 7);
    alloc_counter = 0;
    _ = allocator.xsalloc<alloc_non_trivial>();
    cat::verify(alloc_counter == 1);

    // Test `salloc_multi`.
    allocator.reset();
    _ = allocator.opq_alloc<cat::byte>();  // Offset linear allocator by 1 byte.
    auto [salloc_multi, salloc_multi_bytes] =
        allocator.opq_salloc_multi<int4>(5).value();
    cat::verify(salloc_multi.size() == 5);
    cat::verify(salloc_multi_bytes == 23);
    cat::verify(salloc_multi.raw_size() == 20);

    alloc_counter = 0;
    _ = allocator.opq_salloc_multi<alloc_non_trivial>(5);
    cat::verify(alloc_counter == 5);

    // Test `xsalloc_multi`.
    allocator.reset();
    _ = allocator.opq_alloc<cat::byte>();  // Offset linear allocator by 1 byte.
    auto [xsalloc_multi, xsalloc_multi_bytes] =
        allocator.opq_xsalloc_multi<int4>(5);
    cat::verify(xsalloc_multi.size() == 5);
    cat::verify(xsalloc_multi_bytes == 23);
    cat::verify(xsalloc_multi.raw_size() == 20);

    alloc_counter = 0;
    _ = allocator.opq_xsalloc_multi<alloc_non_trivial>(5);
    cat::verify(alloc_counter == 5);

    // Test `salloc_multi`.
    allocator.reset();
    _ = allocator.opq_alloc<cat::byte>();  // Offset linear allocator by 1 byte.
    auto [p_salloc_multi, p_salloc_multi_bytes] =
        allocator.salloc_multi<int4>(5).value();
    cat::verify(p_salloc_multi_bytes == 23);

    alloc_counter = 0;
    _ = allocator.salloc_multi<alloc_non_trivial>(5);
    cat::verify(alloc_counter == 5);

    // Test `xsalloc_multi`.
    allocator.reset();
    _ = allocator.opq_alloc<cat::byte>();  // Offset linear allocator by 1 byte.
    auto [p_xsalloc_multi, p_xsalloc_multi_bytes] =
        allocator.xsalloc_multi<int4>(5);
    cat::verify(p_xsalloc_multi_bytes == 23);

    alloc_counter = 0;
    _ = allocator.xsalloc_multi<alloc_non_trivial>(5);
    cat::verify(alloc_counter == 5);

    // Test `opq_align_salloc`.
    _ = allocator.opq_align_salloc<int4>(8u);
    allocator.reset();
    auto [align_salloc, align_salloc_bytes] =
        allocator.opq_align_salloc<int4>(8u, 1).value();
    cat::verify(allocator.get(align_salloc) == 1);
    cat::verify(align_salloc_bytes == 8);
    cat::verify(cat::is_aligned(&allocator.get(align_salloc), 8u));

    alloc_counter = 0;
    _ = allocator.opq_align_salloc<alloc_non_trivial>(8u);
    cat::verify(alloc_counter == 1);

    // Test `opq_align_xsalloc`.
    _ = allocator.opq_align_xsalloc<int4>(8u);
    allocator.reset();
    auto [align_xsalloc, align_xsalloc_bytes] =
        allocator.opq_align_xsalloc<int4>(8u, 1);
    cat::verify(allocator.get(align_xsalloc) == 1);
    cat::verify(align_xsalloc_bytes == 8);
    cat::verify(cat::is_aligned(&allocator.get(align_xsalloc), 8u));

    alloc_counter = 0;
    _ = allocator.opq_align_xsalloc<alloc_non_trivial>(8u);
    cat::verify(alloc_counter == 1);

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

    // Test `opq_unalign_salloc`.
    _ = allocator.opq_unalign_salloc<int1>();
    allocator.reset();
    auto [unalign_salloc, unalign_salloc_bytes] =
        allocator.opq_unalign_salloc<int1>(1).value();
    cat::verify(allocator.get(unalign_salloc) == 1);
    cat::verify(unalign_salloc_bytes == 1);

    alloc_counter = 0;
    _ = allocator.opq_unalign_salloc<alloc_non_trivial>();
    cat::verify(alloc_counter == 1);

    // Test `opq_unalign_xsalloc`.
    _ = allocator.opq_unalign_xsalloc<int1>();
    allocator.reset();
    auto [unalign_xsalloc, unalign_xsalloc_bytes] =
        allocator.opq_unalign_xsalloc<int1>(1);
    cat::verify(allocator.get(unalign_xsalloc) == 1);
    cat::verify(unalign_xsalloc_bytes == 1);

    alloc_counter = 0;
    _ = allocator.opq_unalign_xsalloc<alloc_non_trivial>();
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
    auto [align_salloc_multi, align_salloc_multi_bytes] =
        allocator.opq_align_salloc_multi<int4>(8u, 5).value();
    cat::verify(align_salloc_multi_bytes == 24);
    cat::verify(cat::is_aligned(allocator.p_get(align_salloc_multi), 8u));

    alloc_counter = 0;
    _ = allocator.opq_align_salloc_multi<alloc_non_trivial>(8u, 5);
    cat::verify(alloc_counter == 5);

    // Test `align_xsalloc_multi`.
    allocator.reset();
    auto [align_xsalloc_multi, align_xsalloc_multi_bytes] =
        allocator.opq_align_xsalloc_multi<int4>(8u, 5);
    cat::verify(align_xsalloc_multi_bytes == 24);
    cat::verify(cat::is_aligned(allocator.p_get(align_xsalloc_multi), 8u));

    alloc_counter = 0;
    _ = allocator.opq_align_xsalloc_multi<alloc_non_trivial>(8u, 5);
    cat::verify(alloc_counter == 5);

    // Test `align_salloc_multi`.
    allocator.reset();
    auto [p_align_salloc_multi, p_align_salloc_multi_bytes] =
        allocator.align_salloc_multi<int4>(8u, 5).value();
    cat::verify(p_align_salloc_multi_bytes == 24);
    cat::verify(cat::is_aligned(p_align_salloc_multi.data(), 8u));

    alloc_counter = 0;
    _ = allocator.align_salloc_multi<alloc_non_trivial>(8u, 5);
    cat::verify(alloc_counter == 5);

    // Test `align_xsalloc_multi`.
    allocator.reset();
    auto [p_align_xsalloc_multi, p_align_xsalloc_multi_bytes] =
        allocator.align_xsalloc_multi<int4>(8u, 5);
    cat::verify(p_align_xsalloc_multi_bytes == 24);
    cat::verify(cat::is_aligned(p_align_xsalloc_multi.data(), 8u));

    alloc_counter = 0;
    _ = allocator.align_xsalloc_multi<alloc_non_trivial>(8u, 5);
    cat::verify(alloc_counter == 5);

    // Test `unalign_salloc_multi`.
    allocator.reset();
    auto [unalign_salloc_multi, unalign_salloc_multi_bytes] =
        allocator.opq_unalign_salloc_multi<int1>(5).value();
    cat::verify(unalign_salloc_multi_bytes == 5);

    alloc_counter = 0;
    _ = allocator.opq_unalign_salloc_multi<alloc_non_trivial>(5);
    cat::verify(alloc_counter == 5);

    // Test `unalign_xsalloc_multi`.
    allocator.reset();
    auto [unalign_xsalloc_multi, unalign_xsalloc_multi_bytes] =
        allocator.opq_unalign_xsalloc_multi<int1>(5);
    cat::verify(unalign_xsalloc_multi_bytes == 5);

    alloc_counter = 0;
    _ = allocator.opq_unalign_xsalloc_multi<alloc_non_trivial>(5);
    cat::verify(alloc_counter == 5);

    // Test `unalign_salloc_multi`.
    allocator.reset();
    auto [p_unalign_salloc_multi, p_unalign_salloc_multi_bytes] =
        allocator.unalign_salloc_multi<int1>(5).value();
    cat::verify(p_unalign_salloc_multi_bytes == 5);

    alloc_counter = 0;
    _ = allocator.unalign_salloc_multi<alloc_non_trivial>(5);
    cat::verify(alloc_counter == 5);

    // Test `unalign_xsalloc_multi`.
    allocator.reset();
    auto [p_unalign_xsalloc_multi, p_unalign_xsalloc_multi_bytes] =
        allocator.unalign_xsalloc_multi<int1>(5);
    cat::verify(p_unalign_xsalloc_multi_bytes == 5);

    alloc_counter = 0;
    _ = allocator.unalign_xsalloc_multi<alloc_non_trivial>(5);
    cat::verify(alloc_counter == 5);

    // Test `opq_inline_salloc`.
    allocator.reset();
    auto [inline_salloc, inline_salloc_bytes] =
        allocator.opq_inline_salloc<int4>(1).value();
    cat::verify(allocator.get(inline_salloc) == 1);
    cat::verify(inline_salloc_bytes == cat::inline_buffer_size);
    cat::verify(inline_salloc.is_inline());

    auto [inline_salloc_big, inline_salloc_bytes_big] =
        allocator.opq_inline_salloc<alloc_huge_object>().value();
    cat::verify(inline_salloc_bytes_big == ssizeof(alloc_huge_object));
    cat::verify(!inline_salloc_big.is_inline());

    // Test `opq_inline_xsalloc`.
    allocator.reset();
    auto [inline_xsalloc, inline_xsalloc_bytes] =
        allocator.opq_inline_xsalloc<int4>(1);
    cat::verify(inline_xsalloc_bytes == cat::inline_buffer_size);
    cat::verify(inline_xsalloc.is_inline());

    auto [inline_xsalloc_big, inline_xsalloc_bytes_big] =
        allocator.opq_inline_xsalloc<alloc_huge_object>();
    cat::verify(inline_xsalloc_bytes_big == ssizeof(alloc_huge_object));
    cat::verify(!inline_xsalloc_big.is_inline());

    // Test `inline_salloc_multi`.
    allocator.reset();
    auto [inline_salloc_multi, inline_salloc_multi_bytes] =
        allocator.opq_inline_salloc_multi<int4>(5).value();
    cat::verify(inline_salloc_multi_bytes == cat::inline_buffer_size);
    cat::verify(inline_salloc_multi.is_inline());

    auto [inline_salloc_multi_big, inline_salloc_multi_bytes_big] =
        allocator.opq_inline_salloc_multi<alloc_huge_object>(5).value();
    cat::verify(inline_salloc_multi_bytes_big ==
                ssizeof(alloc_huge_object) * 5);
    cat::verify(!inline_salloc_multi_big.is_inline());

    // Test `inline_xsalloc_multi`.
    allocator.reset();
    auto [inline_xsalloc_multi, inline_xsalloc_multi_bytes] =
        allocator.opq_inline_xsalloc_multi<int4>(5);
    cat::verify(inline_xsalloc_multi_bytes == cat::inline_buffer_size);
    cat::verify(inline_xsalloc_multi.is_inline());

    auto [inline_xsalloc_multi_big, inline_xsalloc_multi_bytes_big] =
        allocator.opq_inline_xsalloc_multi<alloc_huge_object>(5);
    cat::verify(inline_xsalloc_multi_bytes_big ==
                ssizeof(alloc_huge_object) * 5);
    cat::verify(!inline_xsalloc_multi_big.is_inline());

    // Test `opq_inline_align_salloc`.
    allocator.reset();
    auto [inline_align_salloc, inline_align_salloc_bytes] =
        allocator.opq_inline_align_salloc<int4>(8u, 1).value();
    cat::verify(allocator.get(inline_align_salloc) == 1);
    cat::verify(inline_align_salloc_bytes >= cat::inline_buffer_size);
    cat::verify(inline_align_salloc.is_inline());

    auto [inline_align_salloc_big, inline_align_salloc_bytes_big] =
        allocator.opq_inline_align_salloc<alloc_huge_object>(8u).value();
    cat::verify(inline_align_salloc_bytes_big >= ssizeof(alloc_huge_object));
    cat::verify(!inline_align_salloc_big.is_inline());

    // Test `opq_inline_align_xsalloc`.
    allocator.reset();
    auto [inline_align_xsalloc, inline_align_xsalloc_bytes] =
        allocator.opq_inline_align_xsalloc<int4>(8u, 1);
    cat::verify(allocator.get(inline_align_xsalloc) == 1);
    cat::verify(inline_align_xsalloc_bytes >= cat::inline_buffer_size);
    cat::verify(inline_align_xsalloc.is_inline());

    auto [inline_align_xsalloc_big, inline_align_xsalloc_bytes_big] =
        allocator.opq_inline_align_xsalloc<alloc_huge_object>(8u);
    cat::verify(inline_align_xsalloc_bytes_big >= ssizeof(alloc_huge_object));
    cat::verify(!inline_align_xsalloc_big.is_inline());

    // Test `opq_inline_unalign_salloc`.
    allocator.reset();
    auto [inline_unalign_salloc, inline_unalign_salloc_bytes] =
        allocator.opq_inline_unalign_salloc<int4>(1).value();
    cat::verify(allocator.get(inline_unalign_salloc) == 1);
    cat::verify(inline_unalign_salloc_bytes == cat::inline_buffer_size);
    cat::verify(inline_unalign_salloc.is_inline());

    auto [inline_unalign_salloc_big, inline_unalign_salloc_bytes_big] =
        allocator.opq_inline_unalign_salloc<alloc_huge_object>().value();
    cat::verify(inline_unalign_salloc_bytes_big == ssizeof(alloc_huge_object));
    cat::verify(!inline_unalign_salloc_big.is_inline());

    // Test `opq_inline_unalign_xsalloc`.
    allocator.reset();
    auto [inline_unalign_xsalloc, inline_unalign_xsalloc_bytes] =
        allocator.opq_inline_unalign_xsalloc<int4>(1);
    cat::verify(allocator.get(inline_unalign_xsalloc) == 1);
    cat::verify(inline_unalign_xsalloc_bytes == cat::inline_buffer_size);
    cat::verify(inline_unalign_xsalloc.is_inline());

    auto [inline_unalign_xsalloc_big, inline_unalign_xsalloc_bytes_big] =
        allocator.opq_inline_unalign_xsalloc<alloc_huge_object>();
    cat::verify(inline_unalign_xsalloc_bytes_big == ssizeof(alloc_huge_object));
    cat::verify(!inline_unalign_xsalloc_big.is_inline());

    // Test `inline_align_salloc_multi`.
    allocator.reset();
    auto [inline_align_salloc_multi, inline_align_salloc_multi_bytes] =
        allocator.opq_inline_align_salloc_multi<int4>(8u, 5).value();
    cat::verify(inline_align_salloc_multi_bytes >= cat::inline_buffer_size);
    cat::verify(inline_align_salloc_multi.is_inline());

    auto [inline_align_salloc_multi_big, inline_align_salloc_multi_bytes_big] =
        allocator.opq_inline_align_salloc_multi<alloc_huge_object>(8u, 5)
            .value();
    cat::verify(inline_align_salloc_multi_bytes_big >=
                ssizeof(alloc_huge_object));
    cat::verify(!inline_align_salloc_multi_big.is_inline());

    // Test `inline_align_xsalloc_multi`.
    allocator.reset();
    auto [inline_align_xsalloc_multi, inline_align_xsalloc_multi_bytes] =
        allocator.opq_inline_align_xsalloc_multi<int4>(8u, 5);
    cat::verify(inline_align_xsalloc_multi_bytes >= cat::inline_buffer_size);
    cat::verify(inline_align_xsalloc_multi.is_inline());

    auto [inline_align_xsalloc_multi_big,
          inline_align_xsalloc_multi_bytes_big] =
        allocator.opq_inline_align_xsalloc_multi<alloc_huge_object>(8u, 5);
    cat::verify(inline_align_xsalloc_multi_bytes_big >=
                ssizeof(alloc_huge_object));
    cat::verify(!inline_align_xsalloc_multi_big.is_inline());

    // Test `inline_unalign_salloc_multi`.
    allocator.reset();
    auto [inline_unalign_salloc_multi, inline_unalign_salloc_multi_bytes] =
        allocator.opq_inline_unalign_salloc_multi<int4>(5).value();
    cat::verify(inline_unalign_salloc_multi_bytes == cat::inline_buffer_size);
    cat::verify(inline_unalign_salloc_multi.is_inline());

    auto [inline_unalign_salloc_multi_big,
          inline_unalign_salloc_multi_bytes_big] =
        allocator.opq_inline_unalign_salloc_multi<alloc_huge_object>(5).value();
    cat::verify(inline_unalign_salloc_multi_bytes_big ==
                ssizeof(alloc_huge_object) * 5);
    cat::verify(!inline_unalign_salloc_multi_big.is_inline());

    // Test `inline_unalign_xsalloc_multi`.
    allocator.reset();
    auto [inline_unalign_xsalloc_multi, inline_unalign_xsalloc_multi_bytes] =
        allocator.opq_inline_unalign_xsalloc_multi<int4>(5);
    cat::verify(inline_unalign_xsalloc_multi_bytes == cat::inline_buffer_size);
    cat::verify(inline_unalign_xsalloc_multi.is_inline());

    auto [inline_unalign_xsalloc_multi_big,
          inline_unalign_xsalloc_multi_bytes_big] =
        allocator.opq_inline_unalign_xsalloc_multi<alloc_huge_object>(5);
    cat::verify(inline_unalign_xsalloc_multi_bytes_big ==
                ssizeof(alloc_huge_object) * 5);
    cat::verify(!inline_unalign_xsalloc_multi_big.is_inline());

    // TODO: Test `opq_calloc` family more comprehensively.

    // Test `opq_calloc`.
    _ = allocator.opq_calloc<int4>().value();
    _ = allocator.opq_calloc<int4>(1).value();

    // Test `opq_xcalloc`.
    _ = allocator.opq_xcalloc<int4>();
    _ = allocator.opq_xcalloc<int4>(1);

    // Test `calloc`.
    _ = allocator.opq_calloc<int4>().value();
    _ = allocator.opq_calloc<int4>(1).value();

    // Test `xcalloc`.
    _ = allocator.xcalloc<int4>();
    _ = allocator.xcalloc<int4>(1);

    // Test `opq_align_calloc`.
    _ = allocator.opq_align_calloc<int4>(8u).value();
    _ = allocator.opq_align_calloc<int4>(8u, 1).value();

    // Test `opq_align_xcalloc`.
    _ = allocator.opq_align_xcalloc<int4>(8u);
    _ = allocator.opq_align_xcalloc<int4>(8u, 1);

    // Test `align_calloc`.
    _ = allocator.opq_align_calloc<int4>(8u).value();
    _ = allocator.opq_align_calloc<int4>(8u, 1).value();

    // Test `align_xcalloc`.
    _ = allocator.align_xcalloc<int4>(8u);
    _ = allocator.align_xcalloc<int4>(8u, 1);

    // Test `opq_unalign_calloc`.
    _ = allocator.opq_unalign_calloc<int1>().value();
    _ = allocator.opq_unalign_calloc<int1>(1).value();

    // Test `opq_unalign_xcalloc`.
    _ = allocator.opq_unalign_xcalloc<int1>();
    _ = allocator.opq_unalign_xcalloc<int1>(1);

    // Test `unalign_calloc`.
    _ = allocator.opq_unalign_calloc<int1>().value();
    _ = allocator.opq_unalign_calloc<int1>(1).value();

    // Test `unalign_xcalloc`.
    _ = allocator.unalign_xcalloc<int1>();
    _ = allocator.unalign_xcalloc<int1>(1);

    // Test `opq_inline_calloc`.
    _ = allocator.opq_inline_calloc<int4>().value();
    _ = allocator.opq_inline_calloc<int4>(1).value();

    // Test `opq_inline_xcalloc`.
    _ = allocator.opq_inline_xcalloc<int4>();
    _ = allocator.opq_inline_xcalloc<int4>(1);

    // Test `opq_inline_align_calloc`.
    _ = allocator.opq_inline_align_calloc<int4>(8u).value();
    _ = allocator.opq_inline_align_calloc<int4>(8u, 1).value();

    // Test `opq_inline_align_xcalloc`.
    _ = allocator.opq_inline_align_xcalloc<int4>(8u);
    _ = allocator.opq_inline_align_xcalloc<int4>(8u, 1);

    // Test `opq_inline_unalign_calloc`.
    _ = allocator.opq_inline_unalign_calloc<int4>().value();
    _ = allocator.opq_inline_unalign_calloc<int4>(1).value();

    // Test `opq_inline_unalign_xcalloc`.
    _ = allocator.opq_inline_unalign_xcalloc<int4>();
    _ = allocator.opq_inline_unalign_xcalloc<int4>(1);

    // Test `opq_xscalloc`.
    _ = allocator.opq_xscalloc<int4>().first();
    _ = allocator.opq_xscalloc<int4>(1).first();

    // Test `scalloc`.
    _ = allocator.opq_scalloc<int4>().value().first();
    _ = allocator.opq_scalloc<int4>(1).value().first();

    // Test `xscalloc`.
    _ = allocator.xscalloc<int4>().first();
    _ = allocator.xscalloc<int4>(1).first();

    // Test `opq_align_scalloc`.
    _ = allocator.opq_align_scalloc<int4>(8u).value().first();
    _ = allocator.opq_align_scalloc<int4>(8u, 1).value().first();

    // Test `opq_align_xscalloc`.
    _ = allocator.opq_align_xscalloc<int4>(8u).first();
    _ = allocator.opq_align_xscalloc<int4>(8u, 1).first();

    // Test `align_scalloc`.
    _ = allocator.opq_align_scalloc<int4>(8u).value().first();
    _ = allocator.opq_align_scalloc<int4>(8u, 1).value().first();

    // Test `align_xscalloc`.
    _ = allocator.align_xscalloc<int4>(8u).first();
    _ = allocator.align_xscalloc<int4>(8u, 1).first();

    // Test `opq_unalign_scalloc`.
    _ = allocator.opq_unalign_scalloc<int1>().value().first();
    _ = allocator.opq_unalign_scalloc<int1>(1).value().first();

    // Test `opq_unalign_xscalloc`.
    _ = allocator.opq_unalign_xscalloc<int1>().first();
    _ = allocator.opq_unalign_xscalloc<int1>(1).first();

    // Test `unalign_scalloc`.
    _ = allocator.opq_unalign_scalloc<int1>().value().first();
    _ = allocator.opq_unalign_scalloc<int1>(1).value().first();

    // Test `unalign_xscalloc`.
    _ = allocator.unalign_xscalloc<int1>().first();
    _ = allocator.unalign_xscalloc<int1>(1).first();

    // Test `opq_inline_scalloc`.
    _ = allocator.opq_inline_scalloc<int4>().value().first();
    _ = allocator.opq_inline_scalloc<int4>(1).value().first();

    // Test `opq_inline_xscalloc`.
    _ = allocator.opq_inline_xscalloc<int4>().first();
    _ = allocator.opq_inline_xscalloc<int4>(1).first();

    // Test `opq_inline_align_scalloc`.
    _ = allocator.opq_inline_align_scalloc<int4>(8u).value().first();
    _ = allocator.opq_inline_align_scalloc<int4>(8u, 1).value().first();

    // Test `opq_inline_align_xscalloc`.
    _ = allocator.opq_inline_align_xscalloc<int4>(8u).first();
    _ = allocator.opq_inline_align_xscalloc<int4>(8u, 1).first();

    // Test `opq_inline_unalign_scalloc`.
    _ = allocator.opq_inline_unalign_scalloc<int4>().value().first();
    _ = allocator.opq_inline_unalign_scalloc<int4>(1).value().first();

    // Test `opq_inline_unalign_xscalloc`.
    _ = allocator.opq_inline_unalign_xscalloc<int4>().first();
    _ = allocator.opq_inline_unalign_xscalloc<int4>(1).first();

    allocator.reset();

    // Test `opq_realloc`.
    auto realloc_1 = allocator.opq_alloc<int4>(1).value();
    auto realloc_2 = allocator.opq_alloc<int4>(2).value();
    cat::verify(allocator.get(realloc_1) == 1);
    cat::verify(allocator.get(realloc_2) == 2);
    realloc_1 = allocator.opq_realloc(realloc_2).value();
    cat::verify(allocator.get(realloc_1) == 2);

    // Test `realloc_to`.
    opq_alloc = allocator.opq_realloc_to(allocator, opq_alloc).value();

    // Test `realloc`.
    auto p_realloc_1 = allocator.alloc<int4>(1).value();
    auto p_realloc_2 = allocator.alloc<int4>(2).value();
    cat::verify(*p_realloc_1 == 1);
    cat::verify(*p_realloc_2 == 2);
    p_realloc_1 = allocator.realloc(p_realloc_2).value();
    cat::verify(*p_realloc_1 == 2);

    // Test `realloc_to`
    p_alloc = allocator.realloc_to(allocator, p_alloc).value();

    // Test `opq_xrealloc`.
    opq_alloc = allocator.opq_xrealloc(opq_alloc);

    // Test `opq_xrealloc_to`.
    opq_alloc = allocator.opq_xrealloc_to(allocator, opq_alloc);

    // Test `xrealloc`
    p_alloc = allocator.xrealloc(p_alloc);

    // Test `xrealloc_to`
    p_alloc = allocator.xrealloc_to(allocator, p_alloc);

    // Test `opq_align_realloc`.
    opq_alloc = allocator.opq_align_realloc(opq_alloc, 8u).value();

    // Test `opq_align_realloc_to`.
    opq_alloc =
        allocator.opq_align_realloc_to(allocator, opq_alloc, 8u).value();

    // Test `opq_align_xrealloc`.
    opq_alloc = allocator.opq_align_xrealloc(opq_alloc, 8u);

    // Test `opq_align_xrealloc_to`.
    opq_alloc = allocator.opq_align_xrealloc_to(allocator, opq_alloc, 8u);

    // Test `opq_unalign_realloc`.
    opq_alloc = allocator.opq_unalign_realloc(opq_alloc).value();

    // Test `opq_unalign_realloc_to`.
    opq_alloc = allocator.opq_unalign_realloc_to(allocator, opq_alloc).value();

    // Test `opq_unalign_xrealloc`.
    opq_alloc = allocator.opq_unalign_xrealloc(opq_alloc);

    // Test `unalign_xrealloc_to`.
    opq_alloc = allocator.opq_unalign_xrealloc_to(allocator, opq_alloc);

    // Test `opq_align_realloc`.
    opq_alloc = allocator.opq_align_realloc(opq_alloc, 8u).value();

    // Test `opq_align_realloc_to`.
    opq_alloc =
        allocator.opq_align_realloc_to(allocator, opq_alloc, 8u).value();

    // Test `opq_align_xrealloc`.
    opq_alloc = allocator.opq_align_xrealloc(opq_alloc, 8u);

    // Test `opq_align_xrealloc_to`.
    opq_alloc = allocator.opq_align_xrealloc_to(allocator, opq_alloc, 8u);

    // Test `opq_unalign_realloc`.
    opq_alloc = allocator.opq_unalign_realloc(opq_alloc).value();

    // Test `opq_unalign_realloc_to`.
    opq_alloc = allocator.opq_unalign_realloc_to(allocator, opq_alloc).value();

    // Test `opq_unalign_xrealloc`.
    opq_alloc = allocator.opq_unalign_xrealloc(opq_alloc);

    // Test `opq_unalign_xrealloc_to`.
    opq_alloc = allocator.opq_unalign_xrealloc_to(allocator, opq_alloc);

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

    // Test `opq_realloc_multi`.
    cat::mem auto opq_alloc_multi =
        allocator.opq_realloc_multi(opq_alloc, 10).value();

    // Test `opq_realloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    opq_alloc_multi =
        allocator.opq_realloc_multi_to(allocator, opq_alloc, 10).value();

    // Test `realloc_multi`.
    p_alloc = allocator.alloc<int4>().value();
    p_alloc = allocator.realloc_multi(p_alloc, 5, 10).value().data();
    allocator.free(opq_alloc);

    // Test `realloc_multi_to`
    p_alloc =
        allocator.realloc_multi_to(allocator, p_alloc, 5, 10).value().data();

    // Test `opq_xrealloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_xrealloc_multi(opq_alloc, 10);
    allocator.free(opq_alloc);

    // Test `opq_xrealloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_xrealloc_multi_to(allocator, opq_alloc, 10);
    allocator.free(opq_alloc);

    // Test `xrealloc_multi`
    p_alloc = allocator.xrealloc_multi(p_alloc, 5, 10).data();

    // Test `xrealloc_multi_to`
    p_alloc = allocator.xrealloc_multi_to(allocator, p_alloc, 5, 10).data();

    // Test `opq_align_realloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_align_realloc_multi(opq_alloc, 8u, 10).value();
    allocator.free(opq_alloc);

    // Test `opq_align_realloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_align_realloc_multi_to(allocator, opq_alloc, 8u, 10)
            .value();
    allocator.free(opq_alloc);

    // Test `opq_align_xrealloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_align_xrealloc_multi(opq_alloc, 8u, 10);
    allocator.free(opq_alloc);

    // Test `opq_align_xrealloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_align_xrealloc_multi_to(allocator, opq_alloc, 8u, 10);
    allocator.free(opq_alloc);

    // Test `opq_unalign_realloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_unalign_realloc_multi(opq_alloc, 10).value();
    allocator.free(opq_alloc);

    // Test `opq_unalign_realloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_unalign_realloc_multi_to(allocator, opq_alloc, 10)
            .value();
    allocator.free(opq_alloc);

    // Test `opq_unalign_xrealloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_unalign_xrealloc_multi(opq_alloc, 10);
    allocator.free(opq_alloc);

    // Test `opq_unalign_xrealloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_unalign_xrealloc_multi_to(allocator, opq_alloc, 10);
    allocator.free(opq_alloc);

    // Test `opq_align_realloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_align_realloc_multi(opq_alloc, 8u, 10).value();
    allocator.free(opq_alloc);

    // Test `opq_align_realloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_align_realloc_multi_to(allocator, opq_alloc, 8u, 10)
            .value();
    allocator.free(opq_alloc);

    // Test `opq_align_xrealloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_align_xrealloc_multi(opq_alloc, 8u, 10);
    allocator.free(opq_alloc);

    // Test `opq_align_xrealloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_align_xrealloc_multi_to(allocator, opq_alloc, 8u, 10);
    allocator.free(opq_alloc);

    // Test `opq_unalign_realloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_unalign_realloc_multi(opq_alloc, 10).value();
    allocator.free(opq_alloc);

    // Test `opq_unalign_realloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_unalign_realloc_multi_to(allocator, opq_alloc, 10)
            .value();
    allocator.free(opq_alloc);

    // Test `opq_unalign_xrealloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_unalign_xrealloc_multi(opq_alloc, 10);
    allocator.free(opq_alloc);

    // Test `opq_unalign_xrealloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_unalign_xrealloc_multi_to(allocator, opq_alloc, 10);
    allocator.free(opq_alloc);

    // Test `align_realloc_multi`.
    p_alloc = allocator.align_realloc_multi(p_alloc, 8u, 5, 10).value().data();

    // Test `align_realloc_multi_to`.
    p_alloc = allocator.align_realloc_multi_to(allocator, p_alloc, 8u, 5, 10)
                  .value()
                  .data();

    // Test `align_xrealloc_multi`.
    p_alloc = allocator.align_xrealloc_multi(p_alloc, 8u, 5, 10).data();

    // Test `align_xrealloc_multi_to`.
    p_alloc =
        allocator.align_xrealloc_multi_to(allocator, p_alloc, 8u, 5, 10).data();

    // Test `unalign_realloc_multi`.
    p_alloc = allocator.unalign_realloc_multi(p_alloc, 5, 10).value().data();

    // Test `unalign_realloc_multi_to`.
    p_alloc = allocator.unalign_realloc_multi_to(allocator, p_alloc, 5, 10)
                  .value()
                  .data();

    // Test `unalign_xrealloc_multi`.
    p_alloc = allocator.unalign_xrealloc_multi(p_alloc, 5, 10).data();

    // Test `unalign_xrealloc_multi_to`.
    p_alloc =
        allocator.unalign_xrealloc_multi_to(allocator, p_alloc, 5, 10).data();

    // The allocator runs out of memory around here.
    allocator.reset();
    // This poisons `p_alloc`, so reallocate it.
    p_alloc = allocator.alloc<int4>(1).value();

    // Test `opq_recalloc`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_recalloc(opq_alloc).value();
    allocator.free(opq_alloc);

    // Test `opq_recalloc_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_recalloc_to(allocator, opq_alloc).value();
    allocator.free(opq_alloc);

    // Test `recalloc`.
    p_alloc = allocator.recalloc(p_alloc).value();

    // Test `recalloc_to`
    p_alloc = allocator.recalloc_to(allocator, p_alloc).value();

    // Test `opq_xrecalloc`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_xrecalloc(opq_alloc);
    allocator.free(opq_alloc);

    // Test `opq_xrecalloc_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_xrecalloc_to(allocator, opq_alloc);
    allocator.free(opq_alloc);

    // Test `xrecalloc`
    p_alloc = allocator.xrecalloc(p_alloc);

    // Test `xrecalloc_to`
    p_alloc = allocator.xrecalloc_to(allocator, p_alloc);

    // Test `opq_align_recalloc`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_align_recalloc(opq_alloc, 8u).value();
    allocator.free(opq_alloc);

    // Test `opq_align_recalloc_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_align_recalloc_to(allocator, opq_alloc, 8u).value();
    allocator.free(opq_alloc);

    // Test `opq_align_xrecalloc`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_align_xrecalloc(opq_alloc, 8u);
    allocator.free(opq_alloc);

    // Test `opq_align_xrecalloc_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_align_xrecalloc_to(allocator, opq_alloc, 8u);
    allocator.free(opq_alloc);

    // Test `opq_unalign_recalloc`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_unalign_recalloc(opq_alloc).value();
    allocator.free(opq_alloc);

    // Test `opq_unalign_recalloc_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_unalign_recalloc_to(allocator, opq_alloc).value();
    allocator.free(opq_alloc);

    // Test `opq_unalign_xrecalloc`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_unalign_xrecalloc(opq_alloc);
    allocator.free(opq_alloc);

    // Test `opq_unalign_xrecalloc_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_unalign_xrecalloc_to(allocator, opq_alloc);
    allocator.free(opq_alloc);

    // Test `opq_align_recalloc`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_align_recalloc(opq_alloc, 8u).value();
    allocator.free(opq_alloc);

    // Test `opq_align_recalloc_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_align_recalloc_to(allocator, opq_alloc, 8u).value();
    allocator.free(opq_alloc);

    // Test `opq_align_xrecalloc`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_align_xrecalloc(opq_alloc, 8u);
    allocator.free(opq_alloc);

    // Test `opq_align_xrecalloc_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_align_xrecalloc_to(allocator, opq_alloc, 8u);
    allocator.free(opq_alloc);

    // Test `opq_unalign_recalloc`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_unalign_recalloc(opq_alloc).value();
    allocator.free(opq_alloc);

    // Test `opq_unalign_recalloc_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_unalign_recalloc_to(allocator, opq_alloc).value();
    allocator.free(opq_alloc);

    // Test `opq_unalign_xrecalloc`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_unalign_xrecalloc(opq_alloc);
    allocator.free(opq_alloc);

    // Test `opq_unalign_xrecalloc_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_unalign_xrecalloc_to(allocator, opq_alloc);
    allocator.free(opq_alloc);

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

    // Test `opq_recalloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_recalloc_multi(opq_alloc, 10).value();
    allocator.free(opq_alloc);

    // Test `opq_recalloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_recalloc_multi_to(allocator, opq_alloc, 10).value();
    allocator.free(opq_alloc);

    // Test `recalloc_multi`.
    p_alloc = allocator.recalloc_multi(p_alloc, 5, 10).value().data();

    // Test `recalloc_multi_to`
    p_alloc =
        allocator.recalloc_multi_to(allocator, p_alloc, 5, 10).value().data();

    // Test `opq_xrecalloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_xrecalloc_multi(opq_alloc, 10);
    allocator.free(opq_alloc);

    // Test `opq_xrecalloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_xrecalloc_multi_to(allocator, opq_alloc, 10);
    allocator.free(opq_alloc);

    // Test `xrecalloc_multi`
    p_alloc = allocator.xrecalloc_multi(p_alloc, 5, 10).data();

    // Test `xrecalloc_multi_to`
    p_alloc = allocator.xrecalloc_multi_to(allocator, p_alloc, 5, 10).data();

    // Test `opq_align_recalloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_align_recalloc_multi(opq_alloc, 8u, 10).value();
    allocator.free(opq_alloc);

    // Test `opq_align_recalloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_align_recalloc_multi_to(allocator, opq_alloc, 8u, 10)
            .value();
    allocator.free(opq_alloc);

    // Test `opq_align_xrecalloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_align_xrecalloc_multi(opq_alloc, 8u, 10);
    allocator.free(opq_alloc);

    // Test `opq_align_xrecalloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_align_xrecalloc_multi_to(allocator, opq_alloc, 8u, 10);
    allocator.free(opq_alloc);

    // Test `opq_unalign_recalloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_unalign_recalloc_multi(opq_alloc, 10).value();
    allocator.free(opq_alloc);

    // Test `opq_unalign_recalloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_unalign_recalloc_multi_to(allocator, opq_alloc, 10)
            .value();
    allocator.free(opq_alloc);

    // Test `opq_unalign_xrecalloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_unalign_xrecalloc_multi(opq_alloc, 10);
    allocator.free(opq_alloc);

    // Test `opq_unalign_xrecalloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_unalign_xrecalloc_multi_to(allocator, opq_alloc, 10);
    allocator.free(opq_alloc);

    // Test `opq_align_recalloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_align_recalloc_multi(opq_alloc, 8u, 10).value();
    allocator.free(opq_alloc);

    // Test `opq_align_recalloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_align_recalloc_multi_to(allocator, opq_alloc, 8u, 10)
            .value();
    allocator.free(opq_alloc);

    // Test `opq_align_xrecalloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_align_xrecalloc_multi(opq_alloc, 8u, 10);
    allocator.free(opq_alloc);

    // Test `opq_align_xrecalloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_align_xrecalloc_multi_to(allocator, opq_alloc, 8u, 10);
    allocator.free(opq_alloc);

    // Test `opq_unalign_recalloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_unalign_recalloc_multi(opq_alloc, 10).value();
    allocator.free(opq_alloc);

    // Test `opq_unalign_recalloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_unalign_recalloc_multi_to(allocator, opq_alloc, 10)
            .value();
    allocator.free(opq_alloc);

    // Test `opq_unalign_xrecalloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_unalign_xrecalloc_multi(opq_alloc, 10);
    allocator.free(opq_alloc);

    // Test `opq_unalign_xrecalloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_unalign_xrecalloc_multi_to(allocator, opq_alloc, 10);
    allocator.free(opq_alloc);

    // Test `align_recalloc_multi`.
    p_alloc = allocator.align_recalloc_multi(p_alloc, 8u, 5, 10).value().data();

    // Test `align_recalloc_multi_to`.
    p_alloc = allocator.align_recalloc_multi_to(allocator, p_alloc, 8u, 5, 10)
                  .value()
                  .data();

    // Test `align_xrecalloc_multi`.
    p_alloc = allocator.align_xrecalloc_multi(p_alloc, 8u, 5, 10).data();

    // Test `align_xrecalloc_multi_to`.
    p_alloc = allocator.align_xrecalloc_multi_to(allocator, p_alloc, 8u, 5, 10)
                  .data();

    // Test `unalign_recalloc_multi`.
    p_alloc = allocator.unalign_recalloc_multi(p_alloc, 5, 10).value().data();

    // Test `unalign_recalloc_multi_to`.
    p_alloc = allocator.unalign_recalloc_multi_to(allocator, p_alloc, 5, 10)
                  .value()
                  .data();

    // Test `unalign_xrecalloc_multi`.
    p_alloc = allocator.unalign_xrecalloc_multi(p_alloc, 5, 10).data();

    // Test `unalign_xrecalloc_multi_to`.
    p_alloc =
        allocator.unalign_xrecalloc_multi_to(allocator, p_alloc, 5, 10).data();

    // Test `opq_inline_realloc`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_realloc(opq_alloc);
    allocator.free(opq_alloc);

    // Test `opq_inline_realloc_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_realloc_to(allocator, opq_alloc).value();
    allocator.free(opq_alloc);

    // Test `opq_inline_xrealloc`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_xrealloc(opq_alloc);
    allocator.free(opq_alloc);

    // Test `opq_inline_xrealloc_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_xrealloc_to(allocator, opq_alloc);
    allocator.free(opq_alloc);

    // Test `opq_inline_align_realloc`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_align_realloc(opq_alloc, 8u).value();
    allocator.free(opq_alloc);

    // Test `opq_inline_align_realloc_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_align_realloc_to(allocator, opq_alloc, 8u).value();
    allocator.free(opq_alloc);

    // Test `opq_inline_align_xrealloc`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_align_xrealloc(opq_alloc, 8u);
    allocator.free(opq_alloc);

    // Test `opq_inline_align_xrealloc_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_align_xrealloc_to(allocator, opq_alloc, 8u);
    allocator.free(opq_alloc);

    // Test `opq_inline_unalign_realloc`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_unalign_realloc(opq_alloc).value();
    allocator.free(opq_alloc);

    // Test `opq_inline_unalign_realloc_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_unalign_realloc_to(allocator, opq_alloc).value();
    allocator.free(opq_alloc);

    // Test `opq_inline_unalign_xrealloc`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_unalign_xrealloc(opq_alloc);
    allocator.free(opq_alloc);

    // Test `opq_inline_unalign_xrealloc_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_unalign_xrealloc_to(allocator, opq_alloc);
    allocator.free(opq_alloc);

    // Test `opq_inline_align_realloc`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_align_realloc(opq_alloc, 8u).value();
    allocator.free(opq_alloc);

    // Test `opq_inline_align_realloc_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_align_realloc_to(allocator, opq_alloc, 8u).value();
    allocator.free(opq_alloc);

    // Test `opq_inline_align_xrealloc`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_align_xrealloc(opq_alloc, 8u);
    allocator.free(opq_alloc);

    // Test `opq_inline_align_xrealloc_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_align_xrealloc_to(allocator, opq_alloc, 8u);
    allocator.free(opq_alloc);

    // Test `opq_inline_unalign_realloc`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_unalign_realloc(opq_alloc).value();
    allocator.free(opq_alloc);

    // Test `opq_inline_unalign_realloc_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_unalign_realloc_to(allocator, opq_alloc).value();
    allocator.free(opq_alloc);

    // Test `opq_inline_unalign_xrealloc`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_unalign_xrealloc(opq_alloc);
    allocator.free(opq_alloc);

    // Test `opq_inline_unalign_xrealloc_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_unalign_xrealloc_to(allocator, opq_alloc);
    allocator.free(opq_alloc);

    // Test `opq_inline_realloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_realloc_multi(opq_alloc, 10).value();
    allocator.free(opq_alloc);

    // Test `opq_inline_realloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_realloc_multi_to(allocator, opq_alloc, 10).value();
    allocator.free(opq_alloc);

    // Test `opq_inline_xrealloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_xrealloc_multi(opq_alloc, 10);
    allocator.free(opq_alloc);

    // Test `opq_inline_xrealloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_xrealloc_multi_to(allocator, opq_alloc, 10);
    allocator.free(opq_alloc);

    // Test `opq_inline_align_realloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_align_realloc_multi(opq_alloc, 8u, 10).value();
    allocator.free(opq_alloc);

    // Test `inline_align_realloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator
            .opq_inline_align_realloc_multi_to(allocator, opq_alloc, 8u, 10)
            .value();
    allocator.free(opq_alloc);

    // Test `opq_inline_align_xrealloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_align_xrealloc_multi(opq_alloc, 8u, 10);
    allocator.free(opq_alloc);

    // Test `opq_inline_align_xrealloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_align_xrealloc_multi_to(allocator, opq_alloc, 8u,
                                                     10);
    allocator.free(opq_alloc);

    // Test `opq_inline_unalign_realloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_unalign_realloc_multi(opq_alloc, 10).value();
    allocator.free(opq_alloc);

    // Test `opq_inline_unalign_realloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_unalign_realloc_multi_to(allocator, opq_alloc, 10)
            .value();
    allocator.free(opq_alloc);

    // Test `opq_inline_unalign_xrealloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_unalign_xrealloc_multi(opq_alloc, 10);
    allocator.free(opq_alloc);

    // Test `opq_inline_unalign_xrealloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_unalign_xrealloc_multi_to(allocator, opq_alloc,
                                                       10);
    allocator.free(opq_alloc);

    // Test `opq_inline_align_realloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_align_realloc_multi(opq_alloc, 8u, 10).value();
    allocator.free(opq_alloc);

    // Test `inline_align_realloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator
            .opq_inline_align_realloc_multi_to(allocator, opq_alloc, 8u, 10)
            .value();
    allocator.free(opq_alloc);

    // Test `opq_inline_align_xrealloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_align_xrealloc_multi(opq_alloc, 8u, 10);
    allocator.free(opq_alloc);

    // Test `opq_inline_align_xrealloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_align_xrealloc_multi_to(allocator, opq_alloc, 8u,
                                                     10);
    allocator.free(opq_alloc);

    // Test `opq_inline_unalign_realloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_unalign_realloc_multi(opq_alloc, 10).value();
    allocator.free(opq_alloc);

    // Test `opq_inline_unalign_realloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_unalign_realloc_multi_to(allocator, opq_alloc, 10)
            .value();
    allocator.free(opq_alloc);

    // Test `opq_inline_unalign_xrealloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_unalign_xrealloc_multi(opq_alloc, 10);
    allocator.free(opq_alloc);

    // Test `opq_inline_unalign_xrealloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_unalign_xrealloc_multi_to(allocator, opq_alloc,
                                                       10);
    allocator.free(opq_alloc);

    // The allocator runs out of memory around here.
    allocator.reset();

    // Test `opq_inline_recalloc`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_recalloc(opq_alloc).value();
    allocator.free(opq_alloc);

    // Test `opq_inline_recalloc_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_recalloc_to(allocator, opq_alloc).value();
    allocator.free(opq_alloc);

    // Test `opq_inline_xrecalloc`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_xrecalloc(opq_alloc);
    allocator.free(opq_alloc);

    // Test `opq_inline_xrecalloc_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_xrecalloc_to(allocator, opq_alloc);
    allocator.free(opq_alloc);

    // Test `opq_inline_align_recalloc`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_align_recalloc(opq_alloc, 8u).value();
    allocator.free(opq_alloc);

    // Test `opq_inline_align_recalloc_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_align_recalloc_to(allocator, opq_alloc, 8u)
            .value();
    allocator.free(opq_alloc);

    // Test `opq_inline_align_xrecalloc`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_align_xrecalloc(opq_alloc, 8u);
    allocator.free(opq_alloc);

    // Test `opq_inline_align_xrecalloc_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_align_xrecalloc_to(allocator, opq_alloc, 8u);
    allocator.free(opq_alloc);

    // Test `opq_inline_unalign_recalloc`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_unalign_recalloc(opq_alloc).value();
    allocator.free(opq_alloc);

    // Test `opq_inline_unalign_recalloc_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_unalign_recalloc_to(allocator, opq_alloc).value();
    allocator.free(opq_alloc);

    // Test `opq_inline_unalign_xrecalloc`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_unalign_xrecalloc(opq_alloc);
    allocator.free(opq_alloc);

    // Test `opq_inline_unalign_xrecalloc_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_unalign_xrecalloc_to(allocator, opq_alloc);
    allocator.free(opq_alloc);

    // Test `opq_inline_align_recalloc`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_align_recalloc(opq_alloc, 8u).value();
    allocator.free(opq_alloc);

    // Test `opq_inline_align_recalloc_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_align_recalloc_to(allocator, opq_alloc, 8u)
            .value();
    allocator.free(opq_alloc);

    // Test `opq_inline_align_xrecalloc`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_align_xrecalloc(opq_alloc, 8u);
    allocator.free(opq_alloc);

    // Test `opq_inline_align_xrecalloc_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_align_xrecalloc_to(allocator, opq_alloc, 8u);
    allocator.free(opq_alloc);

    // Test `opq_inline_unalign_recalloc`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_unalign_recalloc(opq_alloc).value();
    allocator.free(opq_alloc);

    // Test `opq_inline_unalign_recalloc_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_unalign_recalloc_to(allocator, opq_alloc).value();
    allocator.free(opq_alloc);

    // Test `opq_inline_unalign_xrecalloc`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_unalign_xrecalloc(opq_alloc);
    allocator.free(opq_alloc);

    // Test `opq_inline_unalign_xrecalloc_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_unalign_xrecalloc_to(allocator, opq_alloc);
    allocator.free(opq_alloc);

    // Test `opq_inline_recalloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_recalloc_multi(opq_alloc, 10).value();
    allocator.free(opq_alloc);

    // Test `opq_inline_recalloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_recalloc_multi_to(allocator, opq_alloc, 10)
            .value();
    allocator.free(opq_alloc);

    // Test `opq_inline_xrecalloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_xrecalloc_multi(opq_alloc, 10);
    allocator.free(opq_alloc);

    // Test `opq_inline_xrecalloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_xrecalloc_multi_to(allocator, opq_alloc, 10);
    allocator.free(opq_alloc);

    // Test `opq_inline_align_recalloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_align_recalloc_multi(opq_alloc, 8u, 10).value();
    allocator.free(opq_alloc);

    // Test `inline_align_recalloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator
            .opq_inline_align_recalloc_multi_to(allocator, opq_alloc, 8u, 10)
            .value();
    allocator.free(opq_alloc);

    // Test `opq_inline_align_xrecalloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_align_xrecalloc_multi(opq_alloc, 8u, 10);
    allocator.free(opq_alloc);

    // Test `opq_inline_align_xrecalloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_align_xrecalloc_multi_to(allocator, opq_alloc, 8u,
                                                      10);
    allocator.free(opq_alloc);

    // Test `opq_inline_unalign_recalloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_unalign_recalloc_multi(opq_alloc, 10).value();
    allocator.free(opq_alloc);

    // Test `opq_inline_unalign_recalloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_unalign_recalloc_multi_to(allocator, opq_alloc, 10)
            .value();
    allocator.free(opq_alloc);

    // Test `opq_inline_unalign_xrecalloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_unalign_xrecalloc_multi(opq_alloc, 10);
    allocator.free(opq_alloc);

    // Test `opq_inline_unalign_xrecalloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_unalign_xrecalloc_multi_to(allocator, opq_alloc,
                                                        10);
    allocator.free(opq_alloc);

    // Test `opq_inline_align_recalloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_align_recalloc_multi(opq_alloc, 8u, 10).value();
    allocator.free(opq_alloc);

    // Test `inline_align_recalloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator
            .opq_inline_align_recalloc_multi_to(allocator, opq_alloc, 8u, 10)
            .value();
    allocator.free(opq_alloc);

    // Test `opq_inline_align_xrecalloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_align_xrecalloc_multi(opq_alloc, 8u, 10);
    allocator.free(opq_alloc);

    // Test `opq_inline_align_xrecalloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_align_xrecalloc_multi_to(allocator, opq_alloc, 8u,
                                                      10);
    allocator.free(opq_alloc);

    // Test `opq_inline_unalign_recalloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_unalign_recalloc_multi(opq_alloc, 10).value();
    allocator.free(opq_alloc);

    // Test `opq_inline_unalign_recalloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_unalign_recalloc_multi_to(allocator, opq_alloc, 10)
            .value();
    allocator.free(opq_alloc);

    // Test `opq_inline_unalign_xrecalloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_unalign_xrecalloc_multi(opq_alloc, 10);
    allocator.free(opq_alloc);

    // Test `opq_inline_unalign_xrecalloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_unalign_xrecalloc_multi_to(allocator, opq_alloc,
                                                        10);
    allocator.free(opq_alloc);

    // Test `opq_resalloc`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_resalloc(opq_alloc).value();
    allocator.free(opq_alloc);

    // Test `opq_resalloc_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_resalloc_to(allocator, opq_alloc).value();
    allocator.free(opq_alloc);

    // Test `resalloc`.
    p_alloc = allocator.alloc<int4>(0).value();
    p_alloc = allocator.resalloc(p_alloc).value().first();

    // Test `resalloc_to`
    p_alloc = allocator.resalloc_to(allocator, p_alloc).value().first();

    // Test `opq_xresalloc`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_xresalloc(opq_alloc);
    allocator.free(opq_alloc);

    // Test `opq_xresalloc_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_xresalloc_to(allocator, opq_alloc);
    allocator.free(opq_alloc);

    // Test `xresalloc`
    p_alloc = allocator.xresalloc(p_alloc).first();

    // Test `xresalloc_to`
    p_alloc = allocator.xresalloc_to(allocator, p_alloc).first();

    // Test `opq_align_resalloc`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_align_resalloc(opq_alloc, 8u).value();
    allocator.free(opq_alloc);

    // Test `opq_align_resalloc_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_align_resalloc_to(allocator, opq_alloc, 8u).value();
    allocator.free(opq_alloc);

    // Test `opq_align_xresalloc`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_align_xresalloc(opq_alloc, 8u);
    allocator.free(opq_alloc);

    // Test `opq_align_xresalloc_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_align_xresalloc_to(allocator, opq_alloc, 8u);
    allocator.free(opq_alloc);

    // Test `opq_unalign_resalloc`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_unalign_resalloc(opq_alloc).value();
    allocator.free(opq_alloc);

    // Test `opq_unalign_resalloc_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_unalign_resalloc_to(allocator, opq_alloc).value();
    allocator.free(opq_alloc);

    // Test `opq_unalign_xresalloc`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_unalign_xresalloc(opq_alloc);
    allocator.free(opq_alloc);

    // Test `opq_unalign_xresalloc_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_unalign_xresalloc_to(allocator, opq_alloc);
    allocator.free(opq_alloc);

    // Test `opq_align_resalloc`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_align_resalloc(opq_alloc, 8u).value();
    allocator.free(opq_alloc);

    // Test `opq_align_resalloc_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_align_resalloc_to(allocator, opq_alloc, 8u).value();
    allocator.free(opq_alloc);

    opq_alloc = allocator.opq_alloc<int4>().value();
    // Test `opq_align_xresalloc`.
    _ = allocator.opq_align_xresalloc(opq_alloc, 8u);
    allocator.free(opq_alloc);

    // Test `opq_align_xresalloc_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_align_xresalloc_to(allocator, opq_alloc, 8u);
    allocator.free(opq_alloc);

    // Test `opq_unalign_resalloc`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_unalign_resalloc(opq_alloc).value();
    allocator.free(opq_alloc);

    // Test `opq_unalign_resalloc_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_unalign_resalloc_to(allocator, opq_alloc).value();
    allocator.free(opq_alloc);

    // Test `opq_unalign_xresalloc`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_unalign_xresalloc(opq_alloc);
    allocator.free(opq_alloc);

    // Test `opq_unalign_xresalloc_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_unalign_xresalloc_to(allocator, opq_alloc);
    allocator.free(opq_alloc);

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

    // Test `opq_resalloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_resalloc_multi(opq_alloc, 10).value();
    allocator.free(opq_alloc);

    // Test `opq_resalloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_resalloc_multi_to(allocator, opq_alloc, 10).value();
    allocator.free(opq_alloc);

    // Test `resalloc_multi`.
    p_alloc = allocator.resalloc_multi(p_alloc, 5, 10).value().first().data();

    // Test `resalloc_multi_to`
    p_alloc = allocator.resalloc_multi_to(allocator, p_alloc, 5, 10)
                  .value()
                  .first()
                  .data();

    // Test `opq_xresalloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_xresalloc_multi(opq_alloc, 10);
    allocator.free(opq_alloc);

    // Test `opq_xresalloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_xresalloc_multi_to(allocator, opq_alloc, 10);
    allocator.free(opq_alloc);

    // Test `xresalloc_multi`
    p_alloc = allocator.xresalloc_multi(p_alloc, 5, 10).first().data();

    // Test `xresalloc_multi_to`
    p_alloc =
        allocator.xresalloc_multi_to(allocator, p_alloc, 5, 10).first().data();

    // Test `opq_align_resalloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_align_resalloc_multi(opq_alloc, 8u, 10).value();
    allocator.free(opq_alloc);

    // Test `opq_align_resalloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_align_resalloc_multi_to(allocator, opq_alloc, 8u, 10)
            .value();
    allocator.free(opq_alloc);

    // Test `opq_align_xresalloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_align_xresalloc_multi(opq_alloc, 8u, 10);
    allocator.free(opq_alloc);

    // Test `opq_align_xresalloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_align_xresalloc_multi_to(allocator, opq_alloc, 8u, 10);
    allocator.free(opq_alloc);

    // Test `opq_unalign_resalloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_unalign_resalloc_multi(opq_alloc, 10).value();
    allocator.free(opq_alloc);

    // Test `opq_unalign_resalloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_unalign_resalloc_multi_to(allocator, opq_alloc, 10)
            .value();
    allocator.free(opq_alloc);

    // Test `opq_unalign_xresalloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_unalign_xresalloc_multi(opq_alloc, 10);
    allocator.free(opq_alloc);

    // Test `opq_unalign_xresalloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_unalign_xresalloc_multi_to(allocator, opq_alloc, 10);
    allocator.free(opq_alloc);

    // Test `opq_align_resalloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_align_resalloc_multi(opq_alloc, 8u, 10).value();
    allocator.free(opq_alloc);

    // Test `opq_align_resalloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_align_resalloc_multi_to(allocator, opq_alloc, 8u, 10)
            .value();
    allocator.free(opq_alloc);

    // Test `opq_align_xresalloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_align_xresalloc_multi(opq_alloc, 8u, 10);
    allocator.free(opq_alloc);

    // Test `opq_align_xresalloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_align_xresalloc_multi_to(allocator, opq_alloc, 8u, 10);
    allocator.free(opq_alloc);

    // Test `opq_unalign_resalloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_unalign_resalloc_multi(opq_alloc, 10).value();
    allocator.free(opq_alloc);

    // Test `opq_unalign_resalloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_unalign_resalloc_multi_to(allocator, opq_alloc, 10)
            .value();
    allocator.free(opq_alloc);

    // Test `opq_unalign_xresalloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_unalign_xresalloc_multi(opq_alloc, 10);
    allocator.free(opq_alloc);

    // Test `opq_unalign_xresalloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_unalign_xresalloc_multi_to(allocator, opq_alloc, 10);
    allocator.free(opq_alloc);

    // Test `align_resalloc_multi`.
    p_alloc = allocator.align_resalloc_multi(p_alloc, 8u, 5, 10)
                  .value()
                  .first()
                  .data();

    // Test `align_resalloc_multi_to`.
    p_alloc = allocator.align_resalloc_multi_to(allocator, p_alloc, 8u, 5, 10)
                  .value()
                  .first()
                  .data();

    // Test `align_xresalloc_multi`.
    p_alloc =
        allocator.align_xresalloc_multi(p_alloc, 8u, 5, 10).first().data();

    // Test `align_xresalloc_multi_to`.
    p_alloc = allocator.align_xresalloc_multi_to(allocator, p_alloc, 8u, 5, 10)
                  .first()
                  .data();

    // Test `unalign_resalloc_multi`.
    p_alloc =
        allocator.unalign_resalloc_multi(p_alloc, 5, 10).value().first().data();

    // Test `unalign_resalloc_multi_to`.
    p_alloc = allocator.unalign_resalloc_multi_to(allocator, p_alloc, 5, 10)
                  .value()
                  .first()
                  .data();

    // Test `unalign_xresalloc_multi`.
    p_alloc = allocator.unalign_xresalloc_multi(p_alloc, 5, 10).first().data();

    // Test `unalign_xresalloc_multi_to`.
    p_alloc = allocator.unalign_xresalloc_multi_to(allocator, p_alloc, 5, 10)
                  .first()
                  .data();

    // The allocator runs out of memory around here.
    allocator.reset();

    // Test `opq_rescalloc`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_rescalloc(opq_alloc).value();
    allocator.free(opq_alloc);

    // Test `opq_rescalloc_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_rescalloc_to(allocator, opq_alloc).value();
    allocator.free(opq_alloc);

    // Test `rescalloc`.
    p_alloc = allocator.alloc<int4>(0).value();
    p_alloc = allocator.rescalloc(p_alloc).value().first();

    // Test `rescalloc_to`
    p_alloc = allocator.rescalloc_to(allocator, p_alloc).value().first();

    // Test `opq_xrescalloc`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_xrescalloc(opq_alloc);
    allocator.free(opq_alloc);

    // Test `opq_xrescalloc_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_xrescalloc_to(allocator, opq_alloc);
    allocator.free(opq_alloc);

    // Test `xrescalloc`
    p_alloc = allocator.xrescalloc(p_alloc).first();

    // Test `xrescalloc_to`
    p_alloc = allocator.xrescalloc_to(allocator, p_alloc).first();

    // Test `opq_align_rescalloc`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_align_rescalloc(opq_alloc, 8u).value();
    allocator.free(opq_alloc);

    // Test `opq_align_rescalloc_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_align_rescalloc_to(allocator, opq_alloc, 8u).value();
    allocator.free(opq_alloc);

    // Test `opq_align_xrescalloc`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_align_xrescalloc(opq_alloc, 8u);
    allocator.free(opq_alloc);

    // Test `opq_align_xrescalloc_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_align_xrescalloc_to(allocator, opq_alloc, 8u);
    allocator.free(opq_alloc);

    // Test `opq_unalign_rescalloc`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_unalign_rescalloc(opq_alloc).value();
    allocator.free(opq_alloc);

    // Test `opq_unalign_rescalloc_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_unalign_rescalloc_to(allocator, opq_alloc).value();
    allocator.free(opq_alloc);

    // Test `opq_unalign_xrescalloc`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_unalign_xrescalloc(opq_alloc);
    allocator.free(opq_alloc);

    // Test `opq_unalign_xrescalloc_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_unalign_xrescalloc_to(allocator, opq_alloc);
    allocator.free(opq_alloc);

    // Test `opq_align_rescalloc`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_align_rescalloc(opq_alloc, 8u).value();
    allocator.free(opq_alloc);

    // Test `opq_align_rescalloc_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_align_rescalloc_to(allocator, opq_alloc, 8u).value();
    allocator.free(opq_alloc);

    // Test `opq_align_xrescalloc`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_align_xrescalloc(opq_alloc, 8u);
    allocator.free(opq_alloc);

    // Test `opq_align_xrescalloc_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_align_xrescalloc_to(allocator, opq_alloc, 8u);
    allocator.free(opq_alloc);

    // Test `opq_unalign_rescalloc`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_unalign_rescalloc(opq_alloc).value();
    allocator.free(opq_alloc);

    // Test `opq_unalign_rescalloc_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_unalign_rescalloc_to(allocator, opq_alloc).value();
    allocator.free(opq_alloc);

    // Test `opq_unalign_xrescalloc`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_unalign_xrescalloc(opq_alloc);
    allocator.free(opq_alloc);

    // Test `opq_unalign_xrescalloc_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_unalign_xrescalloc_to(allocator, opq_alloc);
    allocator.free(opq_alloc);

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

    // Test `opq_rescalloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_rescalloc_multi(opq_alloc, 10).value();
    allocator.free(opq_alloc);

    // Test `opq_rescalloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_rescalloc_multi_to(allocator, opq_alloc, 10).value();
    allocator.free(opq_alloc);

    // Test `rescalloc_multi`.
    p_alloc = allocator.rescalloc_multi(p_alloc, 5, 10).value().first().data();

    // Test `rescalloc_multi_to`
    p_alloc = allocator.rescalloc_multi_to(allocator, p_alloc, 5, 10)
                  .value()
                  .first()
                  .data();

    // Test `opq_xrescalloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_xrescalloc_multi(opq_alloc, 10);
    allocator.free(opq_alloc);

    // Test `opq_xrescalloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_xrescalloc_multi_to(allocator, opq_alloc, 10);
    allocator.free(opq_alloc);

    // Test `xrescalloc_multi`
    p_alloc = allocator.xrescalloc_multi(p_alloc, 5, 10).first().data();

    // Test `xrescalloc_multi_to`
    p_alloc =
        allocator.xrescalloc_multi_to(allocator, p_alloc, 5, 10).first().data();

    // Test `opq_align_rescalloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_align_rescalloc_multi(opq_alloc, 8u, 10).value();
    allocator.free(opq_alloc);

    // Test `opq_align_rescalloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_align_rescalloc_multi_to(allocator, opq_alloc, 8u, 10)
            .value();
    allocator.free(opq_alloc);

    // Test `opq_align_xrescalloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_align_xrescalloc_multi(opq_alloc, 8u, 10);
    allocator.free(opq_alloc);

    // Test `opq_align_xrescalloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_align_xrescalloc_multi_to(allocator, opq_alloc, 8u, 10);
    allocator.free(opq_alloc);

    // Test `opq_unalign_rescalloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_unalign_rescalloc_multi(opq_alloc, 10).value();
    allocator.free(opq_alloc);

    // Test `opq_unalign_rescalloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_unalign_rescalloc_multi_to(allocator, opq_alloc, 10)
            .value();
    allocator.free(opq_alloc);

    // Test `opq_unalign_xrescalloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_unalign_xrescalloc_multi(opq_alloc, 10);
    allocator.free(opq_alloc);

    // Test `opq_unalign_xrescalloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_unalign_xrescalloc_multi_to(allocator, opq_alloc, 10);
    allocator.free(opq_alloc);

    // Test `opq_align_rescalloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_align_rescalloc_multi(opq_alloc, 8u, 10).value();
    allocator.free(opq_alloc);

    // Test `opq_align_rescalloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_align_rescalloc_multi_to(allocator, opq_alloc, 8u, 10)
            .value();
    allocator.free(opq_alloc);

    // Test `opq_align_xrescalloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_align_xrescalloc_multi(opq_alloc, 8u, 10);
    allocator.free(opq_alloc);

    // Test `opq_align_xrescalloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_align_xrescalloc_multi_to(allocator, opq_alloc, 8u, 10);
    allocator.free(opq_alloc);

    // Test `opq_unalign_rescalloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_unalign_rescalloc_multi(opq_alloc, 10).value();
    allocator.free(opq_alloc);

    // Test `opq_unalign_rescalloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_unalign_rescalloc_multi_to(allocator, opq_alloc, 10)
            .value();
    allocator.free(opq_alloc);

    // Test `opq_unalign_xrescalloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_unalign_xrescalloc_multi(opq_alloc, 10);
    allocator.free(opq_alloc);

    // Test `opq_unalign_xrescalloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_unalign_xrescalloc_multi_to(allocator, opq_alloc, 10);
    allocator.free(opq_alloc);

    // Test `align_rescalloc_multi`.
    p_alloc = allocator.align_rescalloc_multi(p_alloc, 8u, 5, 10)
                  .value()
                  .first()
                  .data();

    // Test `align_rescalloc_multi_to`.
    p_alloc = allocator.align_rescalloc_multi_to(allocator, p_alloc, 8u, 5, 10)
                  .value()
                  .first()
                  .data();

    // Test `align_xrescalloc_multi`.
    p_alloc =
        allocator.align_xrescalloc_multi(p_alloc, 8u, 5, 10).first().data();

    // Test `align_xrescalloc_multi_to`.
    p_alloc = allocator.align_xrescalloc_multi_to(allocator, p_alloc, 8u, 5, 10)
                  .first()
                  .data();

    // Test `unalign_rescalloc_multi`.
    p_alloc = allocator.unalign_rescalloc_multi(p_alloc, 5, 10)
                  .value()
                  .first()
                  .data();

    // Test `unalign_rescalloc_multi_to`.
    p_alloc = allocator.unalign_rescalloc_multi_to(allocator, p_alloc, 5, 10)
                  .value()
                  .first()
                  .data();

    // Test `unalign_xrescalloc_multi`.
    p_alloc = allocator.unalign_xrescalloc_multi(p_alloc, 5, 10).first().data();

    // Test `unalign_xrescalloc_multi_to`.
    p_alloc = allocator.unalign_xrescalloc_multi_to(allocator, p_alloc, 5, 10)
                  .first()
                  .data();

    // Test `opq_inline_resalloc`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_resalloc(opq_alloc);
    allocator.free(opq_alloc);

    // Test `opq_inline_resalloc_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_resalloc_to(allocator, opq_alloc).value();
    allocator.free(opq_alloc);

    // Test `opq_inline_xresalloc`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_xresalloc(opq_alloc);
    allocator.free(opq_alloc);

    // Test `opq_inline_xresalloc_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_xresalloc_to(allocator, opq_alloc);
    allocator.free(opq_alloc);

    // Test `opq_inline_align_resalloc`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_align_resalloc(opq_alloc, 8u).value();
    allocator.free(opq_alloc);

    // Test `opq_inline_align_resalloc_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_align_resalloc_to(allocator, opq_alloc, 8u)
            .value();
    allocator.free(opq_alloc);

    // Test `opq_inline_align_xresalloc`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_align_xresalloc(opq_alloc, 8u);
    allocator.free(opq_alloc);

    // Test `opq_inline_align_xresalloc_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_align_xresalloc_to(allocator, opq_alloc, 8u);
    allocator.free(opq_alloc);

    // Test `opq_inline_unalign_resalloc`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_unalign_resalloc(opq_alloc).value();
    allocator.free(opq_alloc);

    // Test `opq_inline_unalign_resalloc_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_unalign_resalloc_to(allocator, opq_alloc).value();
    allocator.free(opq_alloc);

    // Test `opq_inline_unalign_xresalloc`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_unalign_xresalloc(opq_alloc);
    allocator.free(opq_alloc);

    // Test `opq_inline_unalign_xresalloc_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_unalign_xresalloc_to(allocator, opq_alloc);
    allocator.free(opq_alloc);

    // Test `opq_inline_align_resalloc`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_align_resalloc(opq_alloc, 8u).value();
    allocator.free(opq_alloc);

    // Test `opq_inline_align_resalloc_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_align_resalloc_to(allocator, opq_alloc, 8u)
            .value();
    allocator.free(opq_alloc);

    // Test `opq_inline_align_xresalloc`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_align_xresalloc(opq_alloc, 8u);
    allocator.free(opq_alloc);

    // Test `opq_inline_align_xresalloc_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_align_xresalloc_to(allocator, opq_alloc, 8u);
    allocator.free(opq_alloc);

    // Test `opq_inline_unalign_resalloc`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_unalign_resalloc(opq_alloc).value();
    allocator.free(opq_alloc);

    // Test `opq_inline_unalign_resalloc_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_unalign_resalloc_to(allocator, opq_alloc).value();
    allocator.free(opq_alloc);

    // Test `opq_inline_unalign_xresalloc`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_unalign_xresalloc(opq_alloc);
    allocator.free(opq_alloc);

    // Test `opq_inline_unalign_xresalloc_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_unalign_xresalloc_to(allocator, opq_alloc);
    allocator.free(opq_alloc);

    // Test `opq_inline_resalloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_resalloc_multi(opq_alloc, 10).value();
    allocator.free(opq_alloc);

    // Test `opq_inline_resalloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_resalloc_multi_to(allocator, opq_alloc, 10)
            .value();
    allocator.free(opq_alloc);

    // Test `opq_inline_xresalloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_xresalloc_multi(opq_alloc, 10);
    allocator.free(opq_alloc);

    // Test `opq_inline_xresalloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_xresalloc_multi_to(allocator, opq_alloc, 10);
    allocator.free(opq_alloc);

    // Test `opq_inline_align_resalloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_align_resalloc_multi(opq_alloc, 8u, 10).value();
    allocator.free(opq_alloc);

    // Test `inline_align_resalloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator
            .opq_inline_align_resalloc_multi_to(allocator, opq_alloc, 8u, 10)
            .value();
    allocator.free(opq_alloc);

    // Test `opq_inline_align_xresalloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_align_xresalloc_multi(opq_alloc, 8u, 10);
    allocator.free(opq_alloc);

    // Test `opq_inline_align_xresalloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_align_xresalloc_multi_to(allocator, opq_alloc, 8u,
                                                      10);
    allocator.free(opq_alloc);

    // Test `opq_inline_unalign_resalloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_unalign_resalloc_multi(opq_alloc, 10).value();
    allocator.free(opq_alloc);

    // Test `opq_inline_unalign_resalloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_unalign_resalloc_multi_to(allocator, opq_alloc, 10)
            .value();
    allocator.free(opq_alloc);

    // Test `opq_inline_unalign_xresalloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_unalign_xresalloc_multi(opq_alloc, 10);
    allocator.free(opq_alloc);

    // Test `opq_inline_unalign_xresalloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_unalign_xresalloc_multi_to(allocator, opq_alloc,
                                                        10);
    allocator.free(opq_alloc);

    // Test `opq_inline_align_resalloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_align_resalloc_multi(opq_alloc, 8u, 10).value();
    allocator.free(opq_alloc);

    // Test `inline_align_resalloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator
            .opq_inline_align_resalloc_multi_to(allocator, opq_alloc, 8u, 10)
            .value();
    allocator.free(opq_alloc);

    // Test `opq_inline_align_xresalloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_align_xresalloc_multi(opq_alloc, 8u, 10);
    allocator.free(opq_alloc);

    // Test `opq_inline_align_xresalloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_align_xresalloc_multi_to(allocator, opq_alloc, 8u,
                                                      10);
    allocator.free(opq_alloc);

    // Test `opq_inline_unalign_resalloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_unalign_resalloc_multi(opq_alloc, 10).value();
    allocator.free(opq_alloc);

    // Test `opq_inline_unalign_resalloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_unalign_resalloc_multi_to(allocator, opq_alloc, 10)
            .value();
    allocator.free(opq_alloc);

    // Test `opq_inline_unalign_xresalloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_unalign_xresalloc_multi(opq_alloc, 10);
    allocator.free(opq_alloc);

    // Test `opq_inline_unalign_xresalloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_unalign_xresalloc_multi_to(allocator, opq_alloc,
                                                        10);
    allocator.free(opq_alloc);

    // The allocator runs out of memory around here.
    allocator.reset();

    // Test `opq_inline_rescalloc`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_rescalloc(opq_alloc).value();
    allocator.free(opq_alloc);

    // Test `opq_inline_rescalloc_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_rescalloc_to(allocator, opq_alloc).value();
    allocator.free(opq_alloc);

    // Test `opq_inline_xrescalloc`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_xrescalloc(opq_alloc);
    allocator.free(opq_alloc);

    // Test `opq_inline_xrescalloc_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_xrescalloc_to(allocator, opq_alloc);
    allocator.free(opq_alloc);

    // Test `opq_inline_align_rescalloc`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_align_rescalloc(opq_alloc, 8u).value();
    allocator.free(opq_alloc);

    // Test `opq_inline_align_rescalloc_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_align_rescalloc_to(allocator, opq_alloc, 8u)
            .value();
    allocator.free(opq_alloc);

    // Test `opq_inline_align_xrescalloc`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_align_xrescalloc(opq_alloc, 8u);
    allocator.free(opq_alloc);

    // Test `opq_inline_align_xrescalloc_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_align_xrescalloc_to(allocator, opq_alloc, 8u);
    allocator.free(opq_alloc);

    // Test `opq_inline_unalign_rescalloc`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_unalign_rescalloc(opq_alloc).value();
    allocator.free(opq_alloc);

    // Test `opq_inline_unalign_rescalloc_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_unalign_rescalloc_to(allocator, opq_alloc).value();
    allocator.free(opq_alloc);

    // Test `opq_inline_unalign_xrescalloc`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_unalign_xrescalloc(opq_alloc);
    allocator.free(opq_alloc);

    // Test `opq_inline_unalign_xrescalloc_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_unalign_xrescalloc_to(allocator, opq_alloc);
    allocator.free(opq_alloc);

    // Test `opq_inline_align_rescalloc`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_align_rescalloc(opq_alloc, 8u).value();
    allocator.free(opq_alloc);

    // Test `opq_inline_align_rescalloc_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_align_rescalloc_to(allocator, opq_alloc, 8u)
            .value();
    allocator.free(opq_alloc);

    // Test `opq_inline_align_xrescalloc`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_align_xrescalloc(opq_alloc, 8u);
    allocator.free(opq_alloc);

    // Test `opq_inline_align_xrescalloc_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_align_xrescalloc_to(allocator, opq_alloc, 8u);
    allocator.free(opq_alloc);

    // Test `opq_inline_unalign_rescalloc`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_unalign_rescalloc(opq_alloc).value();
    allocator.free(opq_alloc);

    // Test `opq_inline_unalign_rescalloc_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_unalign_rescalloc_to(allocator, opq_alloc).value();
    allocator.free(opq_alloc);

    // Test `opq_inline_unalign_xrescalloc`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_unalign_xrescalloc(opq_alloc);
    allocator.free(opq_alloc);

    // Test `opq_inline_unalign_xrescalloc_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_unalign_xrescalloc_to(allocator, opq_alloc);
    allocator.free(opq_alloc);

    // Test `opq_inline_rescalloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_rescalloc_multi(opq_alloc, 10).value();
    allocator.free(opq_alloc);

    // Test `opq_inline_rescalloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_rescalloc_multi_to(allocator, opq_alloc, 10)
            .value();
    allocator.free(opq_alloc);

    // Test `opq_inline_xrescalloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_xrescalloc_multi(opq_alloc, 10);
    allocator.free(opq_alloc);

    // Test `opq_inline_xrescalloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_xrescalloc_multi_to(allocator, opq_alloc, 10);
    allocator.free(opq_alloc);

    // Test `opq_inline_align_rescalloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_align_rescalloc_multi(opq_alloc, 8u, 10).value();
    allocator.free(opq_alloc);

    // Test `inline_align_rescalloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator
            .opq_inline_align_rescalloc_multi_to(allocator, opq_alloc, 8u, 10)
            .value();
    allocator.free(opq_alloc);

    // Test `opq_inline_align_xrescalloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_align_xrescalloc_multi(opq_alloc, 8u, 10);
    allocator.free(opq_alloc);

    // Test `opq_inline_align_xrescalloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_align_xrescalloc_multi_to(allocator, opq_alloc, 8u,
                                                       10);
    allocator.free(opq_alloc);

    // Test `opq_inline_unalign_rescalloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_unalign_rescalloc_multi(opq_alloc, 10).value();
    allocator.free(opq_alloc);

    // Test `inline_unalign_rescalloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator
            .opq_inline_unalign_rescalloc_multi_to(allocator, opq_alloc, 10)
            .value();
    allocator.free(opq_alloc);

    // Test `opq_inline_nalign_xrescalloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_unalign_xrescalloc_multi(opq_alloc, 10);
    allocator.free(opq_alloc);

    // Test `opq_inline_unalign_xrescalloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_unalign_xrescalloc_multi_to(allocator, opq_alloc,
                                                         10);
    allocator.free(opq_alloc);

    // Test `opq_inline_align_rescalloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_align_rescalloc_multi(opq_alloc, 8u, 10).value();
    allocator.free(opq_alloc);

    // Test `inline_align_rescalloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator
            .opq_inline_align_rescalloc_multi_to(allocator, opq_alloc, 8u, 10)
            .value();
    allocator.free(opq_alloc);

    // Test `opq_inline_align_xrescalloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_align_xrescalloc_multi(opq_alloc, 8u, 10);
    allocator.free(opq_alloc);

    // Test `opq_inline_align_xrescalloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_align_xrescalloc_multi_to(allocator, opq_alloc, 8u,
                                                       10);
    allocator.free(opq_alloc);

    // Test `opq_inline_unalign_rescalloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_unalign_rescalloc_multi(opq_alloc, 10).value();
    allocator.free(opq_alloc);

    // Test `inline_unalign_rescalloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator
            .opq_inline_unalign_rescalloc_multi_to(allocator, opq_alloc, 10)
            .value();
    allocator.free(opq_alloc);

    // Test `opq_inline_nalign_xrescalloc_multi`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_unalign_xrescalloc_multi(opq_alloc, 10);
    allocator.free(opq_alloc);

    // Test `opq_inline_unalign_xrescalloc_multi_to`.
    opq_alloc = allocator.opq_alloc<int4>().value();
    _ = allocator.opq_inline_unalign_xrescalloc_multi_to(allocator, opq_alloc,
                                                         10);
}
