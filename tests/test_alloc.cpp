#include <cat/bit>
#include <cat/linear_allocator>
#include <cat/page_allocator>

struct HugeObject {
    [[maybe_unused]] uint1 storage[cat::inline_buffer_size.raw + 1];
};

int4 global = 0;

struct NonTrivial {
    char storage;
    NonTrivial() {
        ++global;
    }
};

struct NonTrivialHugeObject {
    [[maybe_unused]] uint1 storage[cat::inline_buffer_size.raw];
    NonTrivialHugeObject() {
        ++global;
    }
};

auto main() -> int {
    // Initialize an allocator.
    cat::PageAllocator paging_allocator;
    paging_allocator.reset();
    int4* p_page = paging_allocator.p_alloc_multi<int4>(1_ki).or_exit();
    defer(paging_allocator.free_multi(p_page, 1_ki);)
    cat::LinearAllocator allocator(p_page, 4_ki - 64);

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
    // `test_linear_allocator.cpp` check that in greater cat::detail.

    // Test `nalloc()`.
    allocator.reset();
    ssize nalloc = allocator.nalloc<int4>().value();
    Result(nalloc == ssizeof<int4>()).or_exit();

    // Test `xnalloc()`.
    allocator.reset();
    ssize xnalloc = allocator.xnalloc<int4>();
    Result(xnalloc == ssizeof<int4>()).or_exit();

    // Test `nalloc_multi()`.
    allocator.reset();
    ssize nalloc_multi = allocator.nalloc_multi<int4>(5).value();
    Result(nalloc_multi == (ssizeof<int4>() * 5)).or_exit();

    // Test `xnalloc_multi()`.
    allocator.reset();
    ssize xnalloc_multi = allocator.xnalloc_multi<int4>(5);
    Result(xnalloc_multi == (ssizeof<int4>() * 5)).or_exit();

    // Test `align_nalloc()`.
    allocator.reset();
    ssize align_nalloc = allocator.align_nalloc<int4>(4u).value();
    Result(align_nalloc == ssizeof<int4>()).or_exit();

    // Test `align_xnalloc()`.
    allocator.reset();
    ssize align_xnalloc = allocator.align_xnalloc<int4>(4u);
    Result(align_xnalloc == ssizeof<int4>()).or_exit();

    // Test `align_nalloc_multi()`.
    allocator.reset();
    ssize align_nalloc_multi =
        allocator.align_nalloc_multi<int4>(4u, 5).value();
    Result(align_nalloc_multi == (ssizeof<int4>() * 5)).or_exit();

    // Test `align_xnalloc_multi()`.
    allocator.reset();
    ssize align_xnalloc_multi = allocator.align_xnalloc_multi<int4>(4u, 5);
    Result(align_xnalloc_multi == (ssizeof<int4>() * 5)).or_exit();

    // Test `unalign_nalloc()`.
    allocator.reset();
    ssize unalign_nalloc = allocator.unalign_nalloc<int4>().value();
    Result(unalign_nalloc == ssizeof<int4>()).or_exit();

    // Test `unalign_xnalloc()`.
    allocator.reset();
    ssize unalign_xnalloc = allocator.unalign_xnalloc<int4>();
    Result(unalign_xnalloc == ssizeof<int4>()).or_exit();

    // Test `unalign_nalloc_multi()`.
    allocator.reset();
    ssize unalign_nalloc_multi =
        allocator.unalign_nalloc_multi<int4>(5).value();
    Result(unalign_nalloc_multi == (ssizeof<int4>() * 5)).or_exit();

    // Test `unalign_xnalloc_multi()`.
    allocator.reset();
    ssize unalign_xnalloc_multi = allocator.unalign_xnalloc_multi<int4>(5);
    Result(unalign_xnalloc_multi == (ssizeof<int4>() * 5)).or_exit();

    // Test `inline_nalloc()`.
    allocator.reset();
    ssize inline_nalloc = allocator.inline_nalloc<int4>().value();
    Result(inline_nalloc == cat::inline_buffer_size).or_exit();
    ssize inline_nalloc_big = allocator.inline_nalloc<HugeObject>().value();
    Result(inline_nalloc_big == 257).or_exit();

    // Test `inline_xnalloc()`.
    allocator.reset();
    ssize inline_xnalloc = allocator.inline_xnalloc<int4>();
    Result(inline_xnalloc == cat::inline_buffer_size).or_exit();
    ssize inline_xnalloc_big = allocator.inline_xnalloc<HugeObject>();
    Result(inline_xnalloc_big == 257).or_exit();

    // Test `inline_nalloc_multi()`.
    allocator.reset();
    ssize inline_nalloc_multi = allocator.inline_nalloc_multi<int4>(5).value();
    Result(inline_nalloc_multi == cat::inline_buffer_size).or_exit();
    ssize inline_nalloc_multi_big =
        allocator.inline_nalloc_multi<HugeObject>(2).value();
    Result(inline_nalloc_multi_big == (257 * 2)).or_exit();

    // Test `inline_xnalloc_multi()`.
    allocator.reset();
    ssize inline_xnalloc_multi = allocator.inline_xnalloc_multi<int4>(5);
    Result(inline_xnalloc_multi == cat::inline_buffer_size).or_exit();
    ssize inline_xnalloc_multi_big =
        allocator.inline_xnalloc_multi<HugeObject>(2);
    Result(inline_xnalloc_multi_big == (257 * 2)).or_exit();

    // Test `inline_align_nalloc()`.
    allocator.reset();
    ssize inline_align_nalloc = allocator.inline_align_nalloc<int4>(4u).value();
    Result(inline_align_nalloc == cat::inline_buffer_size).or_exit();
    ssize inline_align_nalloc_big =
        allocator.inline_align_nalloc<HugeObject>(1u).value();
    Result(inline_align_nalloc_big == 257).or_exit();

    // Test `inline_align_xnalloc()`.
    allocator.reset();
    ssize inline_align_xnalloc = allocator.inline_align_xnalloc<int4>(4u);
    Result(inline_align_xnalloc == cat::inline_buffer_size).or_exit();
    ssize inline_align_xnalloc_big =
        allocator.inline_align_xnalloc<HugeObject>(1u);
    Result(inline_align_xnalloc_big == 257).or_exit();

    // Test `inline_unalign_nalloc()`.
    allocator.reset();
    ssize inline_unalign_nalloc =
        allocator.inline_unalign_nalloc<int4>().value();
    Result(inline_unalign_nalloc == cat::inline_buffer_size).or_exit();
    ssize inline_unalign_nalloc_big =
        allocator.inline_unalign_nalloc<HugeObject>().value();
    Result(inline_unalign_nalloc_big == 257).or_exit();

    // Test `inline_unalign_xnalloc()`.
    allocator.reset();
    ssize inline_unalign_xnalloc = allocator.inline_unalign_xnalloc<int4>();
    Result(inline_unalign_xnalloc == cat::inline_buffer_size).or_exit();
    ssize inline_unalign_xnalloc_big =
        allocator.inline_unalign_xnalloc<HugeObject>();
    Result(inline_unalign_xnalloc_big == 257).or_exit();

    // Test `inline_align_nalloc_multi()`.
    allocator.reset();
    ssize inline_align_nalloc_multi =
        allocator.inline_align_nalloc_multi<int4>(4u, 5).value();
    Result(inline_align_nalloc_multi == cat::inline_buffer_size).or_exit();
    ssize inline_align_nalloc_multi_big =
        allocator.inline_align_nalloc_multi<HugeObject>(1u, 2).value();
    Result(inline_align_nalloc_multi_big == (257 * 2)).or_exit();

    // Test `inline_align_xnalloc_multi()`.
    allocator.reset();
    ssize inline_align_xnalloc_multi =
        allocator.inline_align_xnalloc_multi<int4>(4u, 5);
    Result(inline_align_xnalloc_multi == cat::inline_buffer_size).or_exit();
    ssize inline_align_xnalloc_multi_big =
        allocator.inline_align_xnalloc_multi<HugeObject>(1u, 2);
    Result(inline_align_xnalloc_multi_big == (257 * 2)).or_exit();

    // Test `inline_unalign_nalloc_multi()`.
    allocator.reset();
    ssize inline_unalign_nalloc_multi =
        allocator.inline_unalign_nalloc_multi<int4>(5).value();
    Result(inline_unalign_nalloc_multi == cat::inline_buffer_size).or_exit();
    ssize inline_unalign_nalloc_multi_big =
        allocator.inline_unalign_nalloc_multi<HugeObject>(2).value();
    Result(inline_unalign_nalloc_multi_big == (257 * 2)).or_exit();

    // Test `inline_unalign_xnalloc_multi()`.
    allocator.reset();
    ssize inline_unalign_xnalloc_multi =
        allocator.inline_unalign_xnalloc_multi<int4>(5);
    Result(inline_unalign_xnalloc_multi == cat::inline_buffer_size).or_exit();
    ssize inline_unalign_xnalloc_multi_big =
        allocator.inline_unalign_xnalloc_multi<HugeObject>(2);
    Result(inline_unalign_xnalloc_multi_big == (257 * 2)).or_exit();

    // Test `salloc()`.
    _ = allocator.salloc<int4>().value();
    allocator.reset();
    _ = allocator.alloc<cat::Byte>();  // Offset linear allocator by 1 byte.
    auto [salloc, salloc_size] = allocator.salloc<int4>(1).value();
    Result(allocator.get(salloc) == 1).or_exit();
    Result(salloc_size == 7).or_exit();
    global = 0;
    _ = allocator.salloc<NonTrivial>();
    Result(global == 1).or_exit();

    // Test `xsalloc()`.
    _ = allocator.xsalloc<int4>();
    allocator.reset();
    _ = allocator.alloc<cat::Byte>();  // Offset linear allocator by 1 byte.
    auto [xsalloc, xsalloc_size] = allocator.xsalloc<int4>(1);
    Result(allocator.get(xsalloc) == 1).or_exit();
    Result(xsalloc_size == 7).or_exit();
    global = 0;
    _ = allocator.xsalloc<NonTrivial>();
    Result(global == 1).or_exit();

    // Test `p_salloc()`.
    _ = allocator.p_salloc<int4>().value();
    allocator.reset();
    _ = allocator.alloc<cat::Byte>();  // Offset linear allocator by 1 byte.
    auto [p_salloc, p_salloc_size] = allocator.p_salloc<int4>(1).value();
    Result(*p_salloc == 1).or_exit();
    Result(p_salloc_size == 7).or_exit();
    global = 0;
    _ = allocator.p_salloc<NonTrivial>();
    Result(global == 1).or_exit();

    // Test `p_xsalloc()`.
    _ = allocator.p_xsalloc<int4>();
    allocator.reset();
    _ = allocator.alloc<cat::Byte>();  // Offset linear allocator by 1 byte.
    auto [p_xsalloc, p_xsalloc_size] = allocator.p_xsalloc<int4>(1);
    Result(*p_xsalloc == 1).or_exit();
    Result(p_xsalloc_size == 7).or_exit();
    global = 0;
    _ = allocator.p_xsalloc<NonTrivial>();
    Result(global == 1).or_exit();

    // Test `salloc_multi()`.
    allocator.reset();
    _ = allocator.alloc<cat::Byte>();  // Offset linear allocator by 1 byte.
    auto [salloc_multi, salloc_multi_size] =
        allocator.salloc_multi<int4>(5).value();
    Result(salloc_multi.size() == 5).or_exit();
    Result(salloc_multi_size == 23).or_exit();
    Result(salloc_multi.raw_size() == 20).or_exit();

    global = 0;
    _ = allocator.salloc_multi<NonTrivial>(5);
    Result(global == 5).or_exit();

    // Test `xsalloc_multi()`.
    allocator.reset();
    _ = allocator.alloc<cat::Byte>();  // Offset linear allocator by 1 byte.
    auto [xsalloc_multi, xsalloc_multi_size] = allocator.xsalloc_multi<int4>(5);
    Result(xsalloc_multi.size() == 5).or_exit();
    Result(xsalloc_multi_size == 23).or_exit();
    Result(xsalloc_multi.raw_size() == 20).or_exit();

    global = 0;
    _ = allocator.xsalloc_multi<NonTrivial>(5);
    Result(global == 5).or_exit();

    // Test `p_salloc_multi()`.
    allocator.reset();
    _ = allocator.alloc<cat::Byte>();  // Offset linear allocator by 1 byte.
    auto [p_salloc_multi, p_salloc_multi_size] =
        allocator.p_salloc_multi<int4>(5).value();
    Result(p_salloc_multi_size == 23).or_exit();

    global = 0;
    _ = allocator.p_salloc_multi<NonTrivial>(5);
    Result(global == 5).or_exit();

    // Test `p_xsalloc_multi()`.
    allocator.reset();
    _ = allocator.alloc<cat::Byte>();  // Offset linear allocator by 1 byte.
    auto [p_xsalloc_multi, p_xsalloc_multi_size] =
        allocator.p_xsalloc_multi<int4>(5);
    Result(p_xsalloc_multi_size == 23).or_exit();

    global = 0;
    _ = allocator.p_xsalloc_multi<NonTrivial>(5);
    Result(global == 5).or_exit();

    // Test `align_salloc()`.
    _ = allocator.align_salloc<int4>(8u);
    allocator.reset();
    auto [align_salloc, align_salloc_size] =
        allocator.align_salloc<int4>(8u, 1).value();
    Result(allocator.get(align_salloc) == 1).or_exit();
    Result(align_salloc_size == 8).or_exit();
    Result(cat::is_aligned(&allocator.get(align_salloc), 8u)).or_exit();

    global = 0;
    _ = allocator.align_salloc<NonTrivial>(8u);
    Result(global == 1).or_exit();

    // Test `align_xsalloc()`.
    _ = allocator.align_xsalloc<int4>(8u);
    allocator.reset();
    auto [align_xsalloc, align_xsalloc_size] =
        allocator.align_xsalloc<int4>(8u, 1);
    Result(allocator.get(align_xsalloc) == 1).or_exit();
    Result(align_xsalloc_size == 8).or_exit();
    Result(cat::is_aligned(&allocator.get(align_xsalloc), 8u)).or_exit();

    global = 0;
    _ = allocator.align_xsalloc<NonTrivial>(8u);
    Result(global == 1).or_exit();

    // Test `p_align_salloc()`.
    _ = allocator.p_align_salloc<int4>(8u);
    allocator.reset();
    auto [p_align_salloc, p_align_salloc_size] =
        allocator.p_align_salloc<int4>(8u, 1).value();
    Result(*p_align_salloc == 1).or_exit();
    Result(p_align_salloc_size == 8).or_exit();
    Result(cat::is_aligned(p_align_salloc, 8u)).or_exit();

    global = 0;
    _ = allocator.p_align_salloc<NonTrivial>(8u);
    Result(global == 1).or_exit();

    // Test `p_align_xsalloc()`.
    _ = allocator.p_align_xsalloc<int4>(8u);
    allocator.reset();
    auto [p_align_xsalloc, p_align_xsalloc_size] =
        allocator.p_align_xsalloc<int4>(8u, 1);
    Result(*p_align_xsalloc == 1).or_exit();
    Result(p_align_xsalloc_size == 8).or_exit();
    Result(cat::is_aligned(p_align_xsalloc, 8u)).or_exit();

    global = 0;
    _ = allocator.p_align_xsalloc<NonTrivial>(8u);
    Result(global == 1).or_exit();

    // Test `unalign_salloc()`.
    _ = allocator.unalign_salloc<int1>();
    allocator.reset();
    auto [unalign_salloc, unalign_salloc_size] =
        allocator.unalign_salloc<int1>(1).value();
    Result(allocator.get(unalign_salloc) == 1).or_exit();
    Result(unalign_salloc_size == 1).or_exit();

    global = 0;
    _ = allocator.unalign_salloc<NonTrivial>();
    Result(global == 1).or_exit();
    // Test `unalign_xsalloc()`.
    _ = allocator.unalign_xsalloc<int1>();
    allocator.reset();
    auto [unalign_xsalloc, unalign_xsalloc_size] =
        allocator.unalign_xsalloc<int1>(1);
    Result(allocator.get(unalign_xsalloc) == 1).or_exit();
    Result(unalign_xsalloc_size == 1).or_exit();

    global = 0;
    _ = allocator.unalign_xsalloc<NonTrivial>();
    Result(global == 1).or_exit();

    // Test `p_unalign_salloc()`.
    _ = allocator.p_unalign_salloc<int1>();
    allocator.reset();
    auto [p_unalign_salloc, p_unalign_salloc_size] =
        allocator.p_unalign_salloc<int1>(1).value();
    Result(*p_unalign_salloc == 1).or_exit();
    Result(p_unalign_salloc_size == 1).or_exit();

    global = 0;
    _ = allocator.p_unalign_salloc<NonTrivial>();
    Result(global == 1).or_exit();

    // Test `p_unalign_xsalloc()`.
    _ = allocator.p_unalign_xsalloc<int1>();
    allocator.reset();
    auto [p_unalign_xsalloc, p_unalign_xsalloc_size] =
        allocator.p_unalign_xsalloc<int1>(1);
    Result(*p_unalign_xsalloc == 1).or_exit();
    Result(p_unalign_xsalloc_size == 1).or_exit();

    global = 0;
    _ = allocator.p_unalign_xsalloc<NonTrivial>();
    Result(global == 1).or_exit();

    // Test `align_salloc_multi()`.
    allocator.reset();
    auto [align_salloc_multi, align_salloc_multi_size] =
        allocator.align_salloc_multi<int4>(8u, 5).value();
    Result(align_salloc_multi_size == 24).or_exit();
    Result(cat::is_aligned(allocator.get(align_salloc_multi).p_data(), 8u))
        .or_exit();

    global = 0;
    _ = allocator.align_salloc_multi<NonTrivial>(8u, 5);
    Result(global == 5).or_exit();

    // Test `align_xsalloc_multi()`.
    allocator.reset();
    auto [align_xsalloc_multi, align_xsalloc_multi_size] =
        allocator.align_xsalloc_multi<int4>(8u, 5);
    Result(align_xsalloc_multi_size == 24).or_exit();
    Result(cat::is_aligned(allocator.get(align_xsalloc_multi).p_data(), 8u))
        .or_exit();

    global = 0;
    _ = allocator.align_xsalloc_multi<NonTrivial>(8u, 5);
    Result(global == 5).or_exit();
    // Test `p_align_salloc_multi()`.
    allocator.reset();
    auto [p_align_salloc_multi, p_align_salloc_multi_size] =
        allocator.p_align_salloc_multi<int4>(8u, 5).value();
    Result(p_align_salloc_multi_size == 24).or_exit();
    Result(cat::is_aligned(p_align_salloc_multi, 8u)).or_exit();

    global = 0;
    _ = allocator.p_align_salloc_multi<NonTrivial>(8u, 5);
    Result(global == 5).or_exit();

    // Test `p_align_xsalloc_multi()`.
    allocator.reset();
    auto [p_align_xsalloc_multi, p_align_xsalloc_multi_size] =
        allocator.p_align_xsalloc_multi<int4>(8u, 5);
    Result(p_align_xsalloc_multi_size == 24).or_exit();
    Result(cat::is_aligned(p_align_xsalloc_multi, 8u)).or_exit();

    global = 0;
    _ = allocator.p_align_xsalloc_multi<NonTrivial>(8u, 5);
    Result(global == 5).or_exit();

    // Test `unalign_salloc_multi()`.
    allocator.reset();
    auto [unalign_salloc_multi, unalign_salloc_multi_size] =
        allocator.unalign_salloc_multi<int1>(5).value();
    Result(unalign_salloc_multi_size == 5).or_exit();

    global = 0;
    _ = allocator.unalign_salloc_multi<NonTrivial>(5);
    Result(global == 5).or_exit();

    // Test `unalign_xsalloc_multi()`.
    allocator.reset();
    auto [unalign_xsalloc_multi, unalign_xsalloc_multi_size] =
        allocator.unalign_xsalloc_multi<int1>(5);
    Result(unalign_xsalloc_multi_size == 5).or_exit();

    global = 0;
    _ = allocator.unalign_xsalloc_multi<NonTrivial>(5);
    Result(global == 5).or_exit();

    // Test `p_unalign_salloc_multi()`.
    allocator.reset();
    auto [p_unalign_salloc_multi, p_unalign_salloc_multi_size] =
        allocator.p_unalign_salloc_multi<int1>(5).value();
    Result(p_unalign_salloc_multi_size == 5).or_exit();

    global = 0;
    _ = allocator.p_unalign_salloc_multi<NonTrivial>(5);
    Result(global == 5).or_exit();

    // Test `p_unalign_xsalloc_multi()`.
    allocator.reset();
    auto [p_unalign_xsalloc_multi, p_unalign_xsalloc_multi_size] =
        allocator.p_unalign_xsalloc_multi<int1>(5);
    Result(p_unalign_xsalloc_multi_size == 5).or_exit();

    global = 0;
    _ = allocator.p_unalign_xsalloc_multi<NonTrivial>(5);
    Result(global == 5).or_exit();

    // Test `inline_salloc()`.
    allocator.reset();
    auto [inline_salloc, inline_salloc_size] =
        allocator.inline_salloc<int4>(1).value();
    Result(allocator.get(inline_salloc) == 1).or_exit();
    Result(inline_salloc_size == cat::inline_buffer_size);
    Result(inline_salloc.is_inline()).or_exit();

    auto [inline_salloc_big, inline_salloc_size_big] =
        allocator.inline_salloc<HugeObject>().value();
    Result(inline_salloc_size == ssizeof<HugeObject>());
    Result(!inline_salloc_big.is_inline()).or_exit();

    // Test `inline_xsalloc()`.
    allocator.reset();
    auto [inline_xsalloc, inline_xsalloc_size] =
        allocator.inline_xsalloc<int4>(1);
    Result(allocator.get(inline_xsalloc) == 1).or_exit();
    Result(inline_xsalloc_size == cat::inline_buffer_size);
    Result(inline_xsalloc.is_inline()).or_exit();

    auto [inline_xsalloc_big, inline_xsalloc_size_big] =
        allocator.inline_xsalloc<HugeObject>();
    Result(inline_xsalloc_size == ssizeof<HugeObject>());
    Result(!inline_xsalloc_big.is_inline()).or_exit();

    // Test `inline_salloc_multi()`.
    allocator.reset();
    auto [inline_salloc_multi, inline_salloc_multi_size] =
        allocator.inline_salloc_multi<int4>(5).value();
    Result(inline_salloc_multi_size == cat::inline_buffer_size);
    Result(inline_salloc_multi.is_inline()).or_exit();

    auto [inline_salloc_multi_big, inline_salloc_multi_size_big] =
        allocator.inline_salloc_multi<HugeObject>(5).value();
    Result(inline_salloc_multi_size == ssizeof<HugeObject>() * 5);
    Result(!inline_salloc_multi_big.is_inline()).or_exit();

    // Test `inline_xsalloc_multi()`.
    allocator.reset();
    auto [inline_xsalloc_multi, inline_xsalloc_multi_size] =
        allocator.inline_xsalloc_multi<int4>(5);
    Result(inline_xsalloc_multi_size == cat::inline_buffer_size);
    Result(inline_xsalloc_multi.is_inline()).or_exit();

    auto [inline_xsalloc_multi_big, inline_xsalloc_multi_size_big] =
        allocator.inline_xsalloc_multi<HugeObject>(5);
    Result(inline_xsalloc_multi_size == ssizeof<HugeObject>() * 5);
    Result(!inline_xsalloc_multi_big.is_inline()).or_exit();

    // Test `inline_align_salloc()`.
    allocator.reset();
    auto [inline_align_salloc, inline_align_salloc_size] =
        allocator.inline_align_salloc<int4>(8u, 1).value();
    Result(allocator.get(inline_align_salloc) == 1).or_exit();
    Result(inline_align_salloc_size == cat::inline_buffer_size);
    Result(inline_align_salloc.is_inline()).or_exit();

    auto [inline_align_salloc_big, inline_align_salloc_size_big] =
        allocator.inline_align_salloc<HugeObject>(8u).value();
    Result(inline_align_salloc_size == ssizeof<HugeObject>());
    Result(!inline_align_salloc_big.is_inline()).or_exit();

    // Test `inline_align_xsalloc()`.
    allocator.reset();
    auto [inline_align_xsalloc, inline_align_xsalloc_size] =
        allocator.inline_align_xsalloc<int4>(8u, 1);
    Result(allocator.get(inline_align_xsalloc) == 1).or_exit();
    Result(inline_align_xsalloc_size == cat::inline_buffer_size);
    Result(inline_align_xsalloc.is_inline()).or_exit();

    auto [inline_align_xsalloc_big, inline_align_xsalloc_size_big] =
        allocator.inline_align_xsalloc<HugeObject>(8u);
    Result(inline_align_xsalloc_size == ssizeof<HugeObject>());
    Result(!inline_align_xsalloc_big.is_inline()).or_exit();

    // Test `inline_unalign_salloc()`.
    allocator.reset();
    auto [inline_unalign_salloc, inline_unalign_salloc_size] =
        allocator.inline_unalign_salloc<int4>(1).value();
    Result(allocator.get(inline_unalign_salloc) == 1).or_exit();
    Result(inline_unalign_salloc_size == cat::inline_buffer_size);
    Result(inline_unalign_salloc.is_inline()).or_exit();

    auto [inline_unalign_salloc_big, inline_unalign_salloc_size_big] =
        allocator.inline_unalign_salloc<HugeObject>().value();
    Result(inline_unalign_salloc_size == ssizeof<HugeObject>());
    Result(!inline_unalign_salloc_big.is_inline()).or_exit();

    // Test `inline_unalign_xsalloc()`.
    allocator.reset();
    auto [inline_unalign_xsalloc, inline_unalign_xsalloc_size] =
        allocator.inline_unalign_xsalloc<int4>(1);
    Result(allocator.get(inline_unalign_xsalloc) == 1).or_exit();
    Result(inline_unalign_xsalloc_size == cat::inline_buffer_size);
    Result(inline_unalign_xsalloc.is_inline()).or_exit();

    auto [inline_unalign_xsalloc_big, inline_unalign_xsalloc_size_big] =
        allocator.inline_unalign_xsalloc<HugeObject>();
    Result(inline_unalign_xsalloc_size == ssizeof<HugeObject>());
    Result(!inline_unalign_xsalloc_big.is_inline()).or_exit();

    // Test `inline_align_salloc_multi()`.
    allocator.reset();
    auto [inline_align_salloc_multi, inline_align_salloc_multi_size] =
        allocator.inline_align_salloc_multi<int4>(8u, 5).value();
    Result(inline_align_salloc_multi_size == cat::inline_buffer_size);
    Result(inline_align_salloc_multi.is_inline()).or_exit();

    auto [inline_align_salloc_multi_big, inline_align_salloc_multi_size_big] =
        allocator.inline_align_salloc_multi<HugeObject>(8u, 5).value();
    Result(inline_align_salloc_multi_size == ssizeof<HugeObject>());
    Result(!inline_align_salloc_multi_big.is_inline()).or_exit();

    // Test `inline_align_xsalloc_multi()`.
    allocator.reset();
    auto [inline_align_xsalloc_multi, inline_align_xsalloc_multi_size] =
        allocator.inline_align_xsalloc_multi<int4>(8u, 5);
    Result(inline_align_xsalloc_multi_size == cat::inline_buffer_size);
    Result(inline_align_xsalloc_multi.is_inline()).or_exit();

    auto [inline_align_xsalloc_multi_big, inline_align_xsalloc_multi_size_big] =
        allocator.inline_align_xsalloc_multi<HugeObject>(8u, 5);
    Result(inline_align_xsalloc_multi_size == ssizeof<HugeObject>());
    Result(!inline_align_xsalloc_multi_big.is_inline()).or_exit();

    // Test `inline_unalign_salloc_multi()`.
    allocator.reset();
    auto [inline_unalign_salloc_multi, inline_unalign_salloc_multi_size] =
        allocator.inline_unalign_salloc_multi<int4>(5).value();
    Result(inline_unalign_salloc_multi_size == cat::inline_buffer_size);
    Result(inline_unalign_salloc_multi.is_inline()).or_exit();

    auto [inline_unalign_salloc_multi_big,
          inline_unalign_salloc_multi_size_big] =
        allocator.inline_unalign_salloc_multi<HugeObject>(5).value();
    Result(inline_unalign_salloc_multi_size == ssizeof<HugeObject>());
    Result(!inline_unalign_salloc_multi_big.is_inline()).or_exit();

    // Test `inline_unalign_xsalloc_multi()`.
    allocator.reset();
    auto [inline_unalign_xsalloc_multi, inline_unalign_xsalloc_multi_size] =
        allocator.inline_unalign_xsalloc_multi<int4>(5);
    Result(inline_unalign_xsalloc_multi_size == cat::inline_buffer_size);
    Result(inline_unalign_xsalloc_multi.is_inline()).or_exit();

    auto [inline_unalign_xsalloc_multi_big,
          inline_unalign_xsalloc_multi_size_big] =
        allocator.inline_unalign_xsalloc_multi<HugeObject>(5);
    Result(inline_unalign_xsalloc_multi_size == ssizeof<HugeObject>());
    Result(!inline_unalign_xsalloc_multi_big.is_inline()).or_exit();

    // TODO: Test `calloc()` family more comprehensively.

    // Test `calloc()`.
    _ = allocator.calloc<int4>().value();
    _ = allocator.calloc<int4>(1).value();

    // Test `xcalloc()`.
    _ = allocator.xcalloc<int4>();
    _ = allocator.xcalloc<int4>(1);

    // Test `p_calloc()`.
    _ = allocator.calloc<int4>().value();
    _ = allocator.calloc<int4>(1).value();

    // Test `p_xcalloc()`.
    _ = allocator.p_xcalloc<int4>();
    _ = allocator.p_xcalloc<int4>(1);

    // Test `align_calloc()`.
    _ = allocator.align_calloc<int4>(8u).value();
    _ = allocator.align_calloc<int4>(8u, 1).value();

    // Test `align_xcalloc()`.
    _ = allocator.align_xcalloc<int4>(8u);
    _ = allocator.align_xcalloc<int4>(8u, 1);

    // Test `p_align_calloc()`.
    _ = allocator.align_calloc<int4>(8u).value();
    _ = allocator.align_calloc<int4>(8u, 1).value();

    // Test `p_align_xcalloc()`.
    _ = allocator.p_align_xcalloc<int4>(8u);
    _ = allocator.p_align_xcalloc<int4>(8u, 1);

    // Test `unalign_calloc()`.
    _ = allocator.unalign_calloc<int1>().value();
    _ = allocator.unalign_calloc<int1>(1).value();

    // Test `unalign_xcalloc()`.
    _ = allocator.unalign_xcalloc<int1>();
    _ = allocator.unalign_xcalloc<int1>(1);

    // Test `p_unalign_calloc()`.
    _ = allocator.unalign_calloc<int1>().value();
    _ = allocator.unalign_calloc<int1>(1).value();

    // Test `p_unalign_xcalloc()`.
    _ = allocator.p_unalign_xcalloc<int1>();
    _ = allocator.p_unalign_xcalloc<int1>(1);

    // Test `inline_calloc()`.
    _ = allocator.inline_calloc<int4>().value();
    _ = allocator.inline_calloc<int4>(1).value();

    // Test `inline_xcalloc()`.
    _ = allocator.inline_xcalloc<int4>();
    _ = allocator.inline_xcalloc<int4>(1);

    // Test `inline_align_calloc()`.
    _ = allocator.inline_align_calloc<int4>(8u).value();
    _ = allocator.inline_align_calloc<int4>(8u, 1).value();

    // Test `inline_align_xcalloc()`.
    _ = allocator.inline_align_xcalloc<int4>(8u);
    _ = allocator.inline_align_xcalloc<int4>(8u, 1);

    // Test `inline_unalign_calloc()`.
    _ = allocator.inline_unalign_calloc<int4>().value();
    _ = allocator.inline_unalign_calloc<int4>(1).value();

    // Test `inline_unalign_xcalloc()`.
    _ = allocator.inline_unalign_xcalloc<int4>();
    _ = allocator.inline_unalign_xcalloc<int4>(1);

    // Test `xscalloc()`.
    _ = allocator.xscalloc<int4>().first();
    _ = allocator.xscalloc<int4>(1).first();

    // Test `p_scalloc()`.
    _ = allocator.scalloc<int4>().value().first();
    _ = allocator.scalloc<int4>(1).value().first();

    // Test `p_xscalloc()`.
    _ = allocator.p_xscalloc<int4>().first();
    _ = allocator.p_xscalloc<int4>(1).first();

    // Test `align_scalloc()`.
    _ = allocator.align_scalloc<int4>(8u).value().first();
    _ = allocator.align_scalloc<int4>(8u, 1).value().first();

    // Test `align_xscalloc()`.
    _ = allocator.align_xscalloc<int4>(8u).first();
    _ = allocator.align_xscalloc<int4>(8u, 1).first();

    // Test `p_align_scalloc()`.
    _ = allocator.align_scalloc<int4>(8u).value().first();
    _ = allocator.align_scalloc<int4>(8u, 1).value().first();

    // Test `p_align_xscalloc()`.
    _ = allocator.p_align_xscalloc<int4>(8u).first();
    _ = allocator.p_align_xscalloc<int4>(8u, 1).first();

    // Test `unalign_scalloc()`.
    _ = allocator.unalign_scalloc<int1>().value().first();
    _ = allocator.unalign_scalloc<int1>(1).value().first();

    // Test `unalign_xscalloc()`.
    _ = allocator.unalign_xscalloc<int1>().first();
    _ = allocator.unalign_xscalloc<int1>(1).first();

    // Test `p_unalign_scalloc()`.
    _ = allocator.unalign_scalloc<int1>().value().first();
    _ = allocator.unalign_scalloc<int1>(1).value().first();

    // Test `p_unalign_xscalloc()`.
    _ = allocator.p_unalign_xscalloc<int1>().first();
    _ = allocator.p_unalign_xscalloc<int1>(1).first();

    // Test `inline_scalloc()`.
    _ = allocator.inline_scalloc<int4>().value().first();
    _ = allocator.inline_scalloc<int4>(1).value().first();

    // Test `inline_xscalloc()`.
    _ = allocator.inline_xscalloc<int4>().first();
    _ = allocator.inline_xscalloc<int4>(1).first();

    // Test `inline_align_scalloc()`.
    _ = allocator.inline_align_scalloc<int4>(8u).value().first();
    _ = allocator.inline_align_scalloc<int4>(8u, 1).value().first();

    // Test `inline_align_xscalloc()`.
    _ = allocator.inline_align_xscalloc<int4>(8u).first();
    _ = allocator.inline_align_xscalloc<int4>(8u, 1).first();

    // Test `inline_unalign_scalloc()`.
    _ = allocator.inline_unalign_scalloc<int4>().value().first();
    _ = allocator.inline_unalign_scalloc<int4>(1).value().first();

    // Test `inline_unalign_xscalloc()`.
    _ = allocator.inline_unalign_xscalloc<int4>().first();
    _ = allocator.inline_unalign_xscalloc<int4>(1).first();

    // TODO: Test `realloc()` family more comprehensively.
    allocator.reset();

    // Test `realloc()`.
    auto realloc_1 = allocator.alloc<int4>(1).value();
    auto realloc_2 = allocator.alloc<int4>(2).value();
    Result(allocator.get(realloc_1) == 1).or_exit();
    Result(allocator.get(realloc_2) == 2).or_exit();
    realloc_1 = allocator.realloc(realloc_2).value();
    Result(allocator.get(realloc_1) == 2).or_exit();

    // Test `realloc_to()`.
    _ = allocator.realloc_to(allocator, alloc).value();

    // Test `p_realloc()`.
    auto p_realloc_1 = allocator.p_alloc<int4>(1).value();
    auto p_realloc_2 = allocator.p_alloc<int4>(2).value();
    Result(*p_realloc_1 == 1).or_exit();
    Result(*p_realloc_2 == 2).or_exit();
    p_realloc_1 = allocator.p_realloc(p_realloc_2).value();
    Result(*p_realloc_1 == 2).or_exit();

    // Test `p_realloc_to()`
    _ = allocator.p_realloc_to(allocator, p_alloc);

    // Test `xrealloc()`.
    _ = allocator.xrealloc(alloc);

    // Test `xrealloc_to()`.
    _ = allocator.xrealloc_to(allocator, alloc);

    // Test `p_xrealloc()`
    _ = allocator.p_xrealloc(p_alloc);

    // Test `p_xrealloc_to()`
    _ = allocator.p_xrealloc_to(allocator, p_alloc);

    // Test `align_realloc()`.
    _ = allocator.align_realloc(alloc, 8u).value();

    // Test `align_realloc_to()`.
    _ = allocator.align_realloc_to(allocator, alloc, 8u).value();

    // Test `align_xrealloc()`.
    _ = allocator.align_xrealloc(alloc, 8u);

    // Test `align_xrealloc_to()`.
    _ = allocator.align_xrealloc_to(allocator, alloc, 8u);

    // Test `unalign_realloc()`.
    _ = allocator.unalign_realloc(alloc).value();

    // Test `unalign_realloc_to()`.
    _ = allocator.unalign_realloc_to(allocator, alloc).value();

    // Test `unalign_xrealloc()`.
    _ = allocator.unalign_xrealloc(alloc);

    // Test `unalign_xrealloc_to()`.
    _ = allocator.unalign_xrealloc_to(allocator, alloc);

    // Test `align_realloc()`.
    _ = allocator.align_realloc(alloc, 8u).value();

    // Test `align_realloc_to()`.
    _ = allocator.align_realloc_to(allocator, alloc, 8u).value();

    // Test `align_xrealloc()`.
    _ = allocator.align_xrealloc(alloc, 8u);

    // Test `align_xrealloc_to()`.
    _ = allocator.align_xrealloc_to(allocator, alloc, 8u);

    // Test `unalign_realloc()`.
    _ = allocator.unalign_realloc(alloc).value();

    // Test `unalign_realloc_to()`.
    _ = allocator.unalign_realloc_to(allocator, alloc).value();

    // Test `unalign_xrealloc()`.
    _ = allocator.unalign_xrealloc(alloc);

    // Test `unalign_xrealloc_to()`.
    _ = allocator.unalign_xrealloc_to(allocator, alloc);

    // Test `p_align_realloc()`.
    _ = allocator.p_align_realloc(p_alloc, 8u).value();

    // Test `p_align_realloc_to()`.
    _ = allocator.p_align_realloc_to(allocator, p_alloc, 8u).value();

    // Test `p_align_xrealloc()`.
    _ = allocator.p_align_xrealloc(p_alloc, 8u);

    // Test `p_align_xrealloc_to()`.
    _ = allocator.p_align_xrealloc_to(allocator, p_alloc, 8u);

    // Test `p_unalign_realloc()`.
    _ = allocator.p_unalign_realloc(p_alloc).value();

    // Test `p_unalign_realloc_to()`.
    _ = allocator.p_unalign_realloc_to(allocator, p_alloc).value();

    // Test `p_unalign_xrealloc()`.
    _ = allocator.p_unalign_xrealloc(p_alloc);

    // Test `p_unalign_xrealloc_to()`.
    _ = allocator.p_unalign_xrealloc_to(allocator, p_alloc);

    // Test `realloc_multi()`.
    _ = allocator.realloc_multi(alloc, 10).value();

    // Test `realloc_multi_to()`.
    _ = allocator.realloc_multi_to(allocator, alloc, 10).value();

    // Test `p_realloc_multi()`.
    _ = allocator.p_realloc_multi(p_alloc, 5, 10).value();

    // Test `p_realloc_multi_to()`
    _ = allocator.p_realloc_multi_to(allocator, p_alloc, 5, 10).value();

    // Test `xrealloc_multi()`.
    _ = allocator.xrealloc_multi(alloc, 10);

    // Test `xrealloc_multi_to()`.
    _ = allocator.xrealloc_multi_to(allocator, alloc, 10);

    // Test `p_xrealloc_multi()`
    _ = allocator.p_xrealloc_multi(p_alloc, 5, 10);

    // Test `p_xrealloc_multi_to()`
    _ = allocator.p_xrealloc_multi_to(allocator, p_alloc, 5, 10);

    // Test `align_realloc_multi()`.
    _ = allocator.align_realloc_multi(alloc, 8u, 10).value();

    // Test `align_realloc_multi_to()`.
    _ = allocator.align_realloc_multi_to(allocator, alloc, 8u, 10).value();

    // Test `align_xrealloc_multi()`.
    _ = allocator.align_xrealloc_multi(alloc, 8u, 10);

    // Test `align_xrealloc_multi_to()`.
    _ = allocator.align_xrealloc_multi_to(allocator, alloc, 8u, 10);

    // Test `unalign_realloc_multi()`.
    _ = allocator.unalign_realloc_multi(alloc, 10).value();

    // Test `unalign_realloc_multi_to()`.
    _ = allocator.unalign_realloc_multi_to(allocator, alloc, 10).value();

    // Test `unalign_xrealloc_multi()`.
    _ = allocator.unalign_xrealloc_multi(alloc, 10);

    // Test `unalign_xrealloc_multi_to()`.
    _ = allocator.unalign_xrealloc_multi_to(allocator, alloc, 10);

    // Test `align_realloc_multi()`.
    _ = allocator.align_realloc_multi(alloc, 8u, 10).value();

    // Test `align_realloc_multi_to()`.
    _ = allocator.align_realloc_multi_to(allocator, alloc, 8u, 10).value();

    // Test `align_xrealloc_multi()`.
    _ = allocator.align_xrealloc_multi(alloc, 8u, 10);

    // Test `align_xrealloc_multi_to()`.
    _ = allocator.align_xrealloc_multi_to(allocator, alloc, 8u, 10);

    // Test `unalign_realloc_multi()`.
    _ = allocator.unalign_realloc_multi(alloc, 10).value();

    // Test `unalign_realloc_multi_to()`.
    _ = allocator.unalign_realloc_multi_to(allocator, alloc, 10).value();

    // Test `unalign_xrealloc_multi()`.
    _ = allocator.unalign_xrealloc_multi(alloc, 10);

    // Test `unalign_xrealloc_multi_to()`.
    _ = allocator.unalign_xrealloc_multi_to(allocator, alloc, 10);

    // Test `p_align_realloc_multi()`.
    _ = allocator.p_align_realloc_multi(p_alloc, 8u, 5, 10).value();

    // Test `p_align_realloc_multi_to()`.
    _ = allocator.p_align_realloc_multi_to(allocator, p_alloc, 8u, 5, 10)
            .value();

    // Test `p_align_xrealloc_multi()`.
    _ = allocator.p_align_xrealloc_multi(p_alloc, 8u, 5, 10);

    // Test `p_align_xrealloc_multi_to()`.
    _ = allocator.p_align_xrealloc_multi_to(allocator, p_alloc, 8u, 5, 10);

    // Test `p_unalign_realloc_multi()`.
    _ = allocator.p_unalign_realloc_multi(p_alloc, 5, 10).value();

    // Test `p_unalign_realloc_multi_to()`.
    _ = allocator.p_unalign_realloc_multi_to(allocator, p_alloc, 5, 10).value();

    // Test `p_unalign_xrealloc_multi()`.
    _ = allocator.p_unalign_xrealloc_multi(p_alloc, 5, 10);

    // Test `p_unalign_xrealloc_multi_to()`.
    _ = allocator.p_unalign_xrealloc_multi_to(allocator, p_alloc, 5, 10);

    // The allocator runs out of memory around here.
    allocator.reset();

    // Test `recalloc()`.
    _ = allocator.recalloc(alloc).value();

    // Test `recalloc_to()`.
    _ = allocator.recalloc_to(allocator, alloc).value();

    // Test `p_recalloc()`.
    _ = allocator.p_recalloc(p_alloc).value();

    // Test `p_recalloc_to()`
    _ = allocator.p_recalloc_to(allocator, p_alloc);

    // Test `xrecalloc()`.
    _ = allocator.xrecalloc(alloc);

    // Test `xrecalloc_to()`.
    _ = allocator.xrecalloc_to(allocator, alloc);

    // Test `p_xrecalloc()`
    _ = allocator.p_xrecalloc(p_alloc);

    // Test `p_xrecalloc_to()`
    _ = allocator.p_xrecalloc_to(allocator, p_alloc);

    // Test `align_recalloc()`.
    _ = allocator.align_recalloc(alloc, 8u).value();

    // Test `align_recalloc_to()`.
    _ = allocator.align_recalloc_to(allocator, alloc, 8u).value();

    // Test `align_xrecalloc()`.
    _ = allocator.align_xrecalloc(alloc, 8u);

    // Test `align_xrecalloc_to()`.
    _ = allocator.align_xrecalloc_to(allocator, alloc, 8u);

    // Test `unalign_recalloc()`.
    _ = allocator.unalign_recalloc(alloc).value();

    // Test `unalign_recalloc_to()`.
    _ = allocator.unalign_recalloc_to(allocator, alloc).value();

    // Test `unalign_xrecalloc()`.
    _ = allocator.unalign_xrecalloc(alloc);

    // Test `unalign_xrecalloc_to()`.
    _ = allocator.unalign_xrecalloc_to(allocator, alloc);

    // Test `align_recalloc()`.
    _ = allocator.align_recalloc(alloc, 8u).value();

    // Test `align_recalloc_to()`.
    _ = allocator.align_recalloc_to(allocator, alloc, 8u).value();

    // Test `align_xrecalloc()`.
    _ = allocator.align_xrecalloc(alloc, 8u);

    // Test `align_xrecalloc_to()`.
    _ = allocator.align_xrecalloc_to(allocator, alloc, 8u);

    // Test `unalign_recalloc()`.
    _ = allocator.unalign_recalloc(alloc).value();

    // Test `unalign_recalloc_to()`.
    _ = allocator.unalign_recalloc_to(allocator, alloc).value();

    // Test `unalign_xrecalloc()`.
    _ = allocator.unalign_xrecalloc(alloc);

    // Test `unalign_xrecalloc_to()`.
    _ = allocator.unalign_xrecalloc_to(allocator, alloc);

    // Test `p_align_recalloc()`.
    _ = allocator.p_align_recalloc(p_alloc, 8u).value();

    // Test `p_align_recalloc_to()`.
    _ = allocator.p_align_recalloc_to(allocator, p_alloc, 8u).value();

    // Test `p_align_xrecalloc()`.
    _ = allocator.p_align_xrecalloc(p_alloc, 8u);

    // Test `p_align_xrecalloc_to()`.
    _ = allocator.p_align_xrecalloc_to(allocator, p_alloc, 8u);

    // Test `p_unalign_recalloc()`.
    _ = allocator.p_unalign_recalloc(p_alloc).value();

    // Test `p_unalign_recalloc_to()`.
    _ = allocator.p_unalign_recalloc_to(allocator, p_alloc).value();

    // Test `p_unalign_xrecalloc()`.
    _ = allocator.p_unalign_xrecalloc(p_alloc);

    // Test `p_unalign_xrecalloc_to()`.
    _ = allocator.p_unalign_xrecalloc_to(allocator, p_alloc);

    // Test `recalloc_multi()`.
    _ = allocator.recalloc_multi(alloc, 10).value();

    // Test `recalloc_multi_to()`.
    _ = allocator.recalloc_multi_to(allocator, alloc, 10).value();

    // Test `p_recalloc_multi()`.
    _ = allocator.p_recalloc_multi(p_alloc, 5, 10).value();

    // Test `p_recalloc_multi_to()`
    _ = allocator.p_recalloc_multi_to(allocator, p_alloc, 5, 10).value();

    // Test `xrecalloc_multi()`.
    _ = allocator.xrecalloc_multi(alloc, 10);

    // Test `xrecalloc_multi_to()`.
    _ = allocator.xrecalloc_multi_to(allocator, alloc, 10);

    // Test `p_xrecalloc_multi()`
    _ = allocator.p_xrecalloc_multi(p_alloc, 5, 10);

    // Test `p_xrecalloc_multi_to()`
    _ = allocator.p_xrecalloc_multi_to(allocator, p_alloc, 5, 10);

    // Test `align_recalloc_multi()`.
    _ = allocator.align_recalloc_multi(alloc, 8u, 10).value();

    // Test `align_recalloc_multi_to()`.
    _ = allocator.align_recalloc_multi_to(allocator, alloc, 8u, 10).value();

    // Test `align_xrecalloc_multi()`.
    _ = allocator.align_xrecalloc_multi(alloc, 8u, 10);

    // Test `align_xrecalloc_multi_to()`.
    _ = allocator.align_xrecalloc_multi_to(allocator, alloc, 8u, 10);

    // Test `unalign_recalloc_multi()`.
    _ = allocator.unalign_recalloc_multi(alloc, 10).value();

    // Test `unalign_recalloc_multi_to()`.
    _ = allocator.unalign_recalloc_multi_to(allocator, alloc, 10).value();

    // Test `unalign_xrecalloc_multi()`.
    _ = allocator.unalign_xrecalloc_multi(alloc, 10);

    // Test `unalign_xrecalloc_multi_to()`.
    _ = allocator.unalign_xrecalloc_multi_to(allocator, alloc, 10);

    // Test `align_recalloc_multi()`.
    _ = allocator.align_recalloc_multi(alloc, 8u, 10).value();

    // Test `align_recalloc_multi_to()`.
    _ = allocator.align_recalloc_multi_to(allocator, alloc, 8u, 10).value();

    // Test `align_xrecalloc_multi()`.
    _ = allocator.align_xrecalloc_multi(alloc, 8u, 10);

    // Test `align_xrecalloc_multi_to()`.
    _ = allocator.align_xrecalloc_multi_to(allocator, alloc, 8u, 10);

    // Test `unalign_recalloc_multi()`.
    _ = allocator.unalign_recalloc_multi(alloc, 10).value();

    // Test `unalign_recalloc_multi_to()`.
    _ = allocator.unalign_recalloc_multi_to(allocator, alloc, 10).value();

    // Test `unalign_xrecalloc_multi()`.
    _ = allocator.unalign_xrecalloc_multi(alloc, 10);

    // Test `unalign_xrecalloc_multi_to()`.
    _ = allocator.unalign_xrecalloc_multi_to(allocator, alloc, 10);

    // Test `p_align_recalloc_multi()`.
    _ = allocator.p_align_recalloc_multi(p_alloc, 8u, 5, 10).value();

    // Test `p_align_recalloc_multi_to()`.
    _ = allocator.p_align_recalloc_multi_to(allocator, p_alloc, 8u, 5, 10)
            .value();

    // Test `p_align_xrecalloc_multi()`.
    _ = allocator.p_align_xrecalloc_multi(p_alloc, 8u, 5, 10);

    // Test `p_align_xrecalloc_multi_to()`.
    _ = allocator.p_align_xrecalloc_multi_to(allocator, p_alloc, 8u, 5, 10);

    // Test `p_unalign_recalloc_multi()`.
    _ = allocator.p_unalign_recalloc_multi(p_alloc, 5, 10).value();

    // Test `p_unalign_recalloc_multi_to()`.
    _ = allocator.p_unalign_recalloc_multi_to(allocator, p_alloc, 5, 10)
            .value();

    // Test `p_unalign_xrecalloc_multi()`.
    _ = allocator.p_unalign_xrecalloc_multi(p_alloc, 5, 10);

    // Test `p_unalign_xrecalloc_multi_to()`.
    _ = allocator.p_unalign_xrecalloc_multi_to(allocator, p_alloc, 5, 10);

    // Test `inline_realloc()`.
    _ = allocator.inline_realloc(alloc);

    // Test `inline_realloc_to()`.
    _ = allocator.inline_realloc_to(allocator, alloc).value();

    // Test `inline_xrealloc()`.
    _ = allocator.inline_xrealloc(alloc);

    // Test `inline_xrealloc_to()`.
    _ = allocator.inline_xrealloc_to(allocator, alloc);

    // Test `inline_align_realloc()`.
    _ = allocator.inline_align_realloc(alloc, 8u).value();

    // Test `inline_align_realloc_to()`.
    _ = allocator.inline_align_realloc_to(allocator, alloc, 8u).value();

    // Test `inline_align_xrealloc()`.
    _ = allocator.inline_align_xrealloc(alloc, 8u);

    // Test `inline_align_xrealloc_to()`.
    _ = allocator.inline_align_xrealloc_to(allocator, alloc, 8u);

    // Test `inline_unalign_realloc()`.
    _ = allocator.inline_unalign_realloc(alloc).value();

    // Test `inline_unalign_realloc_to()`.
    _ = allocator.inline_unalign_realloc_to(allocator, alloc).value();

    // Test `inline_unalign_xrealloc()`.
    _ = allocator.inline_unalign_xrealloc(alloc);

    // Test `inline_unalign_xrealloc_to()`.
    _ = allocator.inline_unalign_xrealloc_to(allocator, alloc);

    // Test `inline_align_realloc()`.
    _ = allocator.inline_align_realloc(alloc, 8u).value();

    // Test `inline_align_realloc_to()`.
    _ = allocator.inline_align_realloc_to(allocator, alloc, 8u).value();

    // Test `inline_align_xrealloc()`.
    _ = allocator.inline_align_xrealloc(alloc, 8u);

    // Test `inline_align_xrealloc_to()`.
    _ = allocator.inline_align_xrealloc_to(allocator, alloc, 8u);

    // Test `inline_unalign_realloc()`.
    _ = allocator.inline_unalign_realloc(alloc).value();

    // Test `inline_unalign_realloc_to()`.
    _ = allocator.inline_unalign_realloc_to(allocator, alloc).value();

    // Test `inline_unalign_xrealloc()`.
    _ = allocator.inline_unalign_xrealloc(alloc);

    // Test `inline_unalign_xrealloc_to()`.
    _ = allocator.inline_unalign_xrealloc_to(allocator, alloc);

    // Test `inline_realloc_multi()`.
    _ = allocator.inline_realloc_multi(alloc, 10).value();

    // Test `inline_realloc_multi_to()`.
    _ = allocator.inline_realloc_multi_to(allocator, alloc, 10).value();

    // Test `inline_xrealloc_multi()`.
    _ = allocator.inline_xrealloc_multi(alloc, 10);

    // Test `inline_xrealloc_multi_to()`.
    _ = allocator.inline_xrealloc_multi_to(allocator, alloc, 10);

    // Test `inline_align_realloc_multi()`.
    _ = allocator.inline_align_realloc_multi(alloc, 8u, 10).value();

    // Test `inline_align_realloc_multi_to()`.
    _ = allocator.inline_align_realloc_multi_to(allocator, alloc, 8u, 10)
            .value();

    // Test `inline_align_xrealloc_multi()`.
    _ = allocator.inline_align_xrealloc_multi(alloc, 8u, 10);

    // Test `inline_align_xrealloc_multi_to()`.
    _ = allocator.inline_align_xrealloc_multi_to(allocator, alloc, 8u, 10);

    // Test `inline_unalign_realloc_multi()`.
    _ = allocator.inline_unalign_realloc_multi(alloc, 10).value();

    // Test `inline_unalign_realloc_multi_to()`.
    _ = allocator.inline_unalign_realloc_multi_to(allocator, alloc, 10).value();

    // Test `inline_unalign_xrealloc_multi()`.
    _ = allocator.inline_unalign_xrealloc_multi(alloc, 10);

    // Test `inline_unalign_xrealloc_multi_to()`.
    _ = allocator.inline_unalign_xrealloc_multi_to(allocator, alloc, 10);

    // Test `inline_align_realloc_multi()`.
    _ = allocator.inline_align_realloc_multi(alloc, 8u, 10).value();

    // Test `inline_align_realloc_multi_to()`.
    _ = allocator.inline_align_realloc_multi_to(allocator, alloc, 8u, 10)
            .value();

    // Test `inline_align_xrealloc_multi()`.
    _ = allocator.inline_align_xrealloc_multi(alloc, 8u, 10);

    // Test `inline_align_xrealloc_multi_to()`.
    _ = allocator.inline_align_xrealloc_multi_to(allocator, alloc, 8u, 10);

    // Test `inline_unalign_realloc_multi()`.
    _ = allocator.inline_unalign_realloc_multi(alloc, 10).value();

    // Test `inline_unalign_realloc_multi_to()`.
    _ = allocator.inline_unalign_realloc_multi_to(allocator, alloc, 10).value();

    // Test `inline_unalign_xrealloc_multi()`.
    _ = allocator.inline_unalign_xrealloc_multi(alloc, 10);

    // Test `inline_unalign_xrealloc_multi_to()`.
    _ = allocator.inline_unalign_xrealloc_multi_to(allocator, alloc, 10);

    // The allocator runs out of memory around here.
    allocator.reset();

    // Test `inline_recalloc()`.
    _ = allocator.inline_recalloc(alloc).value();

    // Test `inline_recalloc_to()`.
    _ = allocator.inline_recalloc_to(allocator, alloc).value();

    // Test `inline_xrecalloc()`.
    _ = allocator.inline_xrecalloc(alloc);

    // Test `inline_xrecalloc_to()`.
    _ = allocator.inline_xrecalloc_to(allocator, alloc);

    // Test `inline_align_recalloc()`.
    _ = allocator.inline_align_recalloc(alloc, 8u).value();

    // Test `inline_align_recalloc_to()`.
    _ = allocator.inline_align_recalloc_to(allocator, alloc, 8u).value();

    // Test `inline_align_xrecalloc()`.
    _ = allocator.inline_align_xrecalloc(alloc, 8u);

    // Test `inline_align_xrecalloc_to()`.
    _ = allocator.inline_align_xrecalloc_to(allocator, alloc, 8u);

    // Test `inline_unalign_recalloc()`.
    _ = allocator.inline_unalign_recalloc(alloc).value();

    // Test `inline_unalign_recalloc_to()`.
    _ = allocator.inline_unalign_recalloc_to(allocator, alloc).value();

    // Test `inline_unalign_xrecalloc()`.
    _ = allocator.inline_unalign_xrecalloc(alloc);

    // Test `inline_unalign_xrecalloc_to()`.
    _ = allocator.inline_unalign_xrecalloc_to(allocator, alloc);

    // Test `inline_align_recalloc()`.
    _ = allocator.inline_align_recalloc(alloc, 8u).value();

    // Test `inline_align_recalloc_to()`.
    _ = allocator.inline_align_recalloc_to(allocator, alloc, 8u).value();

    // Test `inline_align_xrecalloc()`.
    _ = allocator.inline_align_xrecalloc(alloc, 8u);

    // Test `inline_align_xrecalloc_to()`.
    _ = allocator.inline_align_xrecalloc_to(allocator, alloc, 8u);

    // Test `inline_unalign_recalloc()`.
    _ = allocator.inline_unalign_recalloc(alloc).value();

    // Test `inline_unalign_recalloc_to()`.
    _ = allocator.inline_unalign_recalloc_to(allocator, alloc).value();

    // Test `inline_unalign_xrecalloc()`.
    _ = allocator.inline_unalign_xrecalloc(alloc);

    // Test `inline_unalign_xrecalloc_to()`.
    _ = allocator.inline_unalign_xrecalloc_to(allocator, alloc);

    // Test `inline_recalloc_multi()`.
    _ = allocator.inline_recalloc_multi(alloc, 10).value();

    // Test `inline_recalloc_multi_to()`.
    _ = allocator.inline_recalloc_multi_to(allocator, alloc, 10).value();

    // Test `inline_xrecalloc_multi()`.
    _ = allocator.inline_xrecalloc_multi(alloc, 10);

    // Test `inline_xrecalloc_multi_to()`.
    _ = allocator.inline_xrecalloc_multi_to(allocator, alloc, 10);

    // Test `inline_align_recalloc_multi()`.
    _ = allocator.inline_align_recalloc_multi(alloc, 8u, 10).value();

    // Test `inline_align_recalloc_multi_to()`.
    _ = allocator.inline_align_recalloc_multi_to(allocator, alloc, 8u, 10)
            .value();

    // Test `inline_align_xrecalloc_multi()`.
    _ = allocator.inline_align_xrecalloc_multi(alloc, 8u, 10);

    // Test `inline_align_xrecalloc_multi_to()`.
    _ = allocator.inline_align_xrecalloc_multi_to(allocator, alloc, 8u, 10);

    // Test `inline_unalign_recalloc_multi()`.
    _ = allocator.inline_unalign_recalloc_multi(alloc, 10).value();

    // Test `inline_unalign_recalloc_multi_to()`.
    _ = allocator.inline_unalign_recalloc_multi_to(allocator, alloc, 10)
            .value();

    // Test `inline_unalign_xrecalloc_multi()`.
    _ = allocator.inline_unalign_xrecalloc_multi(alloc, 10);

    // Test `inline_unalign_xrecalloc_multi_to()`.
    _ = allocator.inline_unalign_xrecalloc_multi_to(allocator, alloc, 10);

    // Test `inline_align_recalloc_multi()`.
    _ = allocator.inline_align_recalloc_multi(alloc, 8u, 10).value();

    // Test `inline_align_recalloc_multi_to()`.
    _ = allocator.inline_align_recalloc_multi_to(allocator, alloc, 8u, 10)
            .value();

    // Test `inline_align_xrecalloc_multi()`.
    _ = allocator.inline_align_xrecalloc_multi(alloc, 8u, 10);

    // Test `inline_align_xrecalloc_multi_to()`.
    _ = allocator.inline_align_xrecalloc_multi_to(allocator, alloc, 8u, 10);

    // Test `inline_unalign_recalloc_multi()`.
    _ = allocator.inline_unalign_recalloc_multi(alloc, 10).value();

    // Test `inline_unalign_recalloc_multi_to()`.
    _ = allocator.inline_unalign_recalloc_multi_to(allocator, alloc, 10)
            .value();

    // Test `inline_unalign_xrecalloc_multi()`.
    _ = allocator.inline_unalign_xrecalloc_multi(alloc, 10);

    // Test `inline_unalign_xrecalloc_multi_to()`.
    _ = allocator.inline_unalign_xrecalloc_multi_to(allocator, alloc, 10);

    // Test `resalloc()`.
    _ = allocator.resalloc(alloc).value();

    // Test `resalloc_to()`.
    _ = allocator.resalloc_to(allocator, alloc).value();

    // Test `p_resalloc()`.
    _ = allocator.p_resalloc(p_alloc).value();

    // Test `p_resalloc_to()`
    _ = allocator.p_resalloc_to(allocator, p_alloc);

    // Test `xresalloc()`.
    _ = allocator.xresalloc(alloc);

    // Test `xresalloc_to()`.
    _ = allocator.xresalloc_to(allocator, alloc);

    // Test `p_xresalloc()`
    _ = allocator.p_xresalloc(p_alloc);

    // Test `p_xresalloc_to()`
    _ = allocator.p_xresalloc_to(allocator, p_alloc);

    // Test `align_resalloc()`.
    _ = allocator.align_resalloc(alloc, 8u).value();

    // Test `align_resalloc_to()`.
    _ = allocator.align_resalloc_to(allocator, alloc, 8u).value();

    // Test `align_xresalloc()`.
    _ = allocator.align_xresalloc(alloc, 8u);

    // Test `align_xresalloc_to()`.
    _ = allocator.align_xresalloc_to(allocator, alloc, 8u);

    // Test `unalign_resalloc()`.
    _ = allocator.unalign_resalloc(alloc).value();

    // Test `unalign_resalloc_to()`.
    _ = allocator.unalign_resalloc_to(allocator, alloc).value();

    // Test `unalign_xresalloc()`.
    _ = allocator.unalign_xresalloc(alloc);

    // Test `unalign_xresalloc_to()`.
    _ = allocator.unalign_xresalloc_to(allocator, alloc);

    // Test `align_resalloc()`.
    _ = allocator.align_resalloc(alloc, 8u).value();

    // Test `align_resalloc_to()`.
    _ = allocator.align_resalloc_to(allocator, alloc, 8u).value();

    // Test `align_xresalloc()`.
    _ = allocator.align_xresalloc(alloc, 8u);

    // Test `align_xresalloc_to()`.
    _ = allocator.align_xresalloc_to(allocator, alloc, 8u);

    // Test `unalign_resalloc()`.
    _ = allocator.unalign_resalloc(alloc).value();

    // Test `unalign_resalloc_to()`.
    _ = allocator.unalign_resalloc_to(allocator, alloc).value();

    // Test `unalign_xresalloc()`.
    _ = allocator.unalign_xresalloc(alloc);

    // Test `unalign_xresalloc_to()`.
    _ = allocator.unalign_xresalloc_to(allocator, alloc);

    // Test `p_align_resalloc()`.
    _ = allocator.p_align_resalloc(p_alloc, 8u).value();

    // Test `p_align_resalloc_to()`.
    _ = allocator.p_align_resalloc_to(allocator, p_alloc, 8u).value();

    // Test `p_align_xresalloc()`.
    _ = allocator.p_align_xresalloc(p_alloc, 8u);

    // Test `p_align_xresalloc_to()`.
    _ = allocator.p_align_xresalloc_to(allocator, p_alloc, 8u);

    // Test `p_unalign_resalloc()`.
    _ = allocator.p_unalign_resalloc(p_alloc).value();

    // Test `p_unalign_resalloc_to()`.
    _ = allocator.p_unalign_resalloc_to(allocator, p_alloc).value();

    // Test `p_unalign_xresalloc()`.
    _ = allocator.p_unalign_xresalloc(p_alloc);

    // Test `p_unalign_xresalloc_to()`.
    _ = allocator.p_unalign_xresalloc_to(allocator, p_alloc);

    // Test `resalloc_multi()`.
    _ = allocator.resalloc_multi(alloc, 10).value();

    // Test `resalloc_multi_to()`.
    _ = allocator.resalloc_multi_to(allocator, alloc, 10).value();

    // Test `p_resalloc_multi()`.
    _ = allocator.p_resalloc_multi(p_alloc, 5, 10).value();

    // Test `p_resalloc_multi_to()`
    _ = allocator.p_resalloc_multi_to(allocator, p_alloc, 5, 10).value();

    // Test `xresalloc_multi()`.
    _ = allocator.xresalloc_multi(alloc, 10);

    // Test `xresalloc_multi_to()`.
    _ = allocator.xresalloc_multi_to(allocator, alloc, 10);

    // Test `p_xresalloc_multi()`
    _ = allocator.p_xresalloc_multi(p_alloc, 5, 10);

    // Test `p_xresalloc_multi_to()`
    _ = allocator.p_xresalloc_multi_to(allocator, p_alloc, 5, 10);

    // Test `align_resalloc_multi()`.
    _ = allocator.align_resalloc_multi(alloc, 8u, 10).value();

    // Test `align_resalloc_multi_to()`.
    _ = allocator.align_resalloc_multi_to(allocator, alloc, 8u, 10).value();

    // Test `align_xresalloc_multi()`.
    _ = allocator.align_xresalloc_multi(alloc, 8u, 10);

    // Test `align_xresalloc_multi_to()`.
    _ = allocator.align_xresalloc_multi_to(allocator, alloc, 8u, 10);

    // Test `unalign_resalloc_multi()`.
    _ = allocator.unalign_resalloc_multi(alloc, 10).value();

    // Test `unalign_resalloc_multi_to()`.
    _ = allocator.unalign_resalloc_multi_to(allocator, alloc, 10).value();

    // Test `unalign_xresalloc_multi()`.
    _ = allocator.unalign_xresalloc_multi(alloc, 10);

    // Test `unalign_xresalloc_multi_to()`.
    _ = allocator.unalign_xresalloc_multi_to(allocator, alloc, 10);

    // Test `align_resalloc_multi()`.
    _ = allocator.align_resalloc_multi(alloc, 8u, 10).value();

    // Test `align_resalloc_multi_to()`.
    _ = allocator.align_resalloc_multi_to(allocator, alloc, 8u, 10).value();

    // Test `align_xresalloc_multi()`.
    _ = allocator.align_xresalloc_multi(alloc, 8u, 10);

    // Test `align_xresalloc_multi_to()`.
    _ = allocator.align_xresalloc_multi_to(allocator, alloc, 8u, 10);

    // Test `unalign_resalloc_multi()`.
    _ = allocator.unalign_resalloc_multi(alloc, 10).value();

    // Test `unalign_resalloc_multi_to()`.
    _ = allocator.unalign_resalloc_multi_to(allocator, alloc, 10).value();

    // Test `unalign_xresalloc_multi()`.
    _ = allocator.unalign_xresalloc_multi(alloc, 10);

    // Test `unalign_xresalloc_multi_to()`.
    _ = allocator.unalign_xresalloc_multi_to(allocator, alloc, 10);

    // Test `p_align_resalloc_multi()`.
    _ = allocator.p_align_resalloc_multi(p_alloc, 8u, 5, 10).value();

    // Test `p_align_resalloc_multi_to()`.
    _ = allocator.p_align_resalloc_multi_to(allocator, p_alloc, 8u, 5, 10)
            .value();

    // Test `p_align_xresalloc_multi()`.
    _ = allocator.p_align_xresalloc_multi(p_alloc, 8u, 5, 10);

    // Test `p_align_xresalloc_multi_to()`.
    _ = allocator.p_align_xresalloc_multi_to(allocator, p_alloc, 8u, 5, 10);

    // Test `p_unalign_resalloc_multi()`.
    _ = allocator.p_unalign_resalloc_multi(p_alloc, 5, 10).value();

    // Test `p_unalign_resalloc_multi_to()`.
    _ = allocator.p_unalign_resalloc_multi_to(allocator, p_alloc, 5, 10)
            .value();

    // Test `p_unalign_xresalloc_multi()`.
    _ = allocator.p_unalign_xresalloc_multi(p_alloc, 5, 10);

    // Test `p_unalign_xresalloc_multi_to()`.
    _ = allocator.p_unalign_xresalloc_multi_to(allocator, p_alloc, 5, 10);

    // The allocator runs out of memory around here.
    allocator.reset();

    // Test `rescalloc()`.
    _ = allocator.rescalloc(alloc).value();

    // Test `rescalloc_to()`.
    _ = allocator.rescalloc_to(allocator, alloc).value();

    // Test `p_rescalloc()`.
    _ = allocator.p_rescalloc(p_alloc).value();

    // Test `p_rescalloc_to()`
    _ = allocator.p_rescalloc_to(allocator, p_alloc);

    // Test `xrescalloc()`.
    _ = allocator.xrescalloc(alloc);

    // Test `xrescalloc_to()`.
    _ = allocator.xrescalloc_to(allocator, alloc);

    // Test `p_xrescalloc()`
    _ = allocator.p_xrescalloc(p_alloc);

    // Test `p_xrescalloc_to()`
    _ = allocator.p_xrescalloc_to(allocator, p_alloc);

    // Test `align_rescalloc()`.
    _ = allocator.align_rescalloc(alloc, 8u).value();

    // Test `align_rescalloc_to()`.
    _ = allocator.align_rescalloc_to(allocator, alloc, 8u).value();

    // Test `align_xrescalloc()`.
    _ = allocator.align_xrescalloc(alloc, 8u);

    // Test `align_xrescalloc_to()`.
    _ = allocator.align_xrescalloc_to(allocator, alloc, 8u);

    // Test `unalign_rescalloc()`.
    _ = allocator.unalign_rescalloc(alloc).value();

    // Test `unalign_rescalloc_to()`.
    _ = allocator.unalign_rescalloc_to(allocator, alloc).value();

    // Test `unalign_xrescalloc()`.
    _ = allocator.unalign_xrescalloc(alloc);

    // Test `unalign_xrescalloc_to()`.
    _ = allocator.unalign_xrescalloc_to(allocator, alloc);

    // Test `align_rescalloc()`.
    _ = allocator.align_rescalloc(alloc, 8u).value();

    // Test `align_rescalloc_to()`.
    _ = allocator.align_rescalloc_to(allocator, alloc, 8u).value();

    // Test `align_xrescalloc()`.
    _ = allocator.align_xrescalloc(alloc, 8u);

    // Test `align_xrescalloc_to()`.
    _ = allocator.align_xrescalloc_to(allocator, alloc, 8u);

    // Test `unalign_rescalloc()`.
    _ = allocator.unalign_rescalloc(alloc).value();

    // Test `unalign_rescalloc_to()`.
    _ = allocator.unalign_rescalloc_to(allocator, alloc).value();

    // Test `unalign_xrescalloc()`.
    _ = allocator.unalign_xrescalloc(alloc);

    // Test `unalign_xrescalloc_to()`.
    _ = allocator.unalign_xrescalloc_to(allocator, alloc);

    // Test `p_align_rescalloc()`.
    _ = allocator.p_align_rescalloc(p_alloc, 8u).value();

    // Test `p_align_rescalloc_to()`.
    _ = allocator.p_align_rescalloc_to(allocator, p_alloc, 8u).value();

    // Test `p_align_xrescalloc()`.
    _ = allocator.p_align_xrescalloc(p_alloc, 8u);

    // Test `p_align_xrescalloc_to()`.
    _ = allocator.p_align_xrescalloc_to(allocator, p_alloc, 8u);

    // Test `p_unalign_rescalloc()`.
    _ = allocator.p_unalign_rescalloc(p_alloc).value();

    // Test `p_unalign_rescalloc_to()`.
    _ = allocator.p_unalign_rescalloc_to(allocator, p_alloc).value();

    // Test `p_unalign_xrescalloc()`.
    _ = allocator.p_unalign_xrescalloc(p_alloc);

    // Test `p_unalign_xrescalloc_to()`.
    _ = allocator.p_unalign_xrescalloc_to(allocator, p_alloc);

    // Test `rescalloc_multi()`.
    _ = allocator.rescalloc_multi(alloc, 10).value();

    // Test `rescalloc_multi_to()`.
    _ = allocator.rescalloc_multi_to(allocator, alloc, 10).value();

    // Test `p_rescalloc_multi()`.
    _ = allocator.p_rescalloc_multi(p_alloc, 5, 10).value();

    // Test `p_rescalloc_multi_to()`
    _ = allocator.p_rescalloc_multi_to(allocator, p_alloc, 5, 10).value();

    // Test `xrescalloc_multi()`.
    _ = allocator.xrescalloc_multi(alloc, 10);

    // Test `xrescalloc_multi_to()`.
    _ = allocator.xrescalloc_multi_to(allocator, alloc, 10);

    // Test `p_xrescalloc_multi()`
    _ = allocator.p_xrescalloc_multi(p_alloc, 5, 10);

    // Test `p_xrescalloc_multi_to()`
    _ = allocator.p_xrescalloc_multi_to(allocator, p_alloc, 5, 10);

    // Test `align_rescalloc_multi()`.
    _ = allocator.align_rescalloc_multi(alloc, 8u, 10).value();

    // Test `align_rescalloc_multi_to()`.
    _ = allocator.align_rescalloc_multi_to(allocator, alloc, 8u, 10).value();

    // Test `align_xrescalloc_multi()`.
    _ = allocator.align_xrescalloc_multi(alloc, 8u, 10);

    // Test `align_xrescalloc_multi_to()`.
    _ = allocator.align_xrescalloc_multi_to(allocator, alloc, 8u, 10);

    // Test `unalign_rescalloc_multi()`.
    _ = allocator.unalign_rescalloc_multi(alloc, 10).value();

    // Test `unalign_rescalloc_multi_to()`.
    _ = allocator.unalign_rescalloc_multi_to(allocator, alloc, 10).value();

    // Test `unalign_xrescalloc_multi()`.
    _ = allocator.unalign_xrescalloc_multi(alloc, 10);

    // Test `unalign_xrescalloc_multi_to()`.
    _ = allocator.unalign_xrescalloc_multi_to(allocator, alloc, 10);

    // Test `align_rescalloc_multi()`.
    _ = allocator.align_rescalloc_multi(alloc, 8u, 10).value();

    // Test `align_rescalloc_multi_to()`.
    _ = allocator.align_rescalloc_multi_to(allocator, alloc, 8u, 10).value();

    // Test `align_xrescalloc_multi()`.
    _ = allocator.align_xrescalloc_multi(alloc, 8u, 10);

    // Test `align_xrescalloc_multi_to()`.
    _ = allocator.align_xrescalloc_multi_to(allocator, alloc, 8u, 10);

    // Test `unalign_rescalloc_multi()`.
    _ = allocator.unalign_rescalloc_multi(alloc, 10).value();

    // Test `unalign_rescalloc_multi_to()`.
    _ = allocator.unalign_rescalloc_multi_to(allocator, alloc, 10).value();

    // Test `unalign_xrescalloc_multi()`.
    _ = allocator.unalign_xrescalloc_multi(alloc, 10);

    // Test `unalign_xrescalloc_multi_to()`.
    _ = allocator.unalign_xrescalloc_multi_to(allocator, alloc, 10);

    // Test `p_align_rescalloc_multi()`.
    _ = allocator.p_align_rescalloc_multi(p_alloc, 8u, 5, 10).value();

    // Test `p_align_rescalloc_multi_to()`.
    _ = allocator.p_align_rescalloc_multi_to(allocator, p_alloc, 8u, 5, 10)
            .value();

    // Test `p_align_xrescalloc_multi()`.
    _ = allocator.p_align_xrescalloc_multi(p_alloc, 8u, 5, 10);

    // Test `p_align_xrescalloc_multi_to()`.
    _ = allocator.p_align_xrescalloc_multi_to(allocator, p_alloc, 8u, 5, 10);

    // Test `p_unalign_rescalloc_multi()`.
    _ = allocator.p_unalign_rescalloc_multi(p_alloc, 5, 10).value();

    // Test `p_unalign_rescalloc_multi_to()`.
    _ = allocator.p_unalign_rescalloc_multi_to(allocator, p_alloc, 5, 10)
            .value();

    // Test `p_unalign_xrescalloc_multi()`.
    _ = allocator.p_unalign_xrescalloc_multi(p_alloc, 5, 10);

    // Test `p_unalign_xrescalloc_multi_to()`.
    _ = allocator.p_unalign_xrescalloc_multi_to(allocator, p_alloc, 5, 10);

    // Test `inline_resalloc()`.
    _ = allocator.inline_resalloc(alloc);

    // Test `inline_resalloc_to()`.
    _ = allocator.inline_resalloc_to(allocator, alloc).value();

    // Test `inline_xresalloc()`.
    _ = allocator.inline_xresalloc(alloc);

    // Test `inline_xresalloc_to()`.
    _ = allocator.inline_xresalloc_to(allocator, alloc);

    // Test `inline_align_resalloc()`.
    _ = allocator.inline_align_resalloc(alloc, 8u).value();

    // Test `inline_align_resalloc_to()`.
    _ = allocator.inline_align_resalloc_to(allocator, alloc, 8u).value();

    // Test `inline_align_xresalloc()`.
    _ = allocator.inline_align_xresalloc(alloc, 8u);

    // Test `inline_align_xresalloc_to()`.
    _ = allocator.inline_align_xresalloc_to(allocator, alloc, 8u);

    // Test `inline_unalign_resalloc()`.
    _ = allocator.inline_unalign_resalloc(alloc).value();

    // Test `inline_unalign_resalloc_to()`.
    _ = allocator.inline_unalign_resalloc_to(allocator, alloc).value();

    // Test `inline_unalign_xresalloc()`.
    _ = allocator.inline_unalign_xresalloc(alloc);

    // Test `inline_unalign_xresalloc_to()`.
    _ = allocator.inline_unalign_xresalloc_to(allocator, alloc);

    // Test `inline_align_resalloc()`.
    _ = allocator.inline_align_resalloc(alloc, 8u).value();

    // Test `inline_align_resalloc_to()`.
    _ = allocator.inline_align_resalloc_to(allocator, alloc, 8u).value();

    // Test `inline_align_xresalloc()`.
    _ = allocator.inline_align_xresalloc(alloc, 8u);

    // Test `inline_align_xresalloc_to()`.
    _ = allocator.inline_align_xresalloc_to(allocator, alloc, 8u);

    // Test `inline_unalign_resalloc()`.
    _ = allocator.inline_unalign_resalloc(alloc).value();

    // Test `inline_unalign_resalloc_to()`.
    _ = allocator.inline_unalign_resalloc_to(allocator, alloc).value();

    // Test `inline_unalign_xresalloc()`.
    _ = allocator.inline_unalign_xresalloc(alloc);

    // Test `inline_unalign_xresalloc_to()`.
    _ = allocator.inline_unalign_xresalloc_to(allocator, alloc);

    // Test `inline_resalloc_multi()`.
    _ = allocator.inline_resalloc_multi(alloc, 10).value();

    // Test `inline_resalloc_multi_to()`.
    _ = allocator.inline_resalloc_multi_to(allocator, alloc, 10).value();

    // Test `inline_xresalloc_multi()`.
    _ = allocator.inline_xresalloc_multi(alloc, 10);

    // Test `inline_xresalloc_multi_to()`.
    _ = allocator.inline_xresalloc_multi_to(allocator, alloc, 10);

    // Test `inline_align_resalloc_multi()`.
    _ = allocator.inline_align_resalloc_multi(alloc, 8u, 10).value();

    // Test `inline_align_resalloc_multi_to()`.
    _ = allocator.inline_align_resalloc_multi_to(allocator, alloc, 8u, 10)
            .value();

    // Test `inline_align_xresalloc_multi()`.
    _ = allocator.inline_align_xresalloc_multi(alloc, 8u, 10);

    // Test `inline_align_xresalloc_multi_to()`.
    _ = allocator.inline_align_xresalloc_multi_to(allocator, alloc, 8u, 10);

    // Test `inline_unalign_resalloc_multi()`.
    _ = allocator.inline_unalign_resalloc_multi(alloc, 10).value();

    // Test `inline_unalign_resalloc_multi_to()`.
    _ = allocator.inline_unalign_resalloc_multi_to(allocator, alloc, 10)
            .value();

    // Test `inline_unalign_xresalloc_multi()`.
    _ = allocator.inline_unalign_xresalloc_multi(alloc, 10);

    // Test `inline_unalign_xresalloc_multi_to()`.
    _ = allocator.inline_unalign_xresalloc_multi_to(allocator, alloc, 10);

    // Test `inline_align_resalloc_multi()`.
    _ = allocator.inline_align_resalloc_multi(alloc, 8u, 10).value();

    // Test `inline_align_resalloc_multi_to()`.
    _ = allocator.inline_align_resalloc_multi_to(allocator, alloc, 8u, 10)
            .value();

    // Test `inline_align_xresalloc_multi()`.
    _ = allocator.inline_align_xresalloc_multi(alloc, 8u, 10);

    // Test `inline_align_xresalloc_multi_to()`.
    _ = allocator.inline_align_xresalloc_multi_to(allocator, alloc, 8u, 10);

    // Test `inline_unalign_resalloc_multi()`.
    _ = allocator.inline_unalign_resalloc_multi(alloc, 10).value();

    // Test `inline_unalign_resalloc_multi_to()`.
    _ = allocator.inline_unalign_resalloc_multi_to(allocator, alloc, 10)
            .value();

    // Test `inline_unalign_xresalloc_multi()`.
    _ = allocator.inline_unalign_xresalloc_multi(alloc, 10);

    // Test `inline_unalign_xresalloc_multi_to()`.
    _ = allocator.inline_unalign_xresalloc_multi_to(allocator, alloc, 10);

    // The allocator runs out of memory around here.
    allocator.reset();

    // Test `inline_rescalloc()`.
    _ = allocator.inline_rescalloc(alloc).value();

    // Test `inline_rescalloc_to()`.
    _ = allocator.inline_rescalloc_to(allocator, alloc).value();

    // Test `inline_xrescalloc()`.
    _ = allocator.inline_xrescalloc(alloc);

    // Test `inline_xrescalloc_to()`.
    _ = allocator.inline_xrescalloc_to(allocator, alloc);

    // Test `inline_align_rescalloc()`.
    _ = allocator.inline_align_rescalloc(alloc, 8u).value();

    // Test `inline_align_rescalloc_to()`.
    _ = allocator.inline_align_rescalloc_to(allocator, alloc, 8u).value();

    // Test `inline_align_xrescalloc()`.
    _ = allocator.inline_align_xrescalloc(alloc, 8u);

    // Test `inline_align_xrescalloc_to()`.
    _ = allocator.inline_align_xrescalloc_to(allocator, alloc, 8u);

    // Test `inline_unalign_rescalloc()`.
    _ = allocator.inline_unalign_rescalloc(alloc).value();

    // Test `inline_unalign_rescalloc_to()`.
    _ = allocator.inline_unalign_rescalloc_to(allocator, alloc).value();

    // Test `inline_unalign_xrescalloc()`.
    _ = allocator.inline_unalign_xrescalloc(alloc);

    // Test `inline_unalign_xrescalloc_to()`.
    _ = allocator.inline_unalign_xrescalloc_to(allocator, alloc);

    // Test `inline_align_rescalloc()`.
    _ = allocator.inline_align_rescalloc(alloc, 8u).value();

    // Test `inline_align_rescalloc_to()`.
    _ = allocator.inline_align_rescalloc_to(allocator, alloc, 8u).value();

    // Test `inline_align_xrescalloc()`.
    _ = allocator.inline_align_xrescalloc(alloc, 8u);

    // Test `inline_align_xrescalloc_to()`.
    _ = allocator.inline_align_xrescalloc_to(allocator, alloc, 8u);

    // Test `inline_unalign_rescalloc()`.
    _ = allocator.inline_unalign_rescalloc(alloc).value();

    // Test `inline_unalign_rescalloc_to()`.
    _ = allocator.inline_unalign_rescalloc_to(allocator, alloc).value();

    // Test `inline_unalign_xrescalloc()`.
    _ = allocator.inline_unalign_xrescalloc(alloc);

    // Test `inline_unalign_xrescalloc_to()`.
    _ = allocator.inline_unalign_xrescalloc_to(allocator, alloc);

    // Test `inline_rescalloc_multi()`.
    _ = allocator.inline_rescalloc_multi(alloc, 10).value();

    // Test `inline_rescalloc_multi_to()`.
    _ = allocator.inline_rescalloc_multi_to(allocator, alloc, 10).value();

    // Test `inline_xrescalloc_multi()`.
    _ = allocator.inline_xrescalloc_multi(alloc, 10);

    // Test `inline_xrescalloc_multi_to()`.
    _ = allocator.inline_xrescalloc_multi_to(allocator, alloc, 10);

    // Test `inline_align_rescalloc_multi()`.
    _ = allocator.inline_align_rescalloc_multi(alloc, 8u, 10).value();

    // Test `inline_align_rescalloc_multi_to()`.
    _ = allocator.inline_align_rescalloc_multi_to(allocator, alloc, 8u, 10)
            .value();

    // Test `inline_align_xrescalloc_multi()`.
    _ = allocator.inline_align_xrescalloc_multi(alloc, 8u, 10);

    // Test `inline_align_xrescalloc_multi_to()`.
    _ = allocator.inline_align_xrescalloc_multi_to(allocator, alloc, 8u, 10);

    // Test `inline_unalign_rescalloc_multi()`.
    _ = allocator.inline_unalign_rescalloc_multi(alloc, 10).value();

    // Test `inline_unalign_rescalloc_multi_to()`.
    _ = allocator.inline_unalign_rescalloc_multi_to(allocator, alloc, 10)
            .value();

    // Test `inline_unalign_xrescalloc_multi()`.
    _ = allocator.inline_unalign_xrescalloc_multi(alloc, 10);

    // Test `inline_unalign_xrescalloc_multi_to()`.
    _ = allocator.inline_unalign_xrescalloc_multi_to(allocator, alloc, 10);

    // Test `inline_align_rescalloc_multi()`.
    _ = allocator.inline_align_rescalloc_multi(alloc, 8u, 10).value();

    // Test `inline_align_rescalloc_multi_to()`.
    _ = allocator.inline_align_rescalloc_multi_to(allocator, alloc, 8u, 10)
            .value();

    // Test `inline_align_xrescalloc_multi()`.
    _ = allocator.inline_align_xrescalloc_multi(alloc, 8u, 10);

    // Test `inline_align_xrescalloc_multi_to()`.
    _ = allocator.inline_align_xrescalloc_multi_to(allocator, alloc, 8u, 10);

    // Test `inline_unalign_rescalloc_multi()`.
    _ = allocator.inline_unalign_rescalloc_multi(alloc, 10).value();

    // Test `inline_unalign_rescalloc_multi_to()`.
    _ = allocator.inline_unalign_rescalloc_multi_to(allocator, alloc, 10)
            .value();

    // Test `inline_unalign_xrescalloc_multi()`.
    _ = allocator.inline_unalign_xrescalloc_multi(alloc, 10);

    // Test `inline_unalign_xrescalloc_multi_to()`.
    _ = allocator.inline_unalign_xrescalloc_multi_to(allocator, alloc, 10);
};
