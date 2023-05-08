#include <cat/pool_allocator>

#include "../unit_tests.hpp"

TEST(test_pool_allocator) {
    // Initialize an allocator.
    cat::page_allocator pager;
    // cat::mem auto page =
    //     pager.opq_alloc_multi<cat::byte>(4_ki).or_exit();
    cat::is_allocator auto allocator =
        cat::pool_allocator<8>::backed(pager, 128).value();

    // Make an allocation.
    int4* p_int1 = allocator.alloc<int4>(10).value();
    cat::verify(*p_int1 == 10);

    // Make another allocation.
    int4* p_int2 = allocator.alloc<int4>(20).value();
    cat::verify(*p_int2 == 20);

    // Test allocation limit is correct.
    for (int4 i = 0; i < (128 / 8) - 2; ++i) {
        _ = allocator.alloc<int4>().verify();
    }
    cat::maybe failed_allocation = allocator.alloc<int4>();
    cat::verify(!failed_allocation.has_value());

    // Make an allocation after resetting.
    allocator.reset();
    int4* p_int3 = allocator.alloc<int4>(30).verify();
    cat::verify(*p_int3 == 30);

    // Test freeing memory.
    for (int4 i = 0; i < (128 / 8) - 1; ++i) {
        _ = allocator.alloc<int4>().verify();
    }
    failed_allocation = allocator.alloc<int4>();
    cat::verify(!failed_allocation.has_value());

    allocator.free(p_int3);

    // Make one allocation after freeing.
    int4* p_int4 = allocator.alloc<int4>(40).verify();
    cat::verify(*p_int4 == 40);
}
