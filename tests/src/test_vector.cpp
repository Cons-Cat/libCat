#include <cat/linear_allocator>
#include <cat/page_allocator>
#include <cat/vector>

#include "../unit_tests.hpp"

// Test that `vector` works in a `constexpr` context.
consteval auto const_func() -> int4 {
    cat::page_allocator allocator;
    cat::vector<int4> vector;
    _ = vector.resize(allocator, 8);

    vector[0] = 1;
    vector[1] = 2;
    vector[7] = 10;
    _ = vector.push_back(allocator, 10);
    return vector[8];
}

TEST(test_vector) {
    // Initialize an allocator.
    cat::page_allocator pager;
    pager.reset();
    cat::mem auto page = pager.opq_alloc_multi<cat::byte>(4_ki - 32).or_exit();
    defer(pager.free(page);)
    auto allocator = cat::linear_allocator::backed_handle(pager, page);

    // Test vector member types.
    using iterator = cat::vector<int>::iterator;
    static_assert(cat::is_same<iterator, decltype(cat::vector<int>().begin())>);
    static_assert(cat::is_same<iterator, decltype(cat::vector<int>().end())>);

    using const_iterator = cat::vector<int>::const_iterator;
    static_assert(
        cat::is_same<const_iterator, decltype(cat::vector<int>().cbegin())>);
    static_assert(
        cat::is_same<const_iterator, decltype(cat::vector<int>().cend())>);

    using reverse_iterator = cat::vector<int>::reverse_iterator;
    static_assert(
        cat::is_same<reverse_iterator, decltype(cat::vector<int>().rbegin())>);
    static_assert(
        cat::is_same<reverse_iterator, decltype(cat::vector<int>().rend())>);

    using const_reverse_iterator = cat::vector<int>::const_reverse_iterator;
    static_assert(cat::is_same<const_reverse_iterator,
                               decltype(cat::vector<int>().crbegin())>);
    static_assert(cat::is_same<const_reverse_iterator,
                               decltype(cat::vector<int>().crend())>);

    static_assert(cat::is_same<int, cat::vector<int>::value_type>);

    // Test default constructing a `vector`.
    cat::vector<int4> int_vec;
    cat::verify(int_vec.size() == 0);
    cat::verify(int_vec.capacity() >= 0);

    // Test pushing back to a `vector`.
    int_vec.push_back(allocator, 1).or_exit();
    int_vec.push_back(allocator, 2).or_exit();
    int_vec.push_back(allocator, 3).or_exit();
    cat::verify(int_vec.size() == 3);
    cat::verify(int_vec.capacity() >= 4);

    int_vec.push_back(allocator, 6).or_exit();
    int_vec.push_back(allocator, 12).or_exit();
    int_vec.push_back(allocator, 24).or_exit();
    cat::verify(int_vec.size() == 6);
    cat::verify(int_vec.capacity() >= 8);

    // Test resizing a `vector`.
    int_vec.resize(allocator, 0).or_exit();
    cat::verify(int_vec.size() == 0);
    cat::verify(int_vec.capacity() >= 8);

    int_vec.resize(allocator, 4).or_exit();
    cat::verify(int_vec.size() == 4);
    cat::verify(int_vec.capacity() >= 8);

    // Test reserving storage for a `vector`.
    int_vec.reserve(allocator, 128).or_exit();
    cat::verify(int_vec.size() == 4);
    cat::verify(int_vec.capacity() >= 128);

    // Test reserve constructor.
    cat::vector reserved_vec =
        cat::vector<int4>::reserved(allocator, 6).or_exit();
    cat::verify(reserved_vec.capacity() >= 6);

    // Test filled constructor.
    cat::vector filled_vec =
        cat::vector<int4>::filled(allocator, 8, 1).or_exit();
    cat::verify(filled_vec.size() == 8);
    cat::verify(filled_vec.capacity() >= 8);
    for (int4 integer : filled_vec) {
        cat::verify(integer == 1);
    }

    // Test cloned constructor.
    cat::vector cloned_vec = filled_vec.clone(allocator).or_exit();
    cat::verify(cloned_vec.size() == 8);
    cat::verify(cloned_vec.capacity() >= 8);
    for (int4 integer : cloned_vec) {
        cat::verify(integer == 1);
    }

    // Test `vector` in a `constexpr` context.
    static_assert(const_func() == 10);

    // Test getters.
    cat::vector<int> default_vector;
    cat::verify(default_vector.is_empty());

    _ = default_vector.reserve(allocator, 2);
    cat::verify(default_vector.is_empty());

    _ = default_vector.push_back(allocator, 0);
    _ = default_vector.push_back(allocator, 0);
    cat::verify(!default_vector.is_empty());

    // Resize the vector to be larger, then check it's full.
    _ = default_vector.resize(allocator, default_vector.capacity() + 1)
            .verify();
    cat::verify(default_vector.is_full());

    // Resize the vector to be smaller, then check it's not full.
    _ = default_vector.resize(allocator, 2).verify();
    cat::verify(!default_vector.is_full());

    // TODO: Test insert iterators.

    // Test algorithms.
    cat::vector origin_vector =
        cat::vector<int>::filled(allocator, 6, 1).verify();
    auto copy_vector = cat::vector<int>::filled(allocator, 6, 0).verify();
    auto move_vector = cat::vector<int>::filled(allocator, 6, 0).verify();
    auto relocate_vector = cat::vector<int>::filled(allocator, 6, 0).verify();

    // `copy()`.
    cat::verify(copy_vector[5] == 0);
    cat::copy(origin_vector.begin(), origin_vector.end(), copy_vector.begin());
    cat::verify(copy_vector[5] == 1);

    copy_vector[5] = 0;
    origin_vector.copy_to(copy_vector);
    cat::verify(copy_vector[5] == 1);

    // `move()`.
    cat::verify(move_vector[5] == 0);
    cat::move(origin_vector.begin(), origin_vector.end(), move_vector.begin());
    cat::verify(move_vector[5] == 1);

    move_vector[5] = 0;
    origin_vector.move_to(move_vector);
    cat::verify(move_vector[5] == 1);

    // `relocate()`.
    cat::verify(relocate_vector[5] == 0);
    cat::relocate(origin_vector.begin(), origin_vector.end(),
                  relocate_vector.begin());
    cat::verify(relocate_vector[5] == 1);

    relocate_vector[5] = 0;
    origin_vector.relocate_to(relocate_vector);
    cat::verify(relocate_vector[5] == 1);
}
