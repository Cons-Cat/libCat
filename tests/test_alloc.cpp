#include <cat/bit>
#include <cat/linear_allocator>
#include <cat/page_allocator>

struct HugeObject {
    [[maybe_unused]] uint1 storage[256];
};

int4 global = 0;

struct NonTrivial {
    char storage;
    NonTrivial() {
        ++global;
    }
};

struct NonTrivialHugeObject {
    [[maybe_unused]] uint1 storage[256];
    NonTrivialHugeObject() {
        ++global;
    }
};

auto main() -> int {
    // Initialize an allocator.
    cat::PageAllocator paging_allocator;
    int4* p_page = paging_allocator.p_alloc_multi<int4>(1_ki).or_exit();
    defer(paging_allocator.free_multi(p_page, 1_ki);)
    cat::LinearAllocator allocator(p_page, 4_ki);

    // Test `alloc()`.
    _ = allocator.alloc<int4>().value();
    auto alloc = allocator.alloc<int4>(1).value();
    Result(allocator.get(alloc) == 1).or_exit();
    global = 0;
    _ = allocator.alloc<NonTrivial>();
    Result(global == 1).or_exit();

    // Test `xalloc()`.
    _ = allocator.xalloc<int4>();
    auto xalloc = allocator.xalloc<int4>(1);
    Result(allocator.get(xalloc) == 1).or_exit();

    // Test `p_alloc()`.
    _ = allocator.p_alloc<int4>().value();
    auto p_alloc = allocator.p_alloc<int4>(1).value();
    Result(*p_alloc == 1).or_exit();

    // Test `p_xalloc()`.
    _ = allocator.p_xalloc<int4>();
    auto p_xalloc = allocator.p_xalloc<int4>(1);
    Result(*p_xalloc == 1).or_exit();

    // Test `alloc_multi()`.
    auto alloc_multi = allocator.alloc_multi<int4>(5).value();
    Result(alloc_multi.size() == 5).or_exit();
    Result(alloc_multi.raw_size() == 20).or_exit();
    global = 0;
    _ = allocator.alloc_multi<NonTrivial>(5);
    Result(global == 5).or_exit();

    // Test `xalloc_multi()`.
    auto xalloc_multi = allocator.xalloc_multi<int4>(5);
    Result(xalloc_multi.size() == 5).or_exit();
    Result(xalloc_multi.raw_size() == 20).or_exit();

    // Test `p_alloc_multi()`.
    _ = allocator.p_alloc_multi<int4>(5).value();

    // Test `p_xalloc_multi()`.
    _ = allocator.p_xalloc_multi<int4>(5);

    // Test `align_alloc()`.
    _ = allocator.align_alloc<int4>(8u).value();
    auto align_alloc = allocator.align_alloc<int4>(8u, 1).value();
    Result(allocator.get(align_alloc) == 1).or_exit();
    Result(cat::is_aligned(&allocator.get(align_alloc), 8u)).or_exit();

    // Test `align_xalloc()`.
    _ = allocator.align_xalloc<int4>(8u);
    auto align_xalloc = allocator.align_xalloc<int4>(8u, 1);
    Result(allocator.get(align_xalloc) == 1).or_exit();
    Result(cat::is_aligned(&allocator.get(align_xalloc), 8u)).or_exit();

    // Test `p_align_alloc()`.
    _ = allocator.p_align_alloc<int4>(8u).value();
    auto p_align_alloc = allocator.p_align_alloc<int4>(8u, 1).value();
    Result(*p_align_alloc == 1).or_exit();
    Result(cat::is_aligned(p_align_alloc, 8u)).or_exit();

    // Test `p_align_xalloc()`.
    _ = allocator.p_align_xalloc<int4>(8u);
    auto p_align_xalloc = allocator.p_align_xalloc<int4>(8u, 1);
    Result(*p_align_xalloc == 1).or_exit();
    Result(cat::is_aligned(p_align_xalloc, 8u)).or_exit();

    // Test `unalign_alloc()`.
    _ = allocator.unalign_alloc<int4>().value();
    auto unalign_alloc = allocator.unalign_alloc<int4>(1).value();
    Result(allocator.get(unalign_alloc) == 1).or_exit();

    // Test `unalign_xalloc()`.
    _ = allocator.unalign_xalloc<int4>(8u);
    auto unalign_xalloc = allocator.unalign_xalloc<int4>(1);
    Result(allocator.get(unalign_xalloc) == 1).or_exit();

    // Test `p_unalign_alloc()`.
    _ = allocator.p_unalign_alloc<int4>(8u).value();
    auto p_unalign_alloc = allocator.p_unalign_alloc<int4>(1).value();
    Result(*p_unalign_alloc == 1).or_exit();

    // Test `p_unalign_xalloc()`.
    _ = allocator.p_unalign_xalloc<int4>(8u);
    auto p_unalign_xalloc = allocator.p_unalign_xalloc<int4>(1);
    Result(*p_unalign_xalloc == 1).or_exit();

    // Test `align_alloc_multi()`.
    auto align_alloc_multi = allocator.align_alloc_multi<int4>(8u, 5).value();
    Result(align_alloc_multi.size() == 5).or_exit();
    Result(align_alloc_multi.raw_size() == 20).or_exit();
    Result(cat::is_aligned(allocator.get(align_alloc_multi).p_data(), 8u))
        .or_exit();
    global = 0;
    _ = allocator.align_alloc_multi<NonTrivial>(8u, 5);
    Result(global == 5).or_exit();

    // Test `align_xalloc_multi()`.
    auto align_xalloc_multi = allocator.align_xalloc_multi<int4>(8u, 5);
    Result(align_xalloc_multi.size() == 5).or_exit();
    Result(align_xalloc_multi.raw_size() == 20).or_exit();
    Result(cat::is_aligned(allocator.get(align_xalloc_multi).p_data(), 8u))
        .or_exit();
    global = 0;
    _ = allocator.align_xalloc_multi<NonTrivial>(8u, 5);
    Result(global == 5).or_exit();

    // Test `p_align_alloc_multi()`.
    auto p_align_alloc_multi =
        allocator.p_align_alloc_multi<int4>(8u, 5).value();
    Result(cat::is_aligned(p_align_alloc_multi, 8u)).or_exit();
    global = 0;
    _ = allocator.p_align_alloc_multi<NonTrivial>(8u, 5);
    Result(global == 5).or_exit();

    // Test `p_align_xalloc_multi()`.
    _ = allocator.p_align_xalloc_multi<int4>(8u, 5);
    global = 0;
    _ = allocator.p_align_xalloc_multi<NonTrivial>(8u, 5);
    Result(global == 5).or_exit();

    // Test `unalign_alloc_multi()`.
    auto unalign_alloc_multi = allocator.unalign_alloc_multi<int4>(5).value();
    Result(unalign_alloc_multi.size() == 5).or_exit();
    Result(unalign_alloc_multi.raw_size() == 20).or_exit();
    global = 0;
    _ = allocator.unalign_alloc_multi<NonTrivial>(5);
    Result(global == 5).or_exit();

    // Test `unalign_xalloc_multi()`.
    auto unalign_xalloc_multi = allocator.unalign_xalloc_multi<int1>(5);
    Result(unalign_xalloc_multi.size() == 5).or_exit();
    Result(unalign_xalloc_multi.raw_size() == 5).or_exit();
    global = 0;
    _ = allocator.unalign_xalloc_multi<NonTrivial>(5);
    Result(global == 5).or_exit();

    // Test `p_unalign_alloc_multi()`.
    _ = allocator.p_unalign_alloc_multi<int1>(5)
            .value();  // `int4` is 4-byte aligned.
    global = 0;
    _ = allocator.p_unalign_alloc_multi<NonTrivial>(5);
    Result(global == 5).or_exit();

    // Test `p_unalign_xalloc_multi()`.
    _ = allocator.p_unalign_xalloc_multi<int1>(5);  // `int4` is 4-byte aligned.
    global = 0;
    _ = allocator.p_unalign_xalloc_multi<NonTrivial>(5);
    Result(global == 5).or_exit();

    // Test `inline_alloc()`.
    _ = allocator.inline_alloc<int4>().value();
    auto inline_alloc = allocator.inline_alloc<int4>(1).value();
    Result(allocator.get(inline_alloc) == 1).or_exit();
    Result(inline_alloc.is_inline()).or_exit();
    global = 0;
    _ = allocator.inline_alloc<NonTrivial>();
    Result(global == 1).or_exit();

    // `HugeObject` is larger than the inline buffer.
    auto inline_alloc_2 = allocator.inline_alloc<HugeObject>().value();
    Result(!inline_alloc_2.is_inline()).or_exit();

    global = 0;
    _ = allocator.inline_alloc<NonTrivialHugeObject>();
    Result(global == 1).or_exit();

    // Test `inline_xalloc()`.
    _ = allocator.inline_xalloc<int4>();
    auto inline_xalloc = allocator.inline_xalloc<int4>(1);
    Result(allocator.get(inline_xalloc) == 1).or_exit();

    // Test `inline_alloc_multi()`.
    auto inline_alloc_multi = allocator.inline_alloc_multi<int4>(5).value();
    Result(inline_alloc_multi.size() == 5).or_exit();
    global = 0;
    _ = allocator.inline_alloc_multi<NonTrivial>(5);
    Result(global == 5).or_exit();

    // Test `inline_xalloc_multi()`.
    auto inline_xalloc_multi = allocator.inline_xalloc_multi<int4>(5);
    Result(inline_xalloc_multi.size() == 5).or_exit();
    global = 0;
    _ = allocator.inline_xalloc_multi<NonTrivial>(5);
    Result(global == 5).or_exit();

    // Test `inline_align_alloc()`.
    _ = allocator.inline_align_alloc<int4>(8u).value();
    auto inline_align_alloc = allocator.inline_align_alloc<int4>(8u, 1).value();
    Result(allocator.get(inline_align_alloc) == 1).or_exit();
    Result(cat::is_aligned(&allocator.get(inline_align_alloc), 8u)).or_exit();
    Result(inline_align_alloc.is_inline()).or_exit();

    // Test `inline_unalign_alloc()`.
    _ = allocator.inline_unalign_alloc<int4>(8u).value();
    auto inline_unalign_alloc = allocator.inline_unalign_alloc<int4>(1).value();
    Result(allocator.get(inline_unalign_alloc) == 1).or_exit();
    Result(inline_unalign_alloc.is_inline()).or_exit();

    // Test `inline_unalign_xalloc()`.
    _ = allocator.inline_unalign_xalloc<int4>(8u);
    auto inline_unalign_xalloc = allocator.inline_unalign_xalloc<int4>(1);
    Result(allocator.get(inline_unalign_xalloc) == 1).or_exit();
    Result(inline_unalign_xalloc.is_inline()).or_exit();

    allocator.reset();

    // Test `inline_align_alloc_multi()`.
    auto inline_align_alloc_multi =
        allocator.inline_align_alloc_multi<int4>(8u, 5).value();
    Result(
        cat::is_aligned(allocator.get(inline_align_alloc_multi).p_data(), 8u))
        .or_exit();
    Result(inline_align_alloc_multi.is_inline()).or_exit();

    auto inline_align_alloc_multi_big =
        allocator.inline_align_alloc_multi<int4>(8u, 64).value();
    Result(!inline_align_alloc_multi_big.is_inline()).or_exit();

    // Test `inline_align_xalloc_multi()`.
    auto inline_align_xalloc_multi =
        allocator.inline_align_xalloc_multi<int4>(8u, 5);
    Result(
        cat::is_aligned(allocator.get(inline_align_xalloc_multi).p_data(), 8u))
        .or_exit();
    Result(inline_align_xalloc_multi.is_inline()).or_exit();

    // Test `inline_unalign_alloc_multi()`.
    auto inline_unalign_alloc_multi =
        allocator.inline_unalign_alloc_multi<int4>(5).value();
    Result(inline_unalign_alloc_multi.is_inline()).or_exit();

    auto inline_unalign_alloc_multi_big =
        allocator.inline_unalign_alloc_multi<int4>(64).value();
    Result(!inline_unalign_alloc_multi_big.is_inline()).or_exit();

    // Test `inline_unalign_xalloc_multi()`.
    auto inline_unalign_xalloc_multi =
        allocator.inline_unalign_xalloc_multi<int4>(5);
    Result(inline_unalign_xalloc_multi.is_inline()).or_exit();

    auto inline_unalign_xalloc_multi_big =
        allocator.inline_unalign_xalloc_multi<int4>(64);
    Result(!inline_unalign_xalloc_multi_big.is_inline()).or_exit();

    // Always reset the allocator so that there are no alignment requirements
    // interfering with `nalloc()` tests. Specific allocator tests such as
    // `test_linear_allocator.cpp` check that in greater detail.

    // Test `nalloc()`.
    allocator.reset();
    ssize nalloc = allocator.nalloc<int4>().value();
    Result(nalloc == ssizeof<int4>()).value();

    // Test `xnalloc()`.
    allocator.reset();
    ssize xnalloc = allocator.xnalloc<int4>();
    Result(xnalloc == ssizeof<int4>()).value();

    cat::exit();
};
