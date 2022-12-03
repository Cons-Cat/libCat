#include <cat/pool_allocator>

#include "../unit_tests.hpp"

TEST(test_pool_allocator) {
    // Initialize an allocator.
    cat::PageAllocator paging_allocator;
    // cat::mem auto page =
    //     paging_allocator.opq_alloc_multi<cat::Byte>(4_ki).or_exit();
    cat::is_stable_allocator auto allocator =
        cat::PoolAllocator<8>::backed(paging_allocator, 128).value();

    int4* p_int = allocator.alloc<int4>().value();
}
