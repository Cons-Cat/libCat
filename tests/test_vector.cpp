#include <cat/vector>

void meow() {
    cat::PageAllocator page_allocator;
    cat::Byte* page = page_allocator.p_malloc(4_ki).or_panic();
    cat::LinearAllocator allocator = {page, 4_ki};

    // Test default constructing a `Vector`.
    cat::Vector<int4> int_vec;
    Result(int_vec.size() == 0).or_panic();
    Result(int_vec.capacity() == 0).or_panic();

    // Test pushing back to a `Vector`.
    int_vec.push_back(allocator, 1).or_panic();
    int_vec.push_back(allocator, 2).or_panic();
    int_vec.push_back(allocator, 3).or_panic();
    Result(int_vec.size() == 3).or_panic();
    Result(int_vec.capacity() == 4).or_panic();

    int_vec.push_back(allocator, 6).or_panic();
    int_vec.push_back(allocator, 12).or_panic();
    int_vec.push_back(allocator, 24).or_panic();
    Result(int_vec.size() == 6).or_panic();
    Result(int_vec.capacity() == 8).or_panic();

    // Test resizing a `Vector`.
    int_vec.resize(allocator, 0).or_panic();
    Result(int_vec.size() == 0).or_panic();
    Result(int_vec.capacity() == 8).or_panic();

    // Test reserving storage for a `Vector`.
    int_vec.reserve(allocator, 128).or_panic();
    Result(int_vec.size() == 0).or_panic();
    Result(int_vec.capacity() == 128).or_panic();

    // Test reserve constructor.
    cat::Vector reserved_vec =
        cat::Vector<int4>::reserved(allocator, 6).or_panic();
    Result(reserved_vec.capacity() == 6).or_panic();

    // Test filled constructor.
    cat::Vector filled_vec =
        cat::Vector<int4>::filled(allocator, 8, 1).or_panic();
    Result(filled_vec.size() == 8).or_panic();
    Result(filled_vec.capacity() == 8).or_panic();
    for (int4 integer : filled_vec) {
        Result(integer == 1).or_panic();
    }

    // Test cloned constructor.
    cat::Vector cloned_vec =
        cat::Vector<int4>::cloned(allocator, filled_vec).or_panic();
    Result(cloned_vec.size() == 8).or_panic();
    Result(cloned_vec.capacity() == 8).or_panic();
    for (int4 integer : cloned_vec) {
        Result(integer == 1).or_panic();
    }

    cat::exit();
}
