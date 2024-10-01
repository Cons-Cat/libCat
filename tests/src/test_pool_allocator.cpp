#include <cat/pool_allocator>

#include "../unit_tests.hpp"

TEST(test_pool_allocator) {
    // Initialize an allocator.
    cat::page_allocator pager;
    cat::span page = pager.alloc_multi<cat::byte>(128u).verify();
    defer {
        pager.free(page);
    };
    cat::is_allocator auto allocator = cat::make_pool_allocator<8>(page);

    // Make an allocation.
    int4* p_int1 = allocator.alloc<int4>(10).value();
    cat::verify(*p_int1 == 10);

    // Make another allocation.
    int8* p_int2 = allocator.alloc<int8>(20).value();
    cat::verify(*p_int2 == 20);

    // Test allocation limit is correct.
    // The allocator owns 128 bytes, divided into 16 sections of 8-bytes.
    // Two allocations have already been made, so 14 remain.
    for (idx i = 0; i < 14; ++i) {
        auto* _ = allocator.alloc<int4>(10).verify();
    }
    // There should now be no remaining allocations, so this fails.
    cat::maybe failed_allocation = allocator.alloc<int4>();
    cat::verify(!failed_allocation.has_value());

    // Make an allocation after resetting.
    allocator.reset();
    int4* p_int3 = allocator.alloc<int4>(30).verify();
    cat::verify(*p_int3 == 30);

    // Test freeing memory.
    for (idx i = 0; i < 15; ++i) {
        auto* _ = allocator.alloc<int4>().verify();
    }
    failed_allocation = allocator.alloc<int4>();
    cat::verify(!failed_allocation.has_value());

    allocator.free(p_int3);

    // Make one allocation after freeing.
    int4* p_int4 = allocator.alloc<int4>(40).verify();
    cat::verify(*p_int4 == 40);
}
