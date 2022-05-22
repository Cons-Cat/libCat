#include <cat/allocators>
#include <cat/array>
#include <cat/math>
#include <cat/numerals>
#include <cat/utility>

void meow() {
    // Initialize an allocator.
    cat::PageAllocator paging_allocator;
    auto page = paging_allocator.malloc<int4>(1_ki).value();

    cat::LinearAllocator allocator(&paging_allocator.get(page), 24);
    // It should not be possible to allocate 7 times here, because 24 bytes can
    // only hold 6 `int4`s.
    for (int i = 0; i < 7; i++) {
        cat::Optional handle = allocator.malloc<int4>();
        if (!handle.has_value()) {
            Result(i == 6).or_panic();
            goto didnt_overallocate;
        }
    }
    cat::exit(1);

didnt_overallocate:
    // Invalidate all memory handles, and allocate again.
    allocator.reset();
    for (int4 i = 0; i < 4; i++) {
        cat::Optional handle = allocator.malloc();
        Result(handle.has_value()).or_panic();
    }
    // This allocated 16 bytes, which is 8-byte-aligned. Another int allocation
    // would make it 4-byte-aligned. However, 8 bytes should be allocated here
    // to keep it 8-byte-aligned.
    auto handle = allocator.aligned_alloc<int4>(8u).value();
    Result(cat::is_aligned(&allocator.get(handle), 8u)).or_panic();

    // Allocate another int.
    auto handle_2 = allocator.malloc<int4>().value();
    Result(cat::is_aligned(&allocator.get(handle_2), 4u)).or_panic();
    // This is now 4-byte-aligned.
    Result(!cat::is_aligned(&allocator.get(handle_2), 8u)).or_panic();

    // Small size allocations shouldn't bump the allocator.
    for (int4 i = 0; i < 20; i++) {
        auto memory = allocator.malloca<int4>();
        Result(memory.has_value()).or_panic();
    }
    cat::Optional handle_3 = allocator.malloc<int4>();
    Result(handle_3.has_value()).or_panic();

    paging_allocator.free(page).or_panic();

    cat::exit();
}
