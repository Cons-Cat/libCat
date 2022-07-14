#include <cat/allocators>
#include <cat/array>
#include <cat/math>
#include <cat/numerals>
#include <cat/utility>

int4 global_int_1 = 0;
int4 global_int_2 = 0;

struct TestType {
    TestType() {
        ++global_int_1;
    }
    ~TestType() {
        ++global_int_2;
    }
};

auto main() -> int {
    // Initialize an allocator.
    cat::PageAllocator allocator;
    // Allocate a page.
    auto memory = allocator.alloc<int4>(1_ki).or_exit("Failed to page memory!");
    // Free the page.
    defer(allocator.free(memory);)

    // Write to the page.
    allocator.get(memory) = 10;
    Result(allocator.get(memory) == 10).or_exit();

    // Allocation with small-size optimization.
    int stack_variable;
    auto small_memory_1 = allocator.inline_alloc<int4>().value();
    allocator.get(small_memory_1) = 2;
    // Both values should be on stack, so these addresses are close
    // together.
    Result(cat::abs(intptr{&stack_variable} -
                    intptr{&allocator.get(small_memory_1)}) < 512)
        .or_exit();
    // The handle's address should be the same as the data's if it was
    // allocated on the stack.
    int4& intref = *static_cast<int4*>(static_cast<void*>(&small_memory_1));
    intref = 10;
    Result(allocator.get(small_memory_1) == 10).or_exit();

    allocator.free(small_memory_1);

    small_memory_1 = allocator.inline_alloc<int4>(1_ki).value();
    // `small_memory_1` should be in a page, so these addresses are far
    // apart.
    Result(cat::abs(intptr{&stack_variable} -
                    intptr{&allocator.get(small_memory_1)}) > 512)
        .or_exit();
    allocator.free(small_memory_1);

    // Small-size handles have unique storage.
    auto small_memory_2 = allocator.inline_alloc<int4>().value();
    allocator.get(small_memory_2) = 1;
    auto small_memory_3 = allocator.inline_alloc<int4>().value();
    allocator.get(small_memory_3) = 2;
    auto small_memory_4 = allocator.inline_alloc<int4>().value();
    allocator.get(small_memory_4) = 3;
    Result(allocator.get(small_memory_2) == 1).or_exit();
    Result(allocator.get(small_memory_3) == 2).or_exit();
    Result(allocator.get(small_memory_4) == 3).or_exit();

    // Test constructor being called.
    cat::Optional testtype = allocator.alloc<TestType>();
    allocator.free(testtype.value());

    // That constructor increments `global_int_1`.
    Result(global_int_1 == 1).or_exit();
    // That destructor increments `global_int_2`.
    Result(global_int_2 == 1).or_exit();

    auto smalltesttype = allocator.inline_alloc<TestType>().value();
    allocator.get(smalltesttype) = TestType{};
    allocator.free(smalltesttype);

    // Aligned memory allocations.
    auto aligned_mem = allocator.align_alloc<int4>(32u, 4).value();
    allocator.get(aligned_mem) = 10;
    Result(allocator.get(aligned_mem) == 10).or_exit();
    allocator.free(aligned_mem);

    cat::exit();
};
