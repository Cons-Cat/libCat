#include <cat/linear_allocator>
#include <cat/page_allocator>
#include <cat/vec>

#include "../unit_tests.hpp"

// Test that `vec` works in a `constexpr` context.
consteval auto
const_func() -> int4 {
    cat::page_allocator allocator;
    cat::vec<int4> vector;
    auto _ = vector.resize(allocator, 8u);

    vector[0] = 1;
    vector[1] = 2;
    vector[7] = 10;
    auto _ = vector.push_back(allocator, 10);
    return vector[8];
}

TEST(test_vec) {
    // Initialize an allocator.
    cat::page_allocator pager;
    cat::span page = pager.alloc_multi<cat::byte>(4_uki).or_exit();
    defer {
        pager.free(page);
    };
    auto allocator = cat::make_linear_allocator(page);

    // Test vector member types.
    using iterator = cat::vec<int>::iterator;
    static_assert(cat::is_same<iterator, decltype(cat::vec<int>().begin())>);
    static_assert(cat::is_same<iterator, decltype(cat::vec<int>().end())>);

    static_assert(cat::is_same<iterator::value_type, int>);
    static_assert(cat::is_same<iterator::reference, int&>);

    using const_iterator = cat::vec<int>::const_iterator;
    static_assert(
        cat::is_same<const_iterator, decltype(cat::vec<int>().cbegin())>);
    static_assert(
        cat::is_same<const_iterator, decltype(cat::vec<int>().cend())>);

    static_assert(cat::is_same<const_iterator::value_type, int const>);
    static_assert(cat::is_same<const_iterator::reference, int const&>);

    using reverse_iterator = cat::vec<int>::reverse_iterator;
    static_assert(
        cat::is_same<reverse_iterator, decltype(cat::vec<int>().rbegin())>);
    static_assert(
        cat::is_same<reverse_iterator, decltype(cat::vec<int>().rend())>);

    static_assert(cat::is_same<reverse_iterator::value_type, int>);
    static_assert(cat::is_same<reverse_iterator::reference, int&>);

    using const_reverse_iterator = cat::vec<int>::const_reverse_iterator;
    static_assert(cat::is_same<const_reverse_iterator,
                               decltype(cat::vec<int>().crbegin())>);
    static_assert(cat::is_same<const_reverse_iterator,
                               decltype(cat::vec<int>().crend())>);

    static_assert(cat::is_same<const_reverse_iterator::value_type, int const>);
    static_assert(cat::is_same<const_reverse_iterator::reference, int const&>);

    static_assert(cat::is_same<int, cat::vec<int>::value_type>);

    // Test default constructing a `vector`.
    cat::vec<int4> int_vec;
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
    int_vec.resize(allocator, 0u).or_exit();
    cat::verify(int_vec.size() == 0u);
    cat::verify(int_vec.capacity() >= 8u);

    int_vec.resize(allocator, 4u).or_exit();
    cat::verify(int_vec.size() == 4u);
    cat::verify(int_vec.capacity() >= 8u);

    // Test reserving storage for a `vector`.
    int_vec.reserve(allocator, 128u).or_exit();
    cat::verify(int_vec.size() == 4u);
    cat::verify(int_vec.capacity() >= 128u);

    // Test reserve constructor.
    cat::vec reserved_vec = cat::vec<int4>::reserved(allocator, 6u).or_exit();
    cat::verify(reserved_vec.capacity() >= 6u);

    // Test filled constructor.
    cat::vec filled_vec = cat::vec<int4>::filled(allocator, 8u, 1).or_exit();
    cat::verify(filled_vec.size() == 8u);
    cat::verify(filled_vec.capacity() >= 8u);
    for (int4 integer : filled_vec) {
        cat::verify(integer == 1);
    }

    // Test cloned constructor.
    cat::vec cloned_vec = filled_vec.clone(allocator).or_exit();
    cat::verify(cloned_vec.size() == 8u);
    cat::verify(cloned_vec.capacity() >= 8u);
    for (int4 integer : cloned_vec) {
        cat::verify(integer == 1);
    }

    // Test `vector` in a `constexpr` context.
    static_assert(const_func() == 10);

    // Test getters.
    cat::vec<int> default_vector;
    cat::verify(default_vector.is_empty());

    auto _ = default_vector.reserve(allocator, 2u);
    cat::verify(default_vector.is_empty());

    auto _ = default_vector.push_back(allocator, 0);
    auto _ = default_vector.push_back(allocator, 0);
    cat::verify(!default_vector.is_empty());

    // Resize the vector to be larger, then check it's full.
    auto _ = default_vector.resize(allocator, default_vector.capacity() + 1u)
                 .verify();
    cat::verify(default_vector.is_full());

    // Resize the vector to be smaller, then check it's not full.
    auto _ = default_vector.resize(allocator, 2u).verify();
    cat::verify(!default_vector.is_full());

    // TODO: Test insert iterators.

    // Test algorithms.
    cat::vec origin_vector = cat::vec<int>::filled(allocator, 6u, 1).verify();
    auto copy_vector = cat::vec<int>::filled(allocator, 6u, 0).verify();
    auto move_vector = cat::vec<int>::filled(allocator, 6u, 0).verify();
    auto relocate_vector = cat::vec<int>::filled(allocator, 6u, 0).verify();

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