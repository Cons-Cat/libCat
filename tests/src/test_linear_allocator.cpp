#include <cat/array>
#include <cat/linear_allocator>
#include <cat/math>
#include <cat/page_allocator>
#include <cat/utility>

#include "../unit_tests.hpp"

TEST(test_linear_allocator) {
    // Initialize an allocator.
    cat::PageAllocator paging_allocator;
    auto page = paging_allocator.opq_alloc_multi<cat::Byte>(4_ki).or_exit();
    defer(paging_allocator.free(page);)
    auto allocator =
        cat::LinearAllocator::backed_handle_sized(paging_allocator, page, 24);

    [[maybe_unused]] auto allocator_2 =
        cat::LinearAllocator::backed(allocator, 128);
    allocator.reset();

    // It should not be possible to allocate 7 times here, because 24 bytes can
    // only hold 6 `int4`s.
    for (int i = 0; i < 7; ++i) {
        cat::Maybe handle = allocator.opq_alloc<int4>();
        if (!handle.has_value()) {
            cat::verify(i == 6);
            goto overallocated;
        }
    }
    cat::exit(1);

overallocated:
    // Invalidate all memory handles, and allocate again.
    allocator.reset();
    for (int4 i = 0; i < 4; ++i) {
        cat::Maybe handle = allocator.opq_alloc<cat::Byte>();
        cat::verify(handle.has_value());
    }
    // This allocated 16 bytes, which is 8-byte-aligned. Another int allocation
    // would make it 4-byte-aligned. However, 8 bytes should be allocated here
    // to keep it 8-byte-aligned.
    auto handle = allocator.opq_align_alloc<int4>(8u).value();
    cat::verify(cat::is_aligned(&allocator.get(handle), 8u));

    // Allocate another int.
    auto handle_2 = allocator.opq_alloc<int4>().value();
    cat::verify(cat::is_aligned(&allocator.get(handle_2), 4u));
    // This is now 4-byte-aligned.
    cat::verify(!cat::is_aligned(&allocator.get(handle_2), 8u));

    // Small size allocations shouldn't bump the allocator.
    for (int4 i = 0; i < 20; ++i) {
        auto memory = allocator.opq_inline_alloc<int4>();
        cat::verify(memory.has_value());
    }
    cat::Maybe handle_3 = allocator.opq_alloc<int4>();
    cat::verify(handle_3.has_value());

    // Test that allocations are reusable.
    allocator.reset();
    decltype(allocator.opq_alloc<int1>()) handles[4];
    for (signed char i = 0; i < 4; ++i) {
        handles[i] = allocator.opq_alloc<int1>();
        cat::verify(handles[i].has_value());
        allocator.get(handles[i].value()) = i;
    }
    for (signed char i = 0; i < 4; ++i) {
        cat::verify(allocator.get(handles[i].value()) == i);
    }

    // Test that allocations have pointer stability.
    allocator.reset();
    int4* p_handles[4];
    for (int i = 0; i < 4; ++i) {
        int4* p_handle = allocator.alloc<int4>().or_exit();
        *p_handle = i;
        p_handles[i] = p_handle;
    }
    for (int i = 0; i < 4; ++i) {
        cat::verify(*(p_handles[i]) == i);
        allocator.free(p_handles[i]);
    }

    allocator.reset();
    // Test allocation constructors.
    int4* p_initialized = allocator.alloc<int4>(100).or_exit();
    cat::verify(*p_initialized == 100);

    // Test sized allocations.
    allocator.reset();
    _ = allocator.opq_alloc<int2>().or_exit();
    // Because the allocator is now 2 byte aligned, an extra 2 bytes have to be
    // reserved to allocate a 4-byte aligned value:
    cat::verify(allocator.opq_nalloc<int4>().or_exit() == 6);
    cat::Tuple alloc_int_size = allocator.opq_salloc<int4>().value();
    cat::verify(alloc_int_size.second() == 6);

    // TODO: Test multi allocations.
    // TODO: Test inline multi allocations.
}
