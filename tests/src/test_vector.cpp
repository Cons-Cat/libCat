#include <cat/linear_allocator>
#include <cat/page_allocator>
#include <cat/vector>

#include "../unit_tests.hpp"

// Test that `Vector` works in a `constexpr` context.
consteval auto const_func() -> int4 {
    cat::Vector<int4> vector;
    vector.resize(8);
    vector[0] = 1;
    vector[1] = 2;
    vector[7] = 2;
    vector.push_back(10);

    // TODO: Make these work.
    // cat::Vector vector_2 = cat::Vector<int4>::cloned(vector);
    // cat::Vector vector_2 = cat::Vector<int4>::from(1, 2, 3);

    return vector[8];
}

TEST(test_vector) {
    // Initialize an allocator.
    cat::PageAllocator paging_allocator;
    paging_allocator.reset();
    auto page = paging_allocator.alloc_multi<cat::Byte>(4_ki - 32).or_exit();
    defer(paging_allocator.free(page);)
    auto allocator =
        cat::LinearAllocator::backed_handle(paging_allocator, page);

    // Test default constructing a `Vector`.
    cat::Vector<int4> int_vec;
    cat::verify(int_vec.size() == 0);
    cat::verify(int_vec.capacity() == 0);

    // Test pushing back to a `Vector`.
    int_vec.push_back(allocator, 1).or_exit();
    int_vec.push_back(allocator, 2).or_exit();
    int_vec.push_back(allocator, 3).or_exit();
    cat::verify(int_vec.size() == 3);
    cat::verify(int_vec.capacity() == 4);

    int_vec.push_back(allocator, 6).or_exit();
    int_vec.push_back(allocator, 12).or_exit();
    int_vec.push_back(allocator, 24).or_exit();
    cat::verify(int_vec.size() == 6);
    cat::verify(int_vec.capacity() == 8);

    // Test resizing a `Vector`.
    int_vec.resize(allocator, 0).or_exit();
    cat::verify(int_vec.size() == 0);
    cat::verify(int_vec.capacity() == 8);

    int_vec.resize(allocator, 4).or_exit();
    cat::verify(int_vec.size() == 4);
    cat::verify(int_vec.capacity() == 8);

    // Test reserving storage for a `Vector`.
    int_vec.reserve(allocator, 128).or_exit();
    cat::verify(int_vec.size() == 4);
    cat::verify(int_vec.capacity() == 128);

    // Test reserve constructor.
    cat::Vector reserved_vec =
        cat::Vector<int4>::reserved(allocator, 6).or_exit();
    cat::verify(reserved_vec.capacity() == 6);

    // Test filled constructor.
    cat::Vector filled_vec =
        cat::Vector<int4>::filled(allocator, 8, 1).or_exit();
    cat::verify(filled_vec.size() == 8);
    cat::verify(filled_vec.capacity() == 8);
    for (int4 integer : filled_vec) {
        cat::verify(integer == 1);
    }

    // Test cloned constructor.
    cat::Vector cloned_vec =
        cat::Vector<int4>::cloned(allocator, filled_vec).or_exit();
    cat::verify(cloned_vec.size() == 8);
    cat::verify(cloned_vec.capacity() == 8);
    for (int4 integer : cloned_vec) {
        cat::verify(integer == 1);
    }

    // Test from constructor.
    cat::Vector from_vec_1 =
        cat::Vector<int4>::from(allocator, cat::value_list<int4, 5, 10>)
            .value();
    cat::verify(from_vec_1.capacity() == 10);
    cat::verify(from_vec_1.size() == 10);
    cat::Vector from_vec_2 =
        cat::Vector<int4>::from(allocator, 1, 2, 3, 4).value();
    cat::verify(from_vec_2.capacity() == 4);
    cat::verify(from_vec_2.size() == 4);

    // Test `Vector` in a `constexpr` context.
    static_assert(const_func() == 10);

    // TODO: Test insert iterators.
}
