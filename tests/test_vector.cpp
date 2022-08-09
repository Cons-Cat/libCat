#include <cat/linear_allocator>
#include <cat/page_allocator>
#include <cat/vector>

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

auto main() -> int {
    cat::PageAllocator page_allocator;
    cat::Byte* p_page = page_allocator.p_alloc_multi<cat::Byte>(4_ki).or_exit();
    cat::LinearAllocator allocator = {p_page, 4_ki - 32};

    // Test default constructing a `Vector`.
    cat::Vector<int4> int_vec;
    Result(int_vec.size() == 0).or_exit();
    Result(int_vec.capacity() == 0).or_exit();

    // Test pushing back to a `Vector`.
    int_vec.push_back(allocator, 1).or_exit();
    int_vec.push_back(allocator, 2).or_exit();
    int_vec.push_back(allocator, 3).or_exit();
    Result(int_vec.size() == 3).or_exit();
    Result(int_vec.capacity() == 4).or_exit();

    int_vec.push_back(allocator, 6).or_exit();
    int_vec.push_back(allocator, 12).or_exit();
    int_vec.push_back(allocator, 24).or_exit();
    Result(int_vec.size() == 6).or_exit();
    Result(int_vec.capacity() == 8).or_exit();

    // Test resizing a `Vector`.
    int_vec.resize(allocator, 0).or_exit();
    Result(int_vec.size() == 0).or_exit();
    Result(int_vec.capacity() == 8).or_exit();

    int_vec.resize(allocator, 4).or_exit();
    Result(int_vec.size() == 4).or_exit();
    Result(int_vec.capacity() == 8).or_exit();

    // Test reserving storage for a `Vector`.
    int_vec.reserve(allocator, 128).or_exit();
    Result(int_vec.size() == 4).or_exit();
    Result(int_vec.capacity() == 128).or_exit();

    // Test reserve constructor.
    cat::Vector reserved_vec =
        cat::Vector<int4>::reserved(allocator, 6).or_exit();
    Result(reserved_vec.capacity() == 6).or_exit();

    // Test filled constructor.
    cat::Vector filled_vec =
        cat::Vector<int4>::filled(allocator, 8, 1).or_exit();
    Result(filled_vec.size() == 8).or_exit();
    Result(filled_vec.capacity() == 8).or_exit();
    for (int4 integer : filled_vec) {
        Result(integer == 1).or_exit();
    }

    // Test cloned constructor.
    cat::Vector cloned_vec =
        cat::Vector<int4>::cloned(allocator, filled_vec).or_exit();
    Result(cloned_vec.size() == 8).or_exit();
    Result(cloned_vec.capacity() == 8).or_exit();
    for (int4 integer : cloned_vec) {
        Result(integer == 1).or_exit();
    }

    // Test from constructor.
    cat::Vector from_vec_1 =
        cat::Vector<int4>::from(allocator, cat::value_list<int4, 5, 10>)
            .value();
    Result(from_vec_1.capacity() == 10).or_exit();
    Result(from_vec_1.size() == 10).or_exit();
    cat::Vector from_vec_2 =
        cat::Vector<int4>::from(allocator, 1, 2, 3, 4).value();
    Result(from_vec_2.capacity() == 4).or_exit();
    Result(from_vec_2.size() == 4).or_exit();

    // Test `Vector` in a `constexpr` context.
    static_assert(const_func() == 10);

    // TODO: Test insert iterators.
}
