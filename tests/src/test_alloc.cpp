#include <cat/bit>
#include <cat/linear_allocator>
#include <cat/page_allocator>

#include "../unit_tests.hpp"

struct AllocHugeObject {
    [[maybe_unused]] uint1 storage[cat::inline_buffer_size.raw + 1];
};

int4 alloc_counter = 0;

struct AllocNonTrivial {
    char storage;
    AllocNonTrivial() {
        ++alloc_counter;
    }
};

struct AllocNonTrivialHugeObject {
    [[maybe_unused]] uint1 storage[cat::inline_buffer_size.raw];
    AllocNonTrivialHugeObject() {
        ++alloc_counter;
    }
};

TEST(test_alloc) {
    // Initialize an allocator.
    cat::PageAllocator paging_allocator;
    paging_allocator.reset();
    // Page the kernel for a linear allocator to test with.
    auto page =
        paging_allocator.opq_alloc_multi<cat::Byte>(4_ki - 64).or_exit();
    DEFER(paging_allocator.free(page);)
    auto allocator =
        cat::LinearAllocator::backed_handle(paging_allocator, page);

    // Test `opq_alloc`.
    _ = allocator.opq_alloc<int4>().value();
    auto opq_alloc = allocator.opq_alloc<int4>(1).value();
    cat::verify(allocator.get(opq_alloc) == 1);
    alloc_counter = 0;
    _ = allocator.opq_alloc<AllocNonTrivial>();
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
    _ = allocator.opq_alloc_multi<AllocNonTrivial>(5);
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
    _ = allocator.opq_align_alloc_multi<AllocNonTrivial>(8u, 5);
    cat::verify(alloc_counter == 5);

    // Test `align_xalloc_multi`.
    auto align_xalloc_multi = allocator.opq_align_xalloc_multi<int4>(8u, 5);
    cat::verify(align_xalloc_multi.size() == 5);
    cat::verify(align_xalloc_multi.raw_size() == 20);
    cat::verify(cat::is_aligned(allocator.p_get(align_xalloc_multi), 8u));
    alloc_counter = 0;
    _ = allocator.opq_align_xalloc_multi<AllocNonTrivial>(8u, 5);

    cat::verify(alloc_counter == 5);

    // Test `align_alloc_multi`.
    auto p_align_alloc_multi =
        allocator.align_alloc_multi<int4>(8u, 5).value().data();
    cat::verify(cat::is_aligned(p_align_alloc_multi, 8u));
    alloc_counter = 0;
    _ = allocator.align_alloc_multi<AllocNonTrivial>(8u, 5);
    cat::verify(alloc_counter == 5);

    // Test `align_xalloc_multi`.
    _ = allocator.align_xalloc_multi<int4>(8u, 5);
    alloc_counter = 0;
    _ = allocator.align_xalloc_multi<AllocNonTrivial>(8u, 5);
    cat::verify(alloc_counter == 5);

    // Test `unalign_alloc_multi`.
    auto unalign_alloc_multi =
        allocator.opq_unalign_alloc_multi<int4>(5).value();
    cat::verify(unalign_alloc_multi.size() == 5);
    cat::verify(unalign_alloc_multi.raw_size() == 20);
    alloc_counter = 0;
    _ = allocator.opq_unalign_alloc_multi<AllocNonTrivial>(5);
    cat::verify(alloc_counter == 5);

    // Test `unalign_xalloc_multi`.
    auto unalign_xalloc_multi = allocator.opq_unalign_xalloc_multi<int1>(5);
    cat::verify(unalign_xalloc_multi.size() == 5);
    cat::verify(unalign_xalloc_multi.raw_size() == 5);
    alloc_counter = 0;
    _ = allocator.opq_unalign_xalloc_multi<AllocNonTrivial>(5);
    cat::verify(alloc_counter == 5);

    // Test `unalign_alloc_multi`.
    _ = allocator.unalign_alloc_multi<int1>(5)
            .value();  // `int4` is 4-byte aligned.
    alloc_counter = 0;
    _ = allocator.unalign_alloc_multi<AllocNonTrivial>(5);
    cat::verify(alloc_counter == 5);

    // Test `unalign_xalloc_multi`.
    _ = allocator.unalign_xalloc_multi<int1>(5);  // `int4` is 4-byte aligned.
    alloc_counter = 0;
    _ = allocator.unalign_xalloc_multi<AllocNonTrivial>(5);
    cat::verify(alloc_counter == 5);

    // Test `opq_inline_alloc`.
    _ = allocator.opq_inline_alloc<int4>().value();
    auto inline_alloc = allocator.opq_inline_alloc<int4>(1).value();
    cat::verify(allocator.get(inline_alloc) == 1);
    cat::verify(inline_alloc.is_inline());
    alloc_counter = 0;
    _ = allocator.opq_inline_alloc<AllocNonTrivial>();
    cat::verify(alloc_counter == 1);

    // `AllocHugeObject` is larger than the inline buffer.
    auto inline_alloc_2 = allocator.opq_inline_alloc<AllocHugeObject>().value();
    cat::verify(!inline_alloc_2.is_inline());

    alloc_counter = 0;
    _ = allocator.opq_inline_alloc<AllocNonTrivialHugeObject>();
    cat::verify(alloc_counter == 1);

    // Test `opq_inline_xalloc`.
    _ = allocator.opq_inline_xalloc<int4>();
    auto inline_xalloc = allocator.opq_inline_xalloc<int4>(1);
    cat::verify(allocator.get(inline_xalloc) == 1);

    // Test `inline_alloc_multi`.
    auto inline_alloc_multi = allocator.opq_inline_alloc_multi<int4>(5).value();
    cat::verify(inline_alloc_multi.size() == 5);
    alloc_counter = 0;
    _ = allocator.opq_inline_alloc_multi<AllocNonTrivial>(5);
    cat::verify(alloc_counter == 5);

    // Test `inline_xalloc_multi`.
    auto inline_xalloc_multi = allocator.opq_inline_xalloc_multi<int4>(5);
    cat::verify(inline_xalloc_multi.size() == 5);
    alloc_counter = 0;
    _ = allocator.opq_inline_xalloc_multi<AllocNonTrivial>(5);
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
    ssize nalloc = allocator.opq_nalloc<int4>().value();
    cat::verify(nalloc == ssizeof<int4>());

    // Test `opq_xnalloc`.
    allocator.reset();
    ssize xnalloc = allocator.opq_xnalloc<int4>();
    cat::verify(xnalloc == ssizeof<int4>());

    // Test `nalloc_multi`.
    allocator.reset();
    ssize nalloc_multi = allocator.opq_nalloc_multi<int4>(5).value();
    cat::verify(nalloc_multi == (ssizeof<int4>() * 5));

    // Test `xnalloc_multi`.
    allocator.reset();
    ssize xnalloc_multi = allocator.opq_xnalloc_multi<int4>(5);
    cat::verify(xnalloc_multi == (ssizeof<int4>() * 5));

    // Test `opq_align_nalloc`.
    allocator.reset();
    ssize align_nalloc = allocator.opq_align_nalloc<int4>(4u).value();
    cat::verify(align_nalloc == ssizeof<int4>());

    // Test `opq_align_xnalloc`.
    allocator.reset();
    ssize align_xnalloc = allocator.opq_align_xnalloc<int4>(4u);
    cat::verify(align_xnalloc == ssizeof<int4>());

    // Test `align_nalloc_multi`.
    allocator.reset();
    ssize align_nalloc_multi =
        allocator.opq_align_nalloc_multi<int4>(4u, 5).value();
    cat::verify(align_nalloc_multi == (ssizeof<int4>() * 5));

    // Test `align_xnalloc_multi`.
    allocator.reset();
    ssize align_xnalloc_multi = allocator.opq_align_xnalloc_multi<int4>(4u, 5);
    cat::verify(align_xnalloc_multi == (ssizeof<int4>() * 5));

    // Test `opq_unalign_nalloc`.
    allocator.reset();
    ssize unalign_nalloc = allocator.opq_unalign_nalloc<int4>().value();
    cat::verify(unalign_nalloc == ssizeof<int4>());

    // Test `opq_unalign_xnalloc`.
    allocator.reset();
    ssize unalign_xnalloc = allocator.opq_unalign_xnalloc<int4>();
    cat::verify(unalign_xnalloc == ssizeof<int4>());

    // Test `unalign_nalloc_multi`.
    allocator.reset();
    ssize unalign_nalloc_multi =
        allocator.opq_unalign_nalloc_multi<int4>(5).value();
    cat::verify(unalign_nalloc_multi == (ssizeof<int4>() * 5));

    // Test `unalign_xnalloc_multi`.
    allocator.reset();
    ssize unalign_xnalloc_multi = allocator.opq_unalign_xnalloc_multi<int4>(5);
    cat::verify(unalign_xnalloc_multi == (ssizeof<int4>() * 5));

    // Test `opq_inline_nalloc`.
    allocator.reset();
    ssize inline_nalloc = allocator.opq_inline_nalloc<int4>().value();
    cat::verify(inline_nalloc == cat::inline_buffer_size);
    ssize inline_nalloc_big =
        allocator.opq_inline_nalloc<AllocHugeObject>().value();
    cat::verify(inline_nalloc_big == 257);

    // Test `opq_inline_xnalloc`.
    allocator.reset();
    ssize inline_xnalloc = allocator.opq_inline_xnalloc<int4>();
    cat::verify(inline_xnalloc == cat::inline_buffer_size);
    ssize inline_xnalloc_big = allocator.opq_inline_xnalloc<AllocHugeObject>();
    cat::verify(inline_xnalloc_big == 257);

    // Test `inline_nalloc_multi`.
    allocator.reset();
    ssize inline_nalloc_multi =
        allocator.opq_inline_nalloc_multi<int4>(5).value();
    cat::verify(inline_nalloc_multi == cat::inline_buffer_size);
    ssize inline_nalloc_multi_big =
        allocator.opq_inline_nalloc_multi<AllocHugeObject>(2).value();
    cat::verify(inline_nalloc_multi_big == (257 * 2));

    // Test `inline_xnalloc_multi`.
    allocator.reset();
    ssize inline_xnalloc_multi = allocator.opq_inline_xnalloc_multi<int4>(5);
    cat::verify(inline_xnalloc_multi == cat::inline_buffer_size);
    ssize inline_xnalloc_multi_big =
        allocator.opq_inline_xnalloc_multi<AllocHugeObject>(2);
    cat::verify(inline_xnalloc_multi_big == (257 * 2));

    // Test `opq_inline_align_nalloc`.
    allocator.reset();
    ssize inline_align_nalloc =
        allocator.opq_inline_align_nalloc<int4>(4u).value();
    cat::verify(inline_align_nalloc == cat::inline_buffer_size);
    ssize inline_align_nalloc_big =
        allocator.opq_inline_align_nalloc<AllocHugeObject>(1u).value();
    cat::verify(inline_align_nalloc_big == 257);

    // Test `opq_inline_align_xnalloc`.
    allocator.reset();
    ssize inline_align_xnalloc = allocator.opq_inline_align_xnalloc<int4>(4u);
    cat::verify(inline_align_xnalloc == cat::inline_buffer_size);
    ssize inline_align_xnalloc_big =
        allocator.opq_inline_align_xnalloc<AllocHugeObject>(1u);
    cat::verify(inline_align_xnalloc_big == 257);

    // Test `opq_inline_unalign_nalloc`.
    allocator.reset();
    ssize inline_unalign_nalloc =
        allocator.opq_inline_unalign_nalloc<int4>().value();
    cat::verify(inline_unalign_nalloc == cat::inline_buffer_size);
    ssize inline_unalign_nalloc_big =
        allocator.opq_inline_unalign_nalloc<AllocHugeObject>().value();
    cat::verify(inline_unalign_nalloc_big == 257);

    // Test `opq_inline_unalign_xnalloc`.
    allocator.reset();
    ssize inline_unalign_xnalloc = allocator.opq_inline_unalign_xnalloc<int4>();
    cat::verify(inline_unalign_xnalloc == cat::inline_buffer_size);
    ssize inline_unalign_xnalloc_big =
        allocator.opq_inline_unalign_xnalloc<AllocHugeObject>();
    cat::verify(inline_unalign_xnalloc_big == 257);

    // Test `inline_align_nalloc_multi`.
    allocator.reset();
    ssize inline_align_nalloc_multi =
        allocator.opq_inline_align_nalloc_multi<int4>(4u, 5).value();
    cat::verify(inline_align_nalloc_multi == cat::inline_buffer_size);
    ssize inline_align_nalloc_multi_big =
        allocator.opq_inline_align_nalloc_multi<AllocHugeObject>(1u, 2).value();
    cat::verify(inline_align_nalloc_multi_big == (257 * 2));

    // Test `inline_align_xnalloc_multi`.
    allocator.reset();
    ssize inline_align_xnalloc_multi =
        allocator.opq_inline_align_xnalloc_multi<int4>(4u, 5);
    cat::verify(inline_align_xnalloc_multi == cat::inline_buffer_size);
    ssize inline_align_xnalloc_multi_big =
        allocator.opq_inline_align_xnalloc_multi<AllocHugeObject>(1u, 2);
    cat::verify(inline_align_xnalloc_multi_big == (257 * 2));

    // Test `inline_unalign_nalloc_multi`.
    allocator.reset();
    ssize inline_unalign_nalloc_multi =
        allocator.opq_inline_unalign_nalloc_multi<int4>(5).value();
    cat::verify(inline_unalign_nalloc_multi == cat::inline_buffer_size);
    ssize inline_unalign_nalloc_multi_big =
        allocator.opq_inline_unalign_nalloc_multi<AllocHugeObject>(2).value();
    cat::verify(inline_unalign_nalloc_multi_big == (257 * 2));

    // Test `inline_unalign_xnalloc_multi`.
    allocator.reset();
    ssize inline_unalign_xnalloc_multi =
        allocator.opq_inline_unalign_xnalloc_multi<int4>(5);
    cat::verify(inline_unalign_xnalloc_multi == cat::inline_buffer_size);
    ssize inline_unalign_xnalloc_multi_big =
        allocator.opq_inline_unalign_xnalloc_multi<AllocHugeObject>(2);
    cat::verify(inline_unalign_xnalloc_multi_big == (257 * 2));

    // Test `opq_salloc`.
    _ = allocator.opq_salloc<int4>().value();
    allocator.reset();
    _ = allocator.opq_alloc<cat::Byte>();  // Offset linear allocator by 1 byte.
    auto [salloc, salloc_size] = allocator.opq_salloc<int4>(1).value();
    cat::verify(allocator.get(salloc) == 1);
    cat::verify(salloc_size == 7);
    alloc_counter = 0;
    _ = allocator.opq_salloc<AllocNonTrivial>();
    cat::verify(alloc_counter == 1);

    // Test `opq_xsalloc`.
    _ = allocator.opq_xsalloc<int4>();
    allocator.reset();
    _ = allocator.opq_alloc<cat::Byte>();  // Offset linear allocator by 1 byte.
    auto [xsalloc, xsalloc_size] = allocator.opq_xsalloc<int4>(1);
    cat::verify(allocator.get(xsalloc) == 1);
    cat::verify(xsalloc_size == 7);
    alloc_counter = 0;
    _ = allocator.opq_xsalloc<AllocNonTrivial>();
    cat::verify(alloc_counter == 1);

    // Test `salloc`.
    _ = allocator.salloc<int4>().value();
    allocator.reset();
    _ = allocator.opq_alloc<cat::Byte>();  // Offset linear allocator by 1 byte.
    auto [p_salloc, p_salloc_size] = allocator.salloc<int4>(1).value();
    cat::verify(*p_salloc == 1);
    cat::verify(p_salloc_size == 7);
    alloc_counter = 0;
    _ = allocator.salloc<AllocNonTrivial>();
    cat::verify(alloc_counter == 1);

    // Test `xsalloc`.
    _ = allocator.xsalloc<int4>();
    allocator.reset();
    _ = allocator.opq_alloc<cat::Byte>();  // Offset linear allocator by 1 byte.
    auto [p_xsalloc, p_xsalloc_size] = allocator.xsalloc<int4>(1);
    cat::verify(*p_xsalloc == 1);
    cat::verify(p_xsalloc_size == 7);
    alloc_counter = 0;
    _ = allocator.xsalloc<AllocNonTrivial>();
    cat::verify(alloc_counter == 1);

    // Test `salloc_multi`.
    allocator.reset();
    _ = allocator.opq_alloc<cat::Byte>();  // Offset linear allocator by 1 byte.
    auto [salloc_multi, salloc_multi_size] =
        allocator.opq_salloc_multi<int4>(5).value();
    cat::verify(salloc_multi.size() == 5);
    cat::verify(salloc_multi_size == 23);
    cat::verify(salloc_multi.raw_size() == 20);

    alloc_counter = 0;
    _ = allocator.opq_salloc_multi<AllocNonTrivial>(5);
    cat::verify(alloc_counter == 5);

    // Test `xsalloc_multi`.
    allocator.reset();
    _ = allocator.opq_alloc<cat::Byte>();  // Offset linear allocator by 1 byte.
    auto [xsalloc_multi, xsalloc_multi_size] =
        allocator.opq_xsalloc_multi<int4>(5);
    cat::verify(xsalloc_multi.size() == 5);
    cat::verify(xsalloc_multi_size == 23);
    cat::verify(xsalloc_multi.raw_size() == 20);

    alloc_counter = 0;
    _ = allocator.opq_xsalloc_multi<AllocNonTrivial>(5);
    cat::verify(alloc_counter == 5);

    // Test `salloc_multi`.
    allocator.reset();
    _ = allocator.opq_alloc<cat::Byte>();  // Offset linear allocator by 1 byte.
    auto [p_salloc_multi, p_salloc_multi_size] =
        allocator.salloc_multi<int4>(5).value();
    cat::verify(p_salloc_multi_size == 23);

    alloc_counter = 0;
    _ = allocator.salloc_multi<AllocNonTrivial>(5);
    cat::verify(alloc_counter == 5);

    // Test `xsalloc_multi`.
    allocator.reset();
    _ = allocator.opq_alloc<cat::Byte>();  // Offset linear allocator by 1 byte.
    auto [p_xsalloc_multi, p_xsalloc_multi_size] =
        allocator.xsalloc_multi<int4>(5);
    cat::verify(p_xsalloc_multi_size == 23);

    alloc_counter = 0;
    _ = allocator.xsalloc_multi<AllocNonTrivial>(5);
    cat::verify(alloc_counter == 5);

    // Test `opq_align_salloc`.
    _ = allocator.opq_align_salloc<int4>(8u);
    allocator.reset();
    auto [align_salloc, align_salloc_size] =
        allocator.opq_align_salloc<int4>(8u, 1).value();
    cat::verify(allocator.get(align_salloc) == 1);
    cat::verify(align_salloc_size == 8);
    cat::verify(cat::is_aligned(&allocator.get(align_salloc), 8u));

    alloc_counter = 0;
    _ = allocator.opq_align_salloc<AllocNonTrivial>(8u);
    cat::verify(alloc_counter == 1);

    // Test `opq_align_xsalloc`.
    _ = allocator.opq_align_xsalloc<int4>(8u);
    allocator.reset();
    auto [align_xsalloc, align_xsalloc_size] =
        allocator.opq_align_xsalloc<int4>(8u, 1);
    cat::verify(allocator.get(align_xsalloc) == 1);
    cat::verify(align_xsalloc_size == 8);
    cat::verify(cat::is_aligned(&allocator.get(align_xsalloc), 8u));

    alloc_counter = 0;
    _ = allocator.opq_align_xsalloc<AllocNonTrivial>(8u);
    cat::verify(alloc_counter == 1);

    // Test `align_salloc`.
    _ = allocator.align_salloc<int4>(8u);
    allocator.reset();
    auto [p_align_salloc, p_align_salloc_size] =
        allocator.align_salloc<int4>(8u, 1).value();
    cat::verify(*p_align_salloc == 1);
    cat::verify(p_align_salloc_size == 8);
    cat::verify(cat::is_aligned(p_align_salloc, 8u));

    alloc_counter = 0;
    _ = allocator.align_salloc<AllocNonTrivial>(8u);
    cat::verify(alloc_counter == 1);

    // Test `align_xsalloc`.
    _ = allocator.align_xsalloc<int4>(8u);
    allocator.reset();
    auto [p_align_xsalloc, p_align_xsalloc_size] =
        allocator.align_xsalloc<int4>(8u, 1);
    cat::verify(*p_align_xsalloc == 1);
    cat::verify(p_align_xsalloc_size == 8);
    cat::verify(cat::is_aligned(p_align_xsalloc, 8u));

    alloc_counter = 0;
    _ = allocator.align_xsalloc<AllocNonTrivial>(8u);
    cat::verify(alloc_counter == 1);

    // Test `opq_unalign_salloc`.
    _ = allocator.opq_unalign_salloc<int1>();
    allocator.reset();
    auto [unalign_salloc, unalign_salloc_size] =
        allocator.opq_unalign_salloc<int1>(1).value();
    cat::verify(allocator.get(unalign_salloc) == 1);
    cat::verify(unalign_salloc_size == 1);

    alloc_counter = 0;
    _ = allocator.opq_unalign_salloc<AllocNonTrivial>();
    cat::verify(alloc_counter == 1);
    // Test `opq_unalign_xsalloc`.
    _ = allocator.opq_unalign_xsalloc<int1>();
    allocator.reset();
    auto [unalign_xsalloc, unalign_xsalloc_size] =
        allocator.opq_unalign_xsalloc<int1>(1);
    cat::verify(allocator.get(unalign_xsalloc) == 1);
    cat::verify(unalign_xsalloc_size == 1);

    alloc_counter = 0;
    _ = allocator.opq_unalign_xsalloc<AllocNonTrivial>();
    cat::verify(alloc_counter == 1);

    // Test `unalign_salloc`.
    _ = allocator.unalign_salloc<int1>();
    allocator.reset();
    auto [p_unalign_salloc, p_unalign_salloc_size] =
        allocator.unalign_salloc<int1>(1).value();
    cat::verify(*p_unalign_salloc == 1);
    cat::verify(p_unalign_salloc_size == 1);

    alloc_counter = 0;
    _ = allocator.unalign_salloc<AllocNonTrivial>();
    cat::verify(alloc_counter == 1);

    // Test `unalign_xsalloc`.
    _ = allocator.unalign_xsalloc<int1>();
    allocator.reset();
    auto [p_unalign_xsalloc, p_unalign_xsalloc_size] =
        allocator.unalign_xsalloc<int1>(1);
    cat::verify(*p_unalign_xsalloc == 1);
    cat::verify(p_unalign_xsalloc_size == 1);

    alloc_counter = 0;
    _ = allocator.unalign_xsalloc<AllocNonTrivial>();
    cat::verify(alloc_counter == 1);

    // Test `align_salloc_multi`.
    allocator.reset();
    auto [align_salloc_multi, align_salloc_multi_size] =
        allocator.opq_align_salloc_multi<int4>(8u, 5).value();
    cat::verify(align_salloc_multi_size == 24);
    cat::verify(cat::is_aligned(allocator.p_get(align_salloc_multi), 8u));

    alloc_counter = 0;
    _ = allocator.opq_align_salloc_multi<AllocNonTrivial>(8u, 5);
    cat::verify(alloc_counter == 5);

    // Test `align_xsalloc_multi`.
    allocator.reset();
    auto [align_xsalloc_multi, align_xsalloc_multi_size] =
        allocator.opq_align_xsalloc_multi<int4>(8u, 5);
    cat::verify(align_xsalloc_multi_size == 24);
    cat::verify(cat::is_aligned(allocator.p_get(align_xsalloc_multi), 8u));

    alloc_counter = 0;
    _ = allocator.opq_align_xsalloc_multi<AllocNonTrivial>(8u, 5);
    cat::verify(alloc_counter == 5);

    // Test `align_salloc_multi`.
    allocator.reset();
    auto [p_align_salloc_multi, p_align_salloc_multi_size] =
        allocator.align_salloc_multi<int4>(8u, 5).value();
    cat::verify(p_align_salloc_multi_size == 24);
    cat::verify(cat::is_aligned(p_align_salloc_multi.data(), 8u));

    alloc_counter = 0;
    _ = allocator.align_salloc_multi<AllocNonTrivial>(8u, 5);
    cat::verify(alloc_counter == 5);

    // Test `align_xsalloc_multi`.
    allocator.reset();
    auto [p_align_xsalloc_multi, p_align_xsalloc_multi_size] =
        allocator.align_xsalloc_multi<int4>(8u, 5);
    cat::verify(p_align_xsalloc_multi_size == 24);
    cat::verify(cat::is_aligned(p_align_xsalloc_multi.data(), 8u));

    alloc_counter = 0;
    _ = allocator.align_xsalloc_multi<AllocNonTrivial>(8u, 5);
    cat::verify(alloc_counter == 5);

    // Test `unalign_salloc_multi`.
    allocator.reset();
    auto [unalign_salloc_multi, unalign_salloc_multi_size] =
        allocator.opq_unalign_salloc_multi<int1>(5).value();
    cat::verify(unalign_salloc_multi_size == 5);

    alloc_counter = 0;
    _ = allocator.opq_unalign_salloc_multi<AllocNonTrivial>(5);
    cat::verify(alloc_counter == 5);

    // Test `unalign_xsalloc_multi`.
    allocator.reset();
    auto [unalign_xsalloc_multi, unalign_xsalloc_multi_size] =
        allocator.opq_unalign_xsalloc_multi<int1>(5);
    cat::verify(unalign_xsalloc_multi_size == 5);

    alloc_counter = 0;
    _ = allocator.opq_unalign_xsalloc_multi<AllocNonTrivial>(5);
    cat::verify(alloc_counter == 5);

    // Test `unalign_salloc_multi`.
    allocator.reset();
    auto [p_unalign_salloc_multi, p_unalign_salloc_multi_size] =
        allocator.unalign_salloc_multi<int1>(5).value();
    cat::verify(p_unalign_salloc_multi_size == 5);

    alloc_counter = 0;
    _ = allocator.unalign_salloc_multi<AllocNonTrivial>(5);
    cat::verify(alloc_counter == 5);

    // Test `unalign_xsalloc_multi`.
    allocator.reset();
    auto [p_unalign_xsalloc_multi, p_unalign_xsalloc_multi_size] =
        allocator.unalign_xsalloc_multi<int1>(5);
    cat::verify(p_unalign_xsalloc_multi_size == 5);

    alloc_counter = 0;
    _ = allocator.unalign_xsalloc_multi<AllocNonTrivial>(5);
    cat::verify(alloc_counter == 5);

    // Test `opq_inline_salloc`.
    allocator.reset();
    auto [inline_salloc, inline_salloc_size] =
        allocator.opq_inline_salloc<int4>(1).value();
    cat::verify(allocator.get(inline_salloc) == 1);
    cat::verify(inline_salloc_size == cat::inline_buffer_size);
    cat::verify(inline_salloc.is_inline());

    auto [inline_salloc_big, inline_salloc_size_big] =
        allocator.opq_inline_salloc<AllocHugeObject>().value();
    cat::verify(inline_salloc_size_big == ssizeof<AllocHugeObject>());
    cat::verify(!inline_salloc_big.is_inline());

    // Test `opq_inline_xsalloc`.
    allocator.reset();
    auto [inline_xsalloc, inline_xsalloc_size] =
        allocator.opq_inline_xsalloc<int4>(1);
    cat::verify(inline_xsalloc_size == cat::inline_buffer_size);
    cat::verify(inline_xsalloc.is_inline());

    auto [inline_xsalloc_big, inline_xsalloc_size_big] =
        allocator.opq_inline_xsalloc<AllocHugeObject>();
    cat::verify(inline_xsalloc_size_big == ssizeof<AllocHugeObject>());
    cat::verify(!inline_xsalloc_big.is_inline());

    // Test `inline_salloc_multi`.
    allocator.reset();
    auto [inline_salloc_multi, inline_salloc_multi_size] =
        allocator.opq_inline_salloc_multi<int4>(5).value();
    cat::verify(inline_salloc_multi_size == cat::inline_buffer_size);
    cat::verify(inline_salloc_multi.is_inline());

    auto [inline_salloc_multi_big, inline_salloc_multi_size_big] =
        allocator.opq_inline_salloc_multi<AllocHugeObject>(5).value();
    cat::verify(inline_salloc_multi_size_big == ssizeof<AllocHugeObject>() * 5);
    cat::verify(!inline_salloc_multi_big.is_inline());

    // Test `inline_xsalloc_multi`.
    allocator.reset();
    auto [inline_xsalloc_multi, inline_xsalloc_multi_size] =
        allocator.opq_inline_xsalloc_multi<int4>(5);
    cat::verify(inline_xsalloc_multi_size == cat::inline_buffer_size);
    cat::verify(inline_xsalloc_multi.is_inline());

    auto [inline_xsalloc_multi_big, inline_xsalloc_multi_size_big] =
        allocator.opq_inline_xsalloc_multi<AllocHugeObject>(5);
    cat::verify(inline_xsalloc_multi_size_big ==
                ssizeof<AllocHugeObject>() * 5);
    cat::verify(!inline_xsalloc_multi_big.is_inline());

    // Test `opq_inline_align_salloc`.
    allocator.reset();
    auto [inline_align_salloc, inline_align_salloc_size] =
        allocator.opq_inline_align_salloc<int4>(8u, 1).value();
    cat::verify(allocator.get(inline_align_salloc) == 1);
    cat::verify(inline_align_salloc_size >= cat::inline_buffer_size);
    cat::verify(inline_align_salloc.is_inline());

    auto [inline_align_salloc_big, inline_align_salloc_size_big] =
        allocator.opq_inline_align_salloc<AllocHugeObject>(8u).value();
    cat::verify(inline_align_salloc_size_big >= ssizeof<AllocHugeObject>());
    cat::verify(!inline_align_salloc_big.is_inline());

    // Test `opq_inline_align_xsalloc`.
    allocator.reset();
    auto [inline_align_xsalloc, inline_align_xsalloc_size] =
        allocator.opq_inline_align_xsalloc<int4>(8u, 1);
    cat::verify(allocator.get(inline_align_xsalloc) == 1);
    cat::verify(inline_align_xsalloc_size >= cat::inline_buffer_size);
    cat::verify(inline_align_xsalloc.is_inline());

    auto [inline_align_xsalloc_big, inline_align_xsalloc_size_big] =
        allocator.opq_inline_align_xsalloc<AllocHugeObject>(8u);
    cat::verify(inline_align_xsalloc_size_big >= ssizeof<AllocHugeObject>());
    cat::verify(!inline_align_xsalloc_big.is_inline());

    // Test `opq_inline_unalign_salloc`.
    allocator.reset();
    auto [inline_unalign_salloc, inline_unalign_salloc_size] =
        allocator.opq_inline_unalign_salloc<int4>(1).value();
    cat::verify(allocator.get(inline_unalign_salloc) == 1);
    cat::verify(inline_unalign_salloc_size == cat::inline_buffer_size);
    cat::verify(inline_unalign_salloc.is_inline());

    auto [inline_unalign_salloc_big, inline_unalign_salloc_size_big] =
        allocator.opq_inline_unalign_salloc<AllocHugeObject>().value();
    cat::verify(inline_unalign_salloc_size_big == ssizeof<AllocHugeObject>());
    cat::verify(!inline_unalign_salloc_big.is_inline());

    // Test `opq_inline_unalign_xsalloc`.
    allocator.reset();
    auto [inline_unalign_xsalloc, inline_unalign_xsalloc_size] =
        allocator.opq_inline_unalign_xsalloc<int4>(1);
    cat::verify(allocator.get(inline_unalign_xsalloc) == 1);
    cat::verify(inline_unalign_xsalloc_size == cat::inline_buffer_size);
    cat::verify(inline_unalign_xsalloc.is_inline());

    auto [inline_unalign_xsalloc_big, inline_unalign_xsalloc_size_big] =
        allocator.opq_inline_unalign_xsalloc<AllocHugeObject>();
    cat::verify(inline_unalign_xsalloc_size_big == ssizeof<AllocHugeObject>());
    cat::verify(!inline_unalign_xsalloc_big.is_inline());

    // Test `inline_align_salloc_multi`.
    allocator.reset();
    auto [inline_align_salloc_multi, inline_align_salloc_multi_size] =
        allocator.opq_inline_align_salloc_multi<int4>(8u, 5).value();
    cat::verify(inline_align_salloc_multi_size >= cat::inline_buffer_size);
    cat::verify(inline_align_salloc_multi.is_inline());

    auto [inline_align_salloc_multi_big, inline_align_salloc_multi_size_big] =
        allocator.opq_inline_align_salloc_multi<AllocHugeObject>(8u, 5).value();
    cat::verify(inline_align_salloc_multi_size_big >=
                ssizeof<AllocHugeObject>());
    cat::verify(!inline_align_salloc_multi_big.is_inline());

    // Test `inline_align_xsalloc_multi`.
    allocator.reset();
    auto [inline_align_xsalloc_multi, inline_align_xsalloc_multi_size] =
        allocator.opq_inline_align_xsalloc_multi<int4>(8u, 5);
    cat::verify(inline_align_xsalloc_multi_size >= cat::inline_buffer_size);
    cat::verify(inline_align_xsalloc_multi.is_inline());

    auto [inline_align_xsalloc_multi_big, inline_align_xsalloc_multi_size_big] =
        allocator.opq_inline_align_xsalloc_multi<AllocHugeObject>(8u, 5);
    cat::verify(inline_align_xsalloc_multi_size_big >=
                ssizeof<AllocHugeObject>());
    cat::verify(!inline_align_xsalloc_multi_big.is_inline());

    // Test `inline_unalign_salloc_multi`.
    allocator.reset();
    auto [inline_unalign_salloc_multi, inline_unalign_salloc_multi_size] =
        allocator.opq_inline_unalign_salloc_multi<int4>(5).value();
    cat::verify(inline_unalign_salloc_multi_size == cat::inline_buffer_size);
    cat::verify(inline_unalign_salloc_multi.is_inline());

    auto [inline_unalign_salloc_multi_big,
          inline_unalign_salloc_multi_size_big] =
        allocator.opq_inline_unalign_salloc_multi<AllocHugeObject>(5).value();
    cat::verify(inline_unalign_salloc_multi_size_big ==
                ssizeof<AllocHugeObject>() * 5);
    cat::verify(!inline_unalign_salloc_multi_big.is_inline());

    // Test `inline_unalign_xsalloc_multi`.
    allocator.reset();
    auto [inline_unalign_xsalloc_multi, inline_unalign_xsalloc_multi_size] =
        allocator.opq_inline_unalign_xsalloc_multi<int4>(5);
    cat::verify(inline_unalign_xsalloc_multi_size == cat::inline_buffer_size);
    cat::verify(inline_unalign_xsalloc_multi.is_inline());

    auto [inline_unalign_xsalloc_multi_big,
          inline_unalign_xsalloc_multi_size_big] =
        allocator.opq_inline_unalign_xsalloc_multi<AllocHugeObject>(5);
    cat::verify(inline_unalign_xsalloc_multi_size_big ==
                ssizeof<AllocHugeObject>() * 5);
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

    // TODO: Test `opq_realloc` family more comprehensively.
    allocator.reset();

    // Test `opq_realloc`.
    auto realloc_1 = allocator.opq_alloc<int4>(1).value();
    auto realloc_2 = allocator.opq_alloc<int4>(2).value();
    cat::verify(allocator.get(realloc_1) == 1);
    cat::verify(allocator.get(realloc_2) == 2);
    realloc_1 = allocator.opq_realloc(realloc_2).value();
    cat::verify(allocator.get(realloc_1) == 2);

    // Test `realloc_to`.
    _ = allocator.opq_realloc_to(allocator, opq_alloc).value();

    // Test `realloc`.
    auto p_realloc_1 = allocator.alloc<int4>(1).value();
    auto p_realloc_2 = allocator.alloc<int4>(2).value();
    cat::verify(*p_realloc_1 == 1);
    cat::verify(*p_realloc_2 == 2);
    p_realloc_1 = allocator.realloc(p_realloc_2).value();
    cat::verify(*p_realloc_1 == 2);

    // Test `realloc_to`
    _ = allocator.realloc_to(allocator, p_alloc);

    // Test `opq_xrealloc`.
    _ = allocator.opq_xrealloc(opq_alloc);

    // Test `xrealloc_to`.
    _ = allocator.opq_xrealloc_to(allocator, opq_alloc);

    // Test `xrealloc`
    _ = allocator.xrealloc(p_alloc);

    // Test `xrealloc_to`
    _ = allocator.xrealloc_to(allocator, p_alloc);

    // Test `opq_align_realloc`.
    _ = allocator.opq_align_realloc(opq_alloc, 8u).value();

    // Test `align_realloc_to`.
    _ = allocator.opq_align_realloc_to(allocator, opq_alloc, 8u).value();

    // Test `opq_align_xrealloc`.
    _ = allocator.opq_align_xrealloc(opq_alloc, 8u);

    // Test `align_xrealloc_to`.
    _ = allocator.opq_align_xrealloc_to(allocator, opq_alloc, 8u);

    // Test `opq_unalign_realloc`.
    _ = allocator.opq_unalign_realloc(opq_alloc).value();

    // Test `unalign_realloc_to`.
    _ = allocator.opq_unalign_realloc_to(allocator, opq_alloc).value();

    // Test `opq_unalign_xrealloc`.
    _ = allocator.opq_unalign_xrealloc(opq_alloc);

    // Test `unalign_xrealloc_to`.
    _ = allocator.opq_unalign_xrealloc_to(allocator, opq_alloc);

    // Test `opq_align_realloc`.
    _ = allocator.opq_align_realloc(opq_alloc, 8u).value();

    // Test `align_realloc_to`.
    _ = allocator.opq_align_realloc_to(allocator, opq_alloc, 8u).value();

    // Test `opq_align_xrealloc`.
    _ = allocator.opq_align_xrealloc(opq_alloc, 8u);

    // Test `align_xrealloc_to`.
    _ = allocator.opq_align_xrealloc_to(allocator, opq_alloc, 8u);

    // Test `opq_unalign_realloc`.
    _ = allocator.opq_unalign_realloc(opq_alloc).value();

    // Test `unalign_realloc_to`.
    _ = allocator.opq_unalign_realloc_to(allocator, opq_alloc).value();

    // Test `opq_unalign_xrealloc`.
    _ = allocator.opq_unalign_xrealloc(opq_alloc);

    // Test `unalign_xrealloc_to`.
    _ = allocator.opq_unalign_xrealloc_to(allocator, opq_alloc);

    // Test `align_realloc`.
    _ = allocator.align_realloc(p_alloc, 8u).value();

    // Test `align_realloc_to`.
    _ = allocator.align_realloc_to(allocator, p_alloc, 8u).value();

    // Test `align_xrealloc`.
    _ = allocator.align_xrealloc(p_alloc, 8u);

    // Test `align_xrealloc_to`.
    _ = allocator.align_xrealloc_to(allocator, p_alloc, 8u);

    // Test `unalign_realloc`.
    _ = allocator.unalign_realloc(p_alloc).value();

    // Test `unalign_realloc_to`.
    _ = allocator.unalign_realloc_to(allocator, p_alloc).value();

    // Test `unalign_xrealloc`.
    _ = allocator.unalign_xrealloc(p_alloc);

    // Test `unalign_xrealloc_to`.
    _ = allocator.unalign_xrealloc_to(allocator, p_alloc);

    // Test `realloc_multi`.
    _ = allocator.opq_realloc_multi(opq_alloc, 10).value();

    // Test `realloc_multi_to`.
    _ = allocator.opq_realloc_multi_to(allocator, opq_alloc, 10).value();

    // Test `realloc_multi`.
    _ = allocator.realloc_multi(p_alloc, 5, 10).value().data();

    // Test `realloc_multi_to`
    _ = allocator.realloc_multi_to(allocator, p_alloc, 5, 10).value().data();

    // Test `xrealloc_multi`.
    _ = allocator.opq_xrealloc_multi(opq_alloc, 10);

    // Test `xrealloc_multi_to`.
    _ = allocator.opq_xrealloc_multi_to(allocator, opq_alloc, 10);

    // Test `xrealloc_multi`
    _ = allocator.xrealloc_multi(p_alloc, 5, 10);

    // Test `xrealloc_multi_to`
    _ = allocator.xrealloc_multi_to(allocator, p_alloc, 5, 10);

    // Test `align_realloc_multi`.
    _ = allocator.opq_align_realloc_multi(opq_alloc, 8u, 10).value();

    // Test `align_realloc_multi_to`.
    _ = allocator.opq_align_realloc_multi_to(allocator, opq_alloc, 8u, 10)
            .value();

    // Test `align_xrealloc_multi`.
    _ = allocator.opq_align_xrealloc_multi(opq_alloc, 8u, 10);

    // Test `align_xrealloc_multi_to`.
    _ = allocator.opq_align_xrealloc_multi_to(allocator, opq_alloc, 8u, 10);

    // Test `unalign_realloc_multi`.
    _ = allocator.opq_unalign_realloc_multi(opq_alloc, 10).value();

    // Test `unalign_realloc_multi_to`.
    _ = allocator.opq_unalign_realloc_multi_to(allocator, opq_alloc, 10)
            .value();

    // Test `unalign_xrealloc_multi`.
    _ = allocator.opq_unalign_xrealloc_multi(opq_alloc, 10);

    // Test `unalign_xrealloc_multi_to`.
    _ = allocator.opq_unalign_xrealloc_multi_to(allocator, opq_alloc, 10);

    // Test `align_realloc_multi`.
    _ = allocator.opq_align_realloc_multi(opq_alloc, 8u, 10).value();

    // Test `align_realloc_multi_to`.
    _ = allocator.opq_align_realloc_multi_to(allocator, opq_alloc, 8u, 10)
            .value();

    // Test `align_xrealloc_multi`.
    _ = allocator.opq_align_xrealloc_multi(opq_alloc, 8u, 10);

    // Test `align_xrealloc_multi_to`.
    _ = allocator.opq_align_xrealloc_multi_to(allocator, opq_alloc, 8u, 10);

    // Test `unalign_realloc_multi`.
    _ = allocator.opq_unalign_realloc_multi(opq_alloc, 10).value();

    // Test `unalign_realloc_multi_to`.
    _ = allocator.opq_unalign_realloc_multi_to(allocator, opq_alloc, 10)
            .value();

    // Test `unalign_xrealloc_multi`.
    _ = allocator.opq_unalign_xrealloc_multi(opq_alloc, 10);

    // Test `unalign_xrealloc_multi_to`.
    _ = allocator.opq_unalign_xrealloc_multi_to(allocator, opq_alloc, 10);

    // Test `align_realloc_multi`.
    _ = allocator.align_realloc_multi(p_alloc, 8u, 5, 10).value().data();

    // Test `align_realloc_multi_to`.
    _ = allocator.align_realloc_multi_to(allocator, p_alloc, 8u, 5, 10)
            .value()
            .data();

    // Test `align_xrealloc_multi`.
    _ = allocator.align_xrealloc_multi(p_alloc, 8u, 5, 10);

    // Test `align_xrealloc_multi_to`.
    _ = allocator.align_xrealloc_multi_to(allocator, p_alloc, 8u, 5, 10);

    // Test `unalign_realloc_multi`.
    _ = allocator.unalign_realloc_multi(p_alloc, 5, 10).value().data();

    // Test `unalign_realloc_multi_to`.
    _ = allocator.unalign_realloc_multi_to(allocator, p_alloc, 5, 10)
            .value()
            .data();

    // Test `unalign_xrealloc_multi`.
    _ = allocator.unalign_xrealloc_multi(p_alloc, 5, 10);

    // Test `unalign_xrealloc_multi_to`.
    _ = allocator.unalign_xrealloc_multi_to(allocator, p_alloc, 5, 10);

    // The allocator runs out of memory around here.
    allocator.reset();

    // Test `opq_recalloc`.
    _ = allocator.opq_recalloc(opq_alloc).value();

    // Test `recalloc_to`.
    _ = allocator.opq_recalloc_to(allocator, opq_alloc).value();

    // Test `recalloc`.
    _ = allocator.recalloc(p_alloc).value();

    // Test `recalloc_to`
    _ = allocator.recalloc_to(allocator, p_alloc);

    // Test `opq_xrecalloc`.
    _ = allocator.opq_xrecalloc(opq_alloc);

    // Test `xrecalloc_to`.
    _ = allocator.opq_xrecalloc_to(allocator, opq_alloc);

    // Test `xrecalloc`
    _ = allocator.xrecalloc(p_alloc);

    // Test `xrecalloc_to`
    _ = allocator.xrecalloc_to(allocator, p_alloc);

    // Test `opq_align_recalloc`.
    _ = allocator.opq_align_recalloc(opq_alloc, 8u).value();

    // Test `align_recalloc_to`.
    _ = allocator.opq_align_recalloc_to(allocator, opq_alloc, 8u).value();

    // Test `opq_align_xrecalloc`.
    _ = allocator.opq_align_xrecalloc(opq_alloc, 8u);

    // Test `align_xrecalloc_to`.
    _ = allocator.opq_align_xrecalloc_to(allocator, opq_alloc, 8u);

    // Test `opq_unalign_recalloc`.
    _ = allocator.opq_unalign_recalloc(opq_alloc).value();

    // Test `unalign_recalloc_to`.
    _ = allocator.opq_unalign_recalloc_to(allocator, opq_alloc).value();

    // Test `opq_unalign_xrecalloc`.
    _ = allocator.opq_unalign_xrecalloc(opq_alloc);

    // Test `unalign_xrecalloc_to`.
    _ = allocator.opq_unalign_xrecalloc_to(allocator, opq_alloc);

    // Test `opq_align_recalloc`.
    _ = allocator.opq_align_recalloc(opq_alloc, 8u).value();

    // Test `align_recalloc_to`.
    _ = allocator.opq_align_recalloc_to(allocator, opq_alloc, 8u).value();

    // Test `opq_align_xrecalloc`.
    _ = allocator.opq_align_xrecalloc(opq_alloc, 8u);

    // Test `align_xrecalloc_to`.
    _ = allocator.opq_align_xrecalloc_to(allocator, opq_alloc, 8u);

    // Test `opq_unalign_recalloc`.
    _ = allocator.opq_unalign_recalloc(opq_alloc).value();

    // Test `unalign_recalloc_to`.
    _ = allocator.opq_unalign_recalloc_to(allocator, opq_alloc).value();

    // Test `opq_unalign_xrecalloc`.
    _ = allocator.opq_unalign_xrecalloc(opq_alloc);

    // Test `unalign_xrecalloc_to`.
    _ = allocator.opq_unalign_xrecalloc_to(allocator, opq_alloc);

    // Test `align_recalloc`.
    _ = allocator.align_recalloc(p_alloc, 8u).value();

    // Test `align_recalloc_to`.
    _ = allocator.align_recalloc_to(allocator, p_alloc, 8u).value();

    // Test `align_xrecalloc`.
    _ = allocator.align_xrecalloc(p_alloc, 8u);

    // Test `align_xrecalloc_to`.
    _ = allocator.align_xrecalloc_to(allocator, p_alloc, 8u);

    // Test `unalign_recalloc`.
    _ = allocator.unalign_recalloc(p_alloc).value();

    // Test `unalign_recalloc_to`.
    _ = allocator.unalign_recalloc_to(allocator, p_alloc).value();

    // Test `unalign_xrecalloc`.
    _ = allocator.unalign_xrecalloc(p_alloc);

    // Test `unalign_xrecalloc_to`.
    _ = allocator.unalign_xrecalloc_to(allocator, p_alloc);

    // Test `recalloc_multi`.
    _ = allocator.opq_recalloc_multi(opq_alloc, 10).value();

    // Test `recalloc_multi_to`.
    _ = allocator.opq_recalloc_multi_to(allocator, opq_alloc, 10).value();

    // Test `recalloc_multi`.
    _ = allocator.recalloc_multi(p_alloc, 5, 10).value().data();

    // Test `recalloc_multi_to`
    _ = allocator.recalloc_multi_to(allocator, p_alloc, 5, 10).value().data();

    // Test `xrecalloc_multi`.
    _ = allocator.opq_xrecalloc_multi(opq_alloc, 10);

    // Test `xrecalloc_multi_to`.
    _ = allocator.opq_xrecalloc_multi_to(allocator, opq_alloc, 10);

    // Test `xrecalloc_multi`
    _ = allocator.xrecalloc_multi(p_alloc, 5, 10);

    // Test `xrecalloc_multi_to`
    _ = allocator.xrecalloc_multi_to(allocator, p_alloc, 5, 10);

    // Test `align_recalloc_multi`.
    _ = allocator.opq_align_recalloc_multi(opq_alloc, 8u, 10).value();

    // Test `align_recalloc_multi_to`.
    _ = allocator.opq_align_recalloc_multi_to(allocator, opq_alloc, 8u, 10)
            .value();

    // Test `align_xrecalloc_multi`.
    _ = allocator.opq_align_xrecalloc_multi(opq_alloc, 8u, 10);

    // Test `align_xrecalloc_multi_to`.
    _ = allocator.opq_align_xrecalloc_multi_to(allocator, opq_alloc, 8u, 10);

    // Test `unalign_recalloc_multi`.
    _ = allocator.opq_unalign_recalloc_multi(opq_alloc, 10).value();

    // Test `unalign_recalloc_multi_to`.
    _ = allocator.opq_unalign_recalloc_multi_to(allocator, opq_alloc, 10)
            .value();

    // Test `unalign_xrecalloc_multi`.
    _ = allocator.opq_unalign_xrecalloc_multi(opq_alloc, 10);

    // Test `unalign_xrecalloc_multi_to`.
    _ = allocator.opq_unalign_xrecalloc_multi_to(allocator, opq_alloc, 10);

    // Test `align_recalloc_multi`.
    _ = allocator.opq_align_recalloc_multi(opq_alloc, 8u, 10).value();

    // Test `align_recalloc_multi_to`.
    _ = allocator.opq_align_recalloc_multi_to(allocator, opq_alloc, 8u, 10)
            .value();

    // Test `align_xrecalloc_multi`.
    _ = allocator.opq_align_xrecalloc_multi(opq_alloc, 8u, 10);

    // Test `align_xrecalloc_multi_to`.
    _ = allocator.opq_align_xrecalloc_multi_to(allocator, opq_alloc, 8u, 10);

    // Test `unalign_recalloc_multi`.
    _ = allocator.opq_unalign_recalloc_multi(opq_alloc, 10).value();

    // Test `unalign_recalloc_multi_to`.
    _ = allocator.opq_unalign_recalloc_multi_to(allocator, opq_alloc, 10)
            .value();

    // Test `unalign_xrecalloc_multi`.
    _ = allocator.opq_unalign_xrecalloc_multi(opq_alloc, 10);

    // Test `unalign_xrecalloc_multi_to`.
    _ = allocator.opq_unalign_xrecalloc_multi_to(allocator, opq_alloc, 10);

    // Test `align_recalloc_multi`.
    _ = allocator.align_recalloc_multi(p_alloc, 8u, 5, 10).value().data();

    // Test `align_recalloc_multi_to`.
    _ = allocator.align_recalloc_multi_to(allocator, p_alloc, 8u, 5, 10)
            .value();

    // Test `align_xrecalloc_multi`.
    _ = allocator.align_xrecalloc_multi(p_alloc, 8u, 5, 10);

    // Test `align_xrecalloc_multi_to`.
    _ = allocator.align_xrecalloc_multi_to(allocator, p_alloc, 8u, 5, 10);

    // Test `unalign_recalloc_multi`.
    _ = allocator.unalign_recalloc_multi(p_alloc, 5, 10).value().data();

    // Test `unalign_recalloc_multi_to`.
    _ = allocator.unalign_recalloc_multi_to(allocator, p_alloc, 5, 10)
            .value()
            .data();

    // Test `unalign_xrecalloc_multi`.
    _ = allocator.unalign_xrecalloc_multi(p_alloc, 5, 10);

    // Test `unalign_xrecalloc_multi_to`.
    _ = allocator.unalign_xrecalloc_multi_to(allocator, p_alloc, 5, 10);

    // Test `opq_inline_realloc`.
    _ = allocator.opq_inline_realloc(opq_alloc);

    // Test `inline_realloc_to`.
    _ = allocator.opq_inline_realloc_to(allocator, opq_alloc).value();

    // Test `opq_inline_xrealloc`.
    _ = allocator.opq_inline_xrealloc(opq_alloc);

    // Test `inline_xrealloc_to`.
    _ = allocator.opq_inline_xrealloc_to(allocator, opq_alloc);

    // Test `opq_inline_align_realloc`.
    _ = allocator.opq_inline_align_realloc(opq_alloc, 8u).value();

    // Test `inline_align_realloc_to`.
    _ = allocator.opq_inline_align_realloc_to(allocator, opq_alloc, 8u).value();

    // Test `opq_inline_align_xrealloc`.
    _ = allocator.opq_inline_align_xrealloc(opq_alloc, 8u);

    // Test `inline_align_xrealloc_to`.
    _ = allocator.opq_inline_align_xrealloc_to(allocator, opq_alloc, 8u);

    // Test `opq_inline_unalign_realloc`.
    _ = allocator.opq_inline_unalign_realloc(opq_alloc).value();

    // Test `inline_unalign_realloc_to`.
    _ = allocator.opq_inline_unalign_realloc_to(allocator, opq_alloc).value();

    // Test `opq_inline_unalign_xrealloc`.
    _ = allocator.opq_inline_unalign_xrealloc(opq_alloc);

    // Test `inline_unalign_xrealloc_to`.
    _ = allocator.opq_inline_unalign_xrealloc_to(allocator, opq_alloc);

    // Test `opq_inline_align_realloc`.
    _ = allocator.opq_inline_align_realloc(opq_alloc, 8u).value();

    // Test `inline_align_realloc_to`.
    _ = allocator.opq_inline_align_realloc_to(allocator, opq_alloc, 8u).value();

    // Test `opq_inline_align_xrealloc`.
    _ = allocator.opq_inline_align_xrealloc(opq_alloc, 8u);

    // Test `inline_align_xrealloc_to`.
    _ = allocator.opq_inline_align_xrealloc_to(allocator, opq_alloc, 8u);

    // Test `opq_inline_unalign_realloc`.
    _ = allocator.opq_inline_unalign_realloc(opq_alloc).value();

    // Test `inline_unalign_realloc_to`.
    _ = allocator.opq_inline_unalign_realloc_to(allocator, opq_alloc).value();

    // Test `opq_inline_unalign_xrealloc`.
    _ = allocator.opq_inline_unalign_xrealloc(opq_alloc);

    // Test `inline_unalign_xrealloc_to`.
    _ = allocator.opq_inline_unalign_xrealloc_to(allocator, opq_alloc);

    // Test `inline_realloc_multi`.
    _ = allocator.opq_inline_realloc_multi(opq_alloc, 10).value();

    // Test `inline_realloc_multi_to`.
    _ = allocator.opq_inline_realloc_multi_to(allocator, opq_alloc, 10).value();

    // Test `inline_xrealloc_multi`.
    _ = allocator.opq_inline_xrealloc_multi(opq_alloc, 10);

    // Test `inline_xrealloc_multi_to`.
    _ = allocator.opq_inline_xrealloc_multi_to(allocator, opq_alloc, 10);

    // Test `inline_align_realloc_multi`.
    _ = allocator.opq_inline_align_realloc_multi(opq_alloc, 8u, 10).value();

    // Test `inline_align_realloc_multi_to`.
    _ = allocator
            .opq_inline_align_realloc_multi_to(allocator, opq_alloc, 8u, 10)
            .value();

    // Test `inline_align_xrealloc_multi`.
    _ = allocator.opq_inline_align_xrealloc_multi(opq_alloc, 8u, 10);

    // Test `inline_align_xrealloc_multi_to`.
    _ = allocator.opq_inline_align_xrealloc_multi_to(allocator, opq_alloc, 8u,
                                                     10);

    // Test `inline_unalign_realloc_multi`.
    _ = allocator.opq_inline_unalign_realloc_multi(opq_alloc, 10).value();

    // Test `inline_unalign_realloc_multi_to`.
    _ = allocator.opq_inline_unalign_realloc_multi_to(allocator, opq_alloc, 10)
            .value();

    // Test `inline_unalign_xrealloc_multi`.
    _ = allocator.opq_inline_unalign_xrealloc_multi(opq_alloc, 10);

    // Test `inline_unalign_xrealloc_multi_to`.
    _ = allocator.opq_inline_unalign_xrealloc_multi_to(allocator, opq_alloc,
                                                       10);

    // Test `inline_align_realloc_multi`.
    _ = allocator.opq_inline_align_realloc_multi(opq_alloc, 8u, 10).value();

    // Test `inline_align_realloc_multi_to`.
    _ = allocator
            .opq_inline_align_realloc_multi_to(allocator, opq_alloc, 8u, 10)
            .value();

    // Test `inline_align_xrealloc_multi`.
    _ = allocator.opq_inline_align_xrealloc_multi(opq_alloc, 8u, 10);

    // Test `inline_align_xrealloc_multi_to`.
    _ = allocator.opq_inline_align_xrealloc_multi_to(allocator, opq_alloc, 8u,
                                                     10);

    // Test `inline_unalign_realloc_multi`.
    _ = allocator.opq_inline_unalign_realloc_multi(opq_alloc, 10).value();

    // Test `inline_unalign_realloc_multi_to`.
    _ = allocator.opq_inline_unalign_realloc_multi_to(allocator, opq_alloc, 10)
            .value();

    // Test `inline_unalign_xrealloc_multi`.
    _ = allocator.opq_inline_unalign_xrealloc_multi(opq_alloc, 10);

    // Test `inline_unalign_xrealloc_multi_to`.
    _ = allocator.opq_inline_unalign_xrealloc_multi_to(allocator, opq_alloc,
                                                       10);

    // The allocator runs out of memory around here.
    allocator.reset();

    // Test `opq_inline_recalloc`.
    _ = allocator.opq_inline_recalloc(opq_alloc).value();

    // Test `inline_recalloc_to`.
    _ = allocator.opq_inline_recalloc_to(allocator, opq_alloc).value();

    // Test `opq_inline_xrecalloc`.
    _ = allocator.opq_inline_xrecalloc(opq_alloc);

    // Test `inline_xrecalloc_to`.
    _ = allocator.opq_inline_xrecalloc_to(allocator, opq_alloc);

    // Test `opq_inline_align_recalloc`.
    _ = allocator.opq_inline_align_recalloc(opq_alloc, 8u).value();

    // Test `inline_align_recalloc_to`.
    _ = allocator.opq_inline_align_recalloc_to(allocator, opq_alloc, 8u)
            .value();

    // Test `opq_inline_align_xrecalloc`.
    _ = allocator.opq_inline_align_xrecalloc(opq_alloc, 8u);

    // Test `inline_align_xrecalloc_to`.
    _ = allocator.opq_inline_align_xrecalloc_to(allocator, opq_alloc, 8u);

    // Test `opq_inline_unalign_recalloc`.
    _ = allocator.opq_inline_unalign_recalloc(opq_alloc).value();

    // Test `inline_unalign_recalloc_to`.
    _ = allocator.opq_inline_unalign_recalloc_to(allocator, opq_alloc).value();

    // Test `opq_inline_unalign_xrecalloc`.
    _ = allocator.opq_inline_unalign_xrecalloc(opq_alloc);

    // Test `inline_unalign_xrecalloc_to`.
    _ = allocator.opq_inline_unalign_xrecalloc_to(allocator, opq_alloc);

    // Test `opq_inline_align_recalloc`.
    _ = allocator.opq_inline_align_recalloc(opq_alloc, 8u).value();

    // Test `inline_align_recalloc_to`.
    _ = allocator.opq_inline_align_recalloc_to(allocator, opq_alloc, 8u)
            .value();

    // Test `opq_inline_align_xrecalloc`.
    _ = allocator.opq_inline_align_xrecalloc(opq_alloc, 8u);

    // Test `inline_align_xrecalloc_to`.
    _ = allocator.opq_inline_align_xrecalloc_to(allocator, opq_alloc, 8u);

    // Test `opq_inline_unalign_recalloc`.
    _ = allocator.opq_inline_unalign_recalloc(opq_alloc).value();

    // Test `inline_unalign_recalloc_to`.
    _ = allocator.opq_inline_unalign_recalloc_to(allocator, opq_alloc).value();

    // Test `opq_inline_unalign_xrecalloc`.
    _ = allocator.opq_inline_unalign_xrecalloc(opq_alloc);

    // Test `inline_unalign_xrecalloc_to`.
    _ = allocator.opq_inline_unalign_xrecalloc_to(allocator, opq_alloc);

    // Test `inline_recalloc_multi`.
    _ = allocator.opq_inline_recalloc_multi(opq_alloc, 10).value();

    // Test `inline_recalloc_multi_to`.
    _ = allocator.opq_inline_recalloc_multi_to(allocator, opq_alloc, 10)
            .value();

    // Test `inline_xrecalloc_multi`.
    _ = allocator.opq_inline_xrecalloc_multi(opq_alloc, 10);

    // Test `inline_xrecalloc_multi_to`.
    _ = allocator.opq_inline_xrecalloc_multi_to(allocator, opq_alloc, 10);

    // Test `inline_align_recalloc_multi`.
    _ = allocator.opq_inline_align_recalloc_multi(opq_alloc, 8u, 10).value();

    // Test `inline_align_recalloc_multi_to`.
    _ = allocator
            .opq_inline_align_recalloc_multi_to(allocator, opq_alloc, 8u, 10)
            .value();

    // Test `inline_align_xrecalloc_multi`.
    _ = allocator.opq_inline_align_xrecalloc_multi(opq_alloc, 8u, 10);

    // Test `inline_align_xrecalloc_multi_to`.
    _ = allocator.opq_inline_align_xrecalloc_multi_to(allocator, opq_alloc, 8u,
                                                      10);

    // Test `inline_unalign_recalloc_multi`.
    _ = allocator.opq_inline_unalign_recalloc_multi(opq_alloc, 10).value();

    // Test `inline_unalign_recalloc_multi_to`.
    _ = allocator.opq_inline_unalign_recalloc_multi_to(allocator, opq_alloc, 10)
            .value();

    // Test `inline_unalign_xrecalloc_multi`.
    _ = allocator.opq_inline_unalign_xrecalloc_multi(opq_alloc, 10);

    // Test `inline_unalign_xrecalloc_multi_to`.
    _ = allocator.opq_inline_unalign_xrecalloc_multi_to(allocator, opq_alloc,
                                                        10);

    // Test `inline_align_recalloc_multi`.
    _ = allocator.opq_inline_align_recalloc_multi(opq_alloc, 8u, 10).value();

    // Test `inline_align_recalloc_multi_to`.
    _ = allocator
            .opq_inline_align_recalloc_multi_to(allocator, opq_alloc, 8u, 10)
            .value();

    // Test `inline_align_xrecalloc_multi`.
    _ = allocator.opq_inline_align_xrecalloc_multi(opq_alloc, 8u, 10);

    // Test `inline_align_xrecalloc_multi_to`.
    _ = allocator.opq_inline_align_xrecalloc_multi_to(allocator, opq_alloc, 8u,
                                                      10);

    // Test `inline_unalign_recalloc_multi`.
    _ = allocator.opq_inline_unalign_recalloc_multi(opq_alloc, 10).value();

    // Test `inline_unalign_recalloc_multi_to`.
    _ = allocator.opq_inline_unalign_recalloc_multi_to(allocator, opq_alloc, 10)
            .value();

    // Test `inline_unalign_xrecalloc_multi`.
    _ = allocator.opq_inline_unalign_xrecalloc_multi(opq_alloc, 10);

    // Test `inline_unalign_xrecalloc_multi_to`.
    _ = allocator.opq_inline_unalign_xrecalloc_multi_to(allocator, opq_alloc,
                                                        10);

    // Test `opq_resalloc`.
    _ = allocator.opq_resalloc(opq_alloc).value();

    // Test `resalloc_to`.
    _ = allocator.opq_resalloc_to(allocator, opq_alloc).value();

    // Test `resalloc`.
    _ = allocator.resalloc(p_alloc).value().first();

    // Test `resalloc_to`
    _ = allocator.resalloc_to(allocator, p_alloc).value().first();

    // Test `opq_xresalloc`.
    _ = allocator.opq_xresalloc(opq_alloc);

    // Test `xresalloc_to`.
    _ = allocator.opq_xresalloc_to(allocator, opq_alloc);

    // Test `xresalloc`
    _ = allocator.xresalloc(p_alloc).first();

    // Test `xresalloc_to`
    _ = allocator.xresalloc_to(allocator, p_alloc).first();

    // Test `opq_align_resalloc`.
    _ = allocator.opq_align_resalloc(opq_alloc, 8u).value();

    // Test `align_resalloc_to`.
    _ = allocator.opq_align_resalloc_to(allocator, opq_alloc, 8u).value();

    // Test `opq_align_xresalloc`.
    _ = allocator.opq_align_xresalloc(opq_alloc, 8u);

    // Test `align_xresalloc_to`.
    _ = allocator.opq_align_xresalloc_to(allocator, opq_alloc, 8u);

    // Test `opq_unalign_resalloc`.
    _ = allocator.opq_unalign_resalloc(opq_alloc).value();

    // Test `unalign_resalloc_to`.
    _ = allocator.opq_unalign_resalloc_to(allocator, opq_alloc).value();

    // Test `opq_unalign_xresalloc`.
    _ = allocator.opq_unalign_xresalloc(opq_alloc);

    // Test `unalign_xresalloc_to`.
    _ = allocator.opq_unalign_xresalloc_to(allocator, opq_alloc);

    // Test `opq_align_resalloc`.
    _ = allocator.opq_align_resalloc(opq_alloc, 8u).value();

    // Test `align_resalloc_to`.
    _ = allocator.opq_align_resalloc_to(allocator, opq_alloc, 8u).value();

    // Test `opq_align_xresalloc`.
    _ = allocator.opq_align_xresalloc(opq_alloc, 8u);

    // Test `align_xresalloc_to`.
    _ = allocator.opq_align_xresalloc_to(allocator, opq_alloc, 8u);

    // Test `opq_unalign_resalloc`.
    _ = allocator.opq_unalign_resalloc(opq_alloc).value();

    // Test `unalign_resalloc_to`.
    _ = allocator.opq_unalign_resalloc_to(allocator, opq_alloc).value();

    // Test `opq_unalign_xresalloc`.
    _ = allocator.opq_unalign_xresalloc(opq_alloc);

    // Test `unalign_xresalloc_to`.
    _ = allocator.opq_unalign_xresalloc_to(allocator, opq_alloc);

    // Test `align_resalloc`.
    _ = allocator.align_resalloc(p_alloc, 8u).value();

    // Test `align_resalloc_to`.
    _ = allocator.align_resalloc_to(allocator, p_alloc, 8u).value();

    // Test `align_xresalloc`.
    _ = allocator.align_xresalloc(p_alloc, 8u);

    // Test `align_xresalloc_to`.
    _ = allocator.align_xresalloc_to(allocator, p_alloc, 8u);

    // Test `unalign_resalloc`.
    _ = allocator.unalign_resalloc(p_alloc).value();

    // Test `unalign_resalloc_to`.
    _ = allocator.unalign_resalloc_to(allocator, p_alloc).value();

    // Test `unalign_xresalloc`.
    _ = allocator.unalign_xresalloc(p_alloc);

    // Test `unalign_xresalloc_to`.
    _ = allocator.unalign_xresalloc_to(allocator, p_alloc);

    // Test `resalloc_multi`.
    _ = allocator.opq_resalloc_multi(opq_alloc, 10).value();

    // Test `resalloc_multi_to`.
    _ = allocator.opq_resalloc_multi_to(allocator, opq_alloc, 10).value();

    // Test `resalloc_multi`.
    _ = allocator.resalloc_multi(p_alloc, 5, 10).value().first().data();

    // Test `resalloc_multi_to`
    _ = allocator.resalloc_multi_to(allocator, p_alloc, 5, 10)
            .value()
            .first()
            .data();

    // Test `xresalloc_multi`.
    _ = allocator.opq_xresalloc_multi(opq_alloc, 10);

    // Test `xresalloc_multi_to`.
    _ = allocator.opq_xresalloc_multi_to(allocator, opq_alloc, 10);

    // Test `xresalloc_multi`
    _ = allocator.xresalloc_multi(p_alloc, 5, 10);

    // Test `xresalloc_multi_to`
    _ = allocator.xresalloc_multi_to(allocator, p_alloc, 5, 10);

    // Test `align_resalloc_multi`.
    _ = allocator.opq_align_resalloc_multi(opq_alloc, 8u, 10).value();

    // Test `align_resalloc_multi_to`.
    _ = allocator.opq_align_resalloc_multi_to(allocator, opq_alloc, 8u, 10)
            .value();

    // Test `align_xresalloc_multi`.
    _ = allocator.opq_align_xresalloc_multi(opq_alloc, 8u, 10);

    // Test `align_xresalloc_multi_to`.
    _ = allocator.opq_align_xresalloc_multi_to(allocator, opq_alloc, 8u, 10);

    // Test `unalign_resalloc_multi`.
    _ = allocator.opq_unalign_resalloc_multi(opq_alloc, 10).value();

    // Test `unalign_resalloc_multi_to`.
    _ = allocator.opq_unalign_resalloc_multi_to(allocator, opq_alloc, 10)
            .value();

    // Test `unalign_xresalloc_multi`.
    _ = allocator.opq_unalign_xresalloc_multi(opq_alloc, 10);

    // Test `unalign_xresalloc_multi_to`.
    _ = allocator.opq_unalign_xresalloc_multi_to(allocator, opq_alloc, 10);

    // Test `align_resalloc_multi`.
    _ = allocator.opq_align_resalloc_multi(opq_alloc, 8u, 10).value();

    // Test `align_resalloc_multi_to`.
    _ = allocator.opq_align_resalloc_multi_to(allocator, opq_alloc, 8u, 10)
            .value();

    // Test `align_xresalloc_multi`.
    _ = allocator.opq_align_xresalloc_multi(opq_alloc, 8u, 10);

    // Test `align_xresalloc_multi_to`.
    _ = allocator.opq_align_xresalloc_multi_to(allocator, opq_alloc, 8u, 10);

    // Test `unalign_resalloc_multi`.
    _ = allocator.opq_unalign_resalloc_multi(opq_alloc, 10).value();

    // Test `unalign_resalloc_multi_to`.
    _ = allocator.opq_unalign_resalloc_multi_to(allocator, opq_alloc, 10)
            .value();

    // Test `unalign_xresalloc_multi`.
    _ = allocator.opq_unalign_xresalloc_multi(opq_alloc, 10);

    // Test `unalign_xresalloc_multi_to`.
    _ = allocator.opq_unalign_xresalloc_multi_to(allocator, opq_alloc, 10);

    // Test `align_resalloc_multi`.
    _ = allocator.align_resalloc_multi(p_alloc, 8u, 5, 10).value();

    // Test `align_resalloc_multi_to`.
    _ = allocator.align_resalloc_multi_to(allocator, p_alloc, 8u, 5, 10)
            .value();

    // Test `align_xresalloc_multi`.
    _ = allocator.align_xresalloc_multi(p_alloc, 8u, 5, 10);

    // Test `align_xresalloc_multi_to`.
    _ = allocator.align_xresalloc_multi_to(allocator, p_alloc, 8u, 5, 10);

    // Test `unalign_resalloc_multi`.
    _ = allocator.unalign_resalloc_multi(p_alloc, 5, 10).value();

    // Test `unalign_resalloc_multi_to`.
    _ = allocator.unalign_resalloc_multi_to(allocator, p_alloc, 5, 10)
            .value()
            .first()
            .data();

    // Test `unalign_xresalloc_multi`.
    _ = allocator.unalign_xresalloc_multi(p_alloc, 5, 10);

    // Test `unalign_xresalloc_multi_to`.
    _ = allocator.unalign_xresalloc_multi_to(allocator, p_alloc, 5, 10);

    // The allocator runs out of memory around here.
    allocator.reset();

    // Test `opq_rescalloc`.
    _ = allocator.opq_rescalloc(opq_alloc).value();

    // Test `rescalloc_to`.
    _ = allocator.opq_rescalloc_to(allocator, opq_alloc).value();

    // Test `rescalloc`.
    _ = allocator.rescalloc(p_alloc).value();

    // Test `rescalloc_to`
    _ = allocator.rescalloc_to(allocator, p_alloc);

    // Test `opq_xrescalloc`.
    _ = allocator.opq_xrescalloc(opq_alloc);

    // Test `xrescalloc_to`.
    _ = allocator.opq_xrescalloc_to(allocator, opq_alloc);

    // Test `xrescalloc`
    _ = allocator.xrescalloc(p_alloc);

    // Test `xrescalloc_to`
    _ = allocator.xrescalloc_to(allocator, p_alloc);

    // Test `opq_align_rescalloc`.
    _ = allocator.opq_align_rescalloc(opq_alloc, 8u).value();

    // Test `align_rescalloc_to`.
    _ = allocator.opq_align_rescalloc_to(allocator, opq_alloc, 8u).value();

    // Test `opq_align_xrescalloc`.
    _ = allocator.opq_align_xrescalloc(opq_alloc, 8u);

    // Test `align_xrescalloc_to`.
    _ = allocator.opq_align_xrescalloc_to(allocator, opq_alloc, 8u);

    // Test `opq_unalign_rescalloc`.
    _ = allocator.opq_unalign_rescalloc(opq_alloc).value();

    // Test `unalign_rescalloc_to`.
    _ = allocator.opq_unalign_rescalloc_to(allocator, opq_alloc).value();

    // Test `opq_unalign_xrescalloc`.
    _ = allocator.opq_unalign_xrescalloc(opq_alloc);

    // Test `unalign_xrescalloc_to`.
    _ = allocator.opq_unalign_xrescalloc_to(allocator, opq_alloc);

    // Test `opq_align_rescalloc`.
    _ = allocator.opq_align_rescalloc(opq_alloc, 8u).value();

    // Test `align_rescalloc_to`.
    _ = allocator.opq_align_rescalloc_to(allocator, opq_alloc, 8u).value();

    // Test `opq_align_xrescalloc`.
    _ = allocator.opq_align_xrescalloc(opq_alloc, 8u);

    // Test `align_xrescalloc_to`.
    _ = allocator.opq_align_xrescalloc_to(allocator, opq_alloc, 8u);

    // Test `opq_unalign_rescalloc`.
    _ = allocator.opq_unalign_rescalloc(opq_alloc).value();

    // Test `unalign_rescalloc_to`.
    _ = allocator.opq_unalign_rescalloc_to(allocator, opq_alloc).value();

    // Test `opq_unalign_xrescalloc`.
    _ = allocator.opq_unalign_xrescalloc(opq_alloc);

    // Test `unalign_xrescalloc_to`.
    _ = allocator.opq_unalign_xrescalloc_to(allocator, opq_alloc);

    // Test `align_rescalloc`.
    _ = allocator.align_rescalloc(p_alloc, 8u).value();

    // Test `align_rescalloc_to`.
    _ = allocator.align_rescalloc_to(allocator, p_alloc, 8u).value();

    // Test `align_xrescalloc`.
    _ = allocator.align_xrescalloc(p_alloc, 8u);

    // Test `align_xrescalloc_to`.
    _ = allocator.align_xrescalloc_to(allocator, p_alloc, 8u);

    // Test `unalign_rescalloc`.
    _ = allocator.unalign_rescalloc(p_alloc).value();

    // Test `unalign_rescalloc_to`.
    _ = allocator.unalign_rescalloc_to(allocator, p_alloc).value();

    // Test `unalign_xrescalloc`.
    _ = allocator.unalign_xrescalloc(p_alloc);

    // Test `unalign_xrescalloc_to`.
    _ = allocator.unalign_xrescalloc_to(allocator, p_alloc);

    // Test `rescalloc_multi`.
    _ = allocator.opq_rescalloc_multi(opq_alloc, 10).value();

    // Test `rescalloc_multi_to`.
    _ = allocator.opq_rescalloc_multi_to(allocator, opq_alloc, 10).value();

    // Test `rescalloc_multi`.
    _ = allocator.rescalloc_multi(p_alloc, 5, 10).value().first().data();

    // Test `rescalloc_multi_to`
    _ = allocator.rescalloc_multi_to(allocator, p_alloc, 5, 10)
            .value()
            .first()
            .data();

    // Test `xrescalloc_multi`.
    _ = allocator.opq_xrescalloc_multi(opq_alloc, 10);

    // Test `xrescalloc_multi_to`.
    _ = allocator.opq_xrescalloc_multi_to(allocator, opq_alloc, 10);

    // Test `xrescalloc_multi`
    _ = allocator.xrescalloc_multi(p_alloc, 5, 10);

    // Test `xrescalloc_multi_to`
    _ = allocator.xrescalloc_multi_to(allocator, p_alloc, 5, 10);

    // Test `align_rescalloc_multi`.
    _ = allocator.opq_align_rescalloc_multi(opq_alloc, 8u, 10).value();

    // Test `align_rescalloc_multi_to`.
    _ = allocator.opq_align_rescalloc_multi_to(allocator, opq_alloc, 8u, 10)
            .value();

    // Test `align_xrescalloc_multi`.
    _ = allocator.opq_align_xrescalloc_multi(opq_alloc, 8u, 10);

    // Test `align_xrescalloc_multi_to`.
    _ = allocator.opq_align_xrescalloc_multi_to(allocator, opq_alloc, 8u, 10);

    // Test `unalign_rescalloc_multi`.
    _ = allocator.opq_unalign_rescalloc_multi(opq_alloc, 10).value();

    // Test `unalign_rescalloc_multi_to`.
    _ = allocator.opq_unalign_rescalloc_multi_to(allocator, opq_alloc, 10)
            .value();

    // Test `unalign_xrescalloc_multi`.
    _ = allocator.opq_unalign_xrescalloc_multi(opq_alloc, 10);

    // Test `unalign_xrescalloc_multi_to`.
    _ = allocator.opq_unalign_xrescalloc_multi_to(allocator, opq_alloc, 10);

    // Test `align_rescalloc_multi`.
    _ = allocator.opq_align_rescalloc_multi(opq_alloc, 8u, 10).value();

    // Test `align_rescalloc_multi_to`.
    _ = allocator.opq_align_rescalloc_multi_to(allocator, opq_alloc, 8u, 10)
            .value();

    // Test `align_xrescalloc_multi`.
    _ = allocator.opq_align_xrescalloc_multi(opq_alloc, 8u, 10);

    // Test `align_xrescalloc_multi_to`.
    _ = allocator.opq_align_xrescalloc_multi_to(allocator, opq_alloc, 8u, 10);

    // Test `unalign_rescalloc_multi`.
    _ = allocator.opq_unalign_rescalloc_multi(opq_alloc, 10).value();

    // Test `unalign_rescalloc_multi_to`.
    _ = allocator.opq_unalign_rescalloc_multi_to(allocator, opq_alloc, 10)
            .value();

    // Test `unalign_xrescalloc_multi`.
    _ = allocator.opq_unalign_xrescalloc_multi(opq_alloc, 10);

    // Test `unalign_xrescalloc_multi_to`.
    _ = allocator.opq_unalign_xrescalloc_multi_to(allocator, opq_alloc, 10);

    // Test `align_rescalloc_multi`.
    _ = allocator.align_rescalloc_multi(p_alloc, 8u, 5, 10)
            .value()
            .first()
            .data();

    // Test `align_rescalloc_multi_to`.
    _ = allocator.align_rescalloc_multi_to(allocator, p_alloc, 8u, 5, 10)
            .value();

    // Test `align_xrescalloc_multi`.
    _ = allocator.align_xrescalloc_multi(p_alloc, 8u, 5, 10);

    // Test `align_xrescalloc_multi_to`.
    _ = allocator.align_xrescalloc_multi_to(allocator, p_alloc, 8u, 5, 10);

    // Test `unalign_rescalloc_multi`.
    _ = allocator.unalign_rescalloc_multi(p_alloc, 5, 10)
            .value()
            .first()
            .data();

    // Test `unalign_rescalloc_multi_to`.
    _ = allocator.unalign_rescalloc_multi_to(allocator, p_alloc, 5, 10)
            .value()
            .first()
            .data();

    // Test `unalign_xrescalloc_multi`.
    _ = allocator.unalign_xrescalloc_multi(p_alloc, 5, 10);

    // Test `unalign_xrescalloc_multi_to`.
    _ = allocator.unalign_xrescalloc_multi_to(allocator, p_alloc, 5, 10);

    // Test `opq_inline_resalloc`.
    _ = allocator.opq_inline_resalloc(opq_alloc);

    // Test `inline_resalloc_to`.
    _ = allocator.opq_inline_resalloc_to(allocator, opq_alloc).value();

    // Test `opq_inline_xresalloc`.
    _ = allocator.opq_inline_xresalloc(opq_alloc);

    // Test `inline_xresalloc_to`.
    _ = allocator.opq_inline_xresalloc_to(allocator, opq_alloc);

    // Test `opq_inline_align_resalloc`.
    _ = allocator.opq_inline_align_resalloc(opq_alloc, 8u).value();

    // Test `inline_align_resalloc_to`.
    _ = allocator.opq_inline_align_resalloc_to(allocator, opq_alloc, 8u)
            .value();

    // Test `opq_inline_align_xresalloc`.
    _ = allocator.opq_inline_align_xresalloc(opq_alloc, 8u);

    // Test `inline_align_xresalloc_to`.
    _ = allocator.opq_inline_align_xresalloc_to(allocator, opq_alloc, 8u);

    // Test `opq_inline_unalign_resalloc`.
    _ = allocator.opq_inline_unalign_resalloc(opq_alloc).value();

    // Test `inline_unalign_resalloc_to`.
    _ = allocator.opq_inline_unalign_resalloc_to(allocator, opq_alloc).value();

    // Test `opq_inline_unalign_xresalloc`.
    _ = allocator.opq_inline_unalign_xresalloc(opq_alloc);

    // Test `inline_unalign_xresalloc_to`.
    _ = allocator.opq_inline_unalign_xresalloc_to(allocator, opq_alloc);

    // Test `opq_inline_align_resalloc`.
    _ = allocator.opq_inline_align_resalloc(opq_alloc, 8u).value();

    // Test `inline_align_resalloc_to`.
    _ = allocator.opq_inline_align_resalloc_to(allocator, opq_alloc, 8u)
            .value();

    // Test `opq_inline_align_xresalloc`.
    _ = allocator.opq_inline_align_xresalloc(opq_alloc, 8u);

    // Test `inline_align_xresalloc_to`.
    _ = allocator.opq_inline_align_xresalloc_to(allocator, opq_alloc, 8u);

    // Test `opq_inline_unalign_resalloc`.
    _ = allocator.opq_inline_unalign_resalloc(opq_alloc).value();

    // Test `inline_unalign_resalloc_to`.
    _ = allocator.opq_inline_unalign_resalloc_to(allocator, opq_alloc).value();

    // Test `opq_inline_unalign_xresalloc`.
    _ = allocator.opq_inline_unalign_xresalloc(opq_alloc);

    // Test `inline_unalign_xresalloc_to`.
    _ = allocator.opq_inline_unalign_xresalloc_to(allocator, opq_alloc);

    // Test `inline_resalloc_multi`.
    _ = allocator.opq_inline_resalloc_multi(opq_alloc, 10).value();

    // Test `inline_resalloc_multi_to`.
    _ = allocator.opq_inline_resalloc_multi_to(allocator, opq_alloc, 10)
            .value();

    // Test `inline_xresalloc_multi`.
    _ = allocator.opq_inline_xresalloc_multi(opq_alloc, 10);

    // Test `inline_xresalloc_multi_to`.
    _ = allocator.opq_inline_xresalloc_multi_to(allocator, opq_alloc, 10);

    // Test `inline_align_resalloc_multi`.
    _ = allocator.opq_inline_align_resalloc_multi(opq_alloc, 8u, 10).value();

    // Test `inline_align_resalloc_multi_to`.
    _ = allocator
            .opq_inline_align_resalloc_multi_to(allocator, opq_alloc, 8u, 10)
            .value();

    // Test `inline_align_xresalloc_multi`.
    _ = allocator.opq_inline_align_xresalloc_multi(opq_alloc, 8u, 10);

    // Test `inline_align_xresalloc_multi_to`.
    _ = allocator.opq_inline_align_xresalloc_multi_to(allocator, opq_alloc, 8u,
                                                      10);

    // Test `inline_unalign_resalloc_multi`.
    _ = allocator.opq_inline_unalign_resalloc_multi(opq_alloc, 10).value();

    // Test `inline_unalign_resalloc_multi_to`.
    _ = allocator.opq_inline_unalign_resalloc_multi_to(allocator, opq_alloc, 10)
            .value();

    // Test `inline_unalign_xresalloc_multi`.
    _ = allocator.opq_inline_unalign_xresalloc_multi(opq_alloc, 10);

    // Test `inline_unalign_xresalloc_multi_to`.
    _ = allocator.opq_inline_unalign_xresalloc_multi_to(allocator, opq_alloc,
                                                        10);

    // Test `inline_align_resalloc_multi`.
    _ = allocator.opq_inline_align_resalloc_multi(opq_alloc, 8u, 10).value();

    // Test `inline_align_resalloc_multi_to`.
    _ = allocator
            .opq_inline_align_resalloc_multi_to(allocator, opq_alloc, 8u, 10)
            .value();

    // Test `inline_align_xresalloc_multi`.
    _ = allocator.opq_inline_align_xresalloc_multi(opq_alloc, 8u, 10);

    // Test `inline_align_xresalloc_multi_to`.
    _ = allocator.opq_inline_align_xresalloc_multi_to(allocator, opq_alloc, 8u,
                                                      10);

    // Test `inline_unalign_resalloc_multi`.
    _ = allocator.opq_inline_unalign_resalloc_multi(opq_alloc, 10).value();

    // Test `inline_unalign_resalloc_multi_to`.
    _ = allocator.opq_inline_unalign_resalloc_multi_to(allocator, opq_alloc, 10)
            .value();

    // Test `inline_unalign_xresalloc_multi`.
    _ = allocator.opq_inline_unalign_xresalloc_multi(opq_alloc, 10);

    // Test `inline_unalign_xresalloc_multi_to`.
    _ = allocator.opq_inline_unalign_xresalloc_multi_to(allocator, opq_alloc,
                                                        10);

    // The allocator runs out of memory around here.
    allocator.reset();

    // Test `opq_inline_rescalloc`.
    _ = allocator.opq_inline_rescalloc(opq_alloc).value();

    // Test `inline_rescalloc_to`.
    _ = allocator.opq_inline_rescalloc_to(allocator, opq_alloc).value();

    // Test `opq_inline_xrescalloc`.
    _ = allocator.opq_inline_xrescalloc(opq_alloc);

    // Test `inline_xrescalloc_to`.
    _ = allocator.opq_inline_xrescalloc_to(allocator, opq_alloc);

    // Test `opq_inline_align_rescalloc`.
    _ = allocator.opq_inline_align_rescalloc(opq_alloc, 8u).value();

    // Test `inline_align_rescalloc_to`.
    _ = allocator.opq_inline_align_rescalloc_to(allocator, opq_alloc, 8u)
            .value();

    // Test `opq_inline_align_xrescalloc`.
    _ = allocator.opq_inline_align_xrescalloc(opq_alloc, 8u);

    // Test `inline_align_xrescalloc_to`.
    _ = allocator.opq_inline_align_xrescalloc_to(allocator, opq_alloc, 8u);

    // Test `opq_inline_unalign_rescalloc`.
    _ = allocator.opq_inline_unalign_rescalloc(opq_alloc).value();

    // Test `inline_unalign_rescalloc_to`.
    _ = allocator.opq_inline_unalign_rescalloc_to(allocator, opq_alloc).value();

    // Test `opq_inline_unalign_xrescalloc`.
    _ = allocator.opq_inline_unalign_xrescalloc(opq_alloc);

    // Test `inline_unalign_xrescalloc_to`.
    _ = allocator.opq_inline_unalign_xrescalloc_to(allocator, opq_alloc);

    // Test `opq_inline_align_rescalloc`.
    _ = allocator.opq_inline_align_rescalloc(opq_alloc, 8u).value();

    // Test `inline_align_rescalloc_to`.
    _ = allocator.opq_inline_align_rescalloc_to(allocator, opq_alloc, 8u)
            .value();

    // Test `opq_inline_align_xrescalloc`.
    _ = allocator.opq_inline_align_xrescalloc(opq_alloc, 8u);

    // Test `inline_align_xrescalloc_to`.
    _ = allocator.opq_inline_align_xrescalloc_to(allocator, opq_alloc, 8u);

    // Test `opq_inline_unalign_rescalloc`.
    _ = allocator.opq_inline_unalign_rescalloc(opq_alloc).value();

    // Test `inline_unalign_rescalloc_to`.
    _ = allocator.opq_inline_unalign_rescalloc_to(allocator, opq_alloc).value();

    // Test `opq_inline_unalign_xrescalloc`.
    _ = allocator.opq_inline_unalign_xrescalloc(opq_alloc);

    // Test `inline_unalign_xrescalloc_to`.
    _ = allocator.opq_inline_unalign_xrescalloc_to(allocator, opq_alloc);

    // Test `inline_rescalloc_multi`.
    _ = allocator.opq_inline_rescalloc_multi(opq_alloc, 10).value();

    // Test `inline_rescalloc_multi_to`.
    _ = allocator.opq_inline_rescalloc_multi_to(allocator, opq_alloc, 10)
            .value();

    // Test `inline_xrescalloc_multi`.
    _ = allocator.opq_inline_xrescalloc_multi(opq_alloc, 10);

    // Test `inline_xrescalloc_multi_to`.
    _ = allocator.opq_inline_xrescalloc_multi_to(allocator, opq_alloc, 10);

    // Test `inline_align_rescalloc_multi`.
    _ = allocator.opq_inline_align_rescalloc_multi(opq_alloc, 8u, 10).value();

    // Test `inline_align_rescalloc_multi_to`.
    _ = allocator
            .opq_inline_align_rescalloc_multi_to(allocator, opq_alloc, 8u, 10)
            .value();

    // Test `inline_align_xrescalloc_multi`.
    _ = allocator.opq_inline_align_xrescalloc_multi(opq_alloc, 8u, 10);

    // Test `inline_align_xrescalloc_multi_to`.
    _ = allocator.opq_inline_align_xrescalloc_multi_to(allocator, opq_alloc, 8u,
                                                       10);

    // Test `inline_unalign_rescalloc_multi`.
    _ = allocator.opq_inline_unalign_rescalloc_multi(opq_alloc, 10).value();

    // Test `inline_unalign_rescalloc_multi_to`.
    _ = allocator
            .opq_inline_unalign_rescalloc_multi_to(allocator, opq_alloc, 10)
            .value();

    // Test `inline_unalign_xrescalloc_multi`.
    _ = allocator.opq_inline_unalign_xrescalloc_multi(opq_alloc, 10);

    // Test `inline_unalign_xrescalloc_multi_to`.
    _ = allocator.opq_inline_unalign_xrescalloc_multi_to(allocator, opq_alloc,
                                                         10);

    // Test `inline_align_rescalloc_multi`.
    _ = allocator.opq_inline_align_rescalloc_multi(opq_alloc, 8u, 10).value();

    // Test `inline_align_rescalloc_multi_to`.
    _ = allocator
            .opq_inline_align_rescalloc_multi_to(allocator, opq_alloc, 8u, 10)
            .value();

    // Test `inline_align_xrescalloc_multi`.
    _ = allocator.opq_inline_align_xrescalloc_multi(opq_alloc, 8u, 10);

    // Test `inline_align_xrescalloc_multi_to`.
    _ = allocator.opq_inline_align_xrescalloc_multi_to(allocator, opq_alloc, 8u,
                                                       10);

    // Test `inline_unalign_rescalloc_multi`.
    _ = allocator.opq_inline_unalign_rescalloc_multi(opq_alloc, 10).value();

    // Test `inline_unalign_rescalloc_multi_to`.
    _ = allocator
            .opq_inline_unalign_rescalloc_multi_to(allocator, opq_alloc, 10)
            .value();

    // Test `inline_unalign_xrescalloc_multi`.
    _ = allocator.opq_inline_unalign_xrescalloc_multi(opq_alloc, 10);

    // Test `inline_unalign_xrescalloc_multi_to`.
    _ = allocator.opq_inline_unalign_xrescalloc_multi_to(allocator, opq_alloc,
                                                         10);
};
