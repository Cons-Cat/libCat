#include <cat/memory>
#include <cat/numerals>
#include <cat/page_allocator>

auto main() -> int {
    cat::PageAllocator allocator;
    uint1* p_page = allocator.p_alloc_multi<uint1>(4_ki).or_exit();

    // TODO: Make a `cat::compare_memory()` or something for this.

    cat::set_memory(p_page, 1_u1, 4_ki);
    Result(p_page[1000] == 1_u1).or_exit();

    // Test that unaligned memory still sets correctly.
    cat::set_memory(p_page + 3, 2_u1, 2_ki - 6);
    Result(p_page[0] == 1_u1).or_exit();
    Result(p_page[2] == 1_u1).or_exit();
    Result(p_page[3] == 2_u1).or_exit();
    Result(p_page[(2_ki - 4).raw] == 2_u1).or_exit();
    Result(p_page[(2_ki - 3).raw] == 1_u1).or_exit();

    // Test zeroing out memory.
    cat::zero_memory(p_page, 4_ki);
    Result(p_page[0] == 0_u1).or_exit();
    Result(p_page[(4_ki).raw - 1] == 0_u1).or_exit();

    // Test setting values larger than 1 byte.
    cat::set_memory(p_page, 1_i2, 2_ki);
    Result(static_cast<int2*>(static_cast<void*>(p_page))[10] == 1_i2)
        .or_exit();
    // The next byte after this should be 0.
    Result(static_cast<int1*>(static_cast<void*>(p_page))[21] == 0).or_exit();

    cat::set_memory(p_page, 1_i4, 1_ki);
    Result(static_cast<int4*>(static_cast<void*>(p_page))[10] == 1_i4)
        .or_exit();

    cat::set_memory(p_page, 1_i8, 0.5_ki);
    Result(static_cast<int4*>(static_cast<void*>(p_page))[10] == 1_i4)
        .or_exit();

    // Test scalar `set_memory()`.
    cat::set_memory_scalar(p_page, 1_u1, 4_ki);
    Result(p_page[1001] == 1_u1).or_exit();

    // Test scalar `zero_memory()`.
    cat::zero_memory_scalar(p_page, 4_ki);
    Result(p_page[1001] == 0_u1).or_exit();
};
