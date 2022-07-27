#include <cat/memory>
#include <cat/numerals>
#include <cat/page_allocator>

auto main() -> int {
    cat::PageAllocator allocator;
    uint1* p_page = allocator.p_alloc_multi<uint1>(4_ki).or_exit();

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
};
