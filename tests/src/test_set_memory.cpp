#include <cat/memory>
#include <cat/page_allocator>

#include "../unit_tests.hpp"

TEST(test_set_memory) {
    using namespace cat::arithmetic_literals;

    cat::page_allocator allocator;
    uint1* p_page = allocator.alloc<uint1>().or_exit();

    // TODO: Make a `cat::compare_memory()` or something for this.

    cat::set_memory(p_page, 1_u1, 4_uki);
    cat::verify(p_page[1000] == 1_u1);

    // Test that unaligned memory still sets correctly.
    cat::set_memory(p_page + 3, 2_u1, 2_uki - 6);
    cat::verify(p_page[0] == 1_u1);
    cat::verify(p_page[2] == 1_u1);
    cat::verify(p_page[3] == 2_u1);
    // TODO: Why did this stop working?
    // cat::verify(p_page[(2_ki - 4).get_raw()] == 2_u1);
    cat::verify(p_page[(2_ki - 3).get_raw()] == 1_u1);

    // Test zeroing out memory.
    cat::zero_memory(p_page, 4_uki);
    cat::verify(p_page[0] == 0_u1);
    cat::verify(p_page[(4_ki).get_raw() - 1] == 0_u1);

    // Test setting values larger than 1 byte.
    cat::set_memory(p_page, 1_i2, 2_uki);
    cat::verify(static_cast<int2*>(static_cast<void*>(p_page))[10] == 1_i2);
    // The next byte after this should be 0.
    cat::verify(static_cast<int1*>(static_cast<void*>(p_page))[21] == 0);

    cat::set_memory(p_page, 1_i4, 1_uki);
    cat::verify(static_cast<int4*>(static_cast<void*>(p_page))[10] == 1_i4);

    cat::set_memory(p_page, 1_i8, 0.5_uki);
    cat::verify(static_cast<int4*>(static_cast<void*>(p_page))[10] == 1_i4);

    // Test scalar `set_memory()`.
    cat::set_memory_scalar(p_page, 1_u1, 4_uki);
    cat::verify(p_page[1001] == 1_u1);

    // Test scalar `zero_memory()`.
    cat::zero_memory_scalar(p_page, 4_uki);
    cat::verify(p_page[1001] == 0_u1);
};
