#include <cat/insert_iterators>
#include <cat/linear_allocator>
#include <cat/list>
#include <cat/page_allocator>

#include "../unit_tests.hpp"

TEST(test_list) {
    // Initialize an allocator.
    cat::page_allocator pager;
    pager.reset();
    cat::mem auto page = pager.opq_alloc_multi<cat::byte>(4_ki - 64).verify();
    defer(pager.free(page);)
    auto allocator = cat::linear_allocator::backed_handle(pager, page);

    // Test insert.
    cat::list<int4> list_1;
    _ = list_1.insert(allocator, list_1.begin(), 3).verify();
    _ = list_1.insert(allocator, list_1.begin(), 2).verify();
    _ = list_1.insert(allocator, list_1.begin(), 1).verify();
    cat::verify(list_1.front() == 1);
    cat::verify(list_1.back() == 3);

    // Test iteration.
    int i = 1;
    for (auto& node : list_1) {
        cat::verify(node == i);
        ++i;
    }

    list_1.pop_front(allocator);
    cat::verify(list_1.front() == 2);
    list_1.pop_back(allocator);
    cat::verify(list_1.back() == 2);

    // Test push.
    cat::list<int4> list_2;
    _ = list_2.push_front(allocator, 0).verify();
    _ = list_2.push_back(allocator, 4).verify();
    cat::verify(list_2.front() == 0);
    cat::verify(list_2.back() == 4);
    _ = list_2.insert(allocator, ++list_2.begin(), 1).verify();
    cat::verify(list_2.front() == 0);
    cat::verify(*++list_2.begin() == 1);

    // Test iteration.
    for ([[maybe_unused]] int4 _ : list_2) {
    }

    // Test emplace.
    cat::list<int4> list_3;
    _ = list_3.emplace_front(allocator, 1).verify();
    _ = list_3.emplace_front(allocator, 2).verify();
    _ = list_3.emplace_back(allocator, 3).verify();
    _ = list_3.emplace(allocator, ++list_3.begin(), 4);
    cat::verify(list_3.front() == 2);
    cat::verify(list_3.back() == 3);
    cat::verify((*(++list_3.begin())) == 4);

    for (auto& node : list_3) {
        // Iterate.
        node = node;  // NOLINT
    }

    // Test special iterators.
    _ = list_1.emplace(allocator, list_1.begin()++, 0);
    _ = list_1.cbegin();
    _ = list_1.cend();
    _ = list_1.rbegin();
    _ = list_1.rend();
    auto iter = list_1.crbegin();
    _ = list_1.crend();
    cat::verify(*iter == 2);
    ++iter;
    cat::verify(*iter == 0);

    // Test freeing nodes.
    list_1.erase(allocator, list_1.begin());
    for (int i = 0; i < 10; ++i) {
        list_1.pop_front(allocator);
    }

    list_1.clear(allocator);
    list_2.clear(allocator);

    // Deep copy a `list`.
    _ = list_1.push_front(allocator, 3).verify();
    _ = list_1.push_front(allocator, 2).verify();
    _ = list_1.push_front(allocator, 1).verify();
    _ = list_1.push_front(allocator, 0).verify();
    cat::list list_5 = list_1.clone(allocator).verify();

    auto list_it_1 = list_1.begin();
    auto list_it_5 = list_5.begin();

    // Prove all elements copied correctly.
    while (list_it_1 != list_1.end()) {
        cat::verify(*list_it_1 == *list_it_5);
        ++list_it_1;
        ++list_it_5;
    }

    // Test that the copy was deep.
    list_1.clear(allocator);
    cat::verify(*(list_5.begin()) == 0);
    cat::verify(*(list_5.begin() + 1) == 1);
    cat::verify(*(list_5.begin() + 2) == 2);
    cat::verify(*(list_5.begin() + 3) == 3);

    // Test moving `list`.
    list_1.push_front(allocator, 2);
    list_1.push_front(allocator, 1);
    list_1.push_front(allocator, 0);
    cat::list<int4> list_4 = cat::move(list_1);
    cat::verify(list_4.front() == 0);
    cat::verify(*(list_4.begin() + 1) == 1);
    cat::verify(*(list_4.begin() + 2) == 2);

    // Test initialized `list`.
    [[maybe_unused]] cat::list list_init_1 =
        cat::list<int4>::from(allocator, 1, 2, 3).verify();
    cat::list list_init_2 =
        cat::list<int4>::from(allocator, cat::value_list<int4, 0, 4>).verify();
    for (int4 i : list_init_2) {
        cat::verify(i == 0);
    }

    // Test `forward_list`.
    allocator.reset();
    cat::forward_list<int4> forward_list_1;
    _ = forward_list_1.push_front(allocator, 0).verify();
    _ = forward_list_1.emplace_front(allocator, 1).verify();
    _ = forward_list_1.insert_after(allocator, forward_list_1.begin() + 1, 2)
            .verify();
    _ = forward_list_1.emplace_after(allocator, forward_list_1.end(), 3)
            .verify();

    cat::verify(*forward_list_1.begin() == 1);
    cat::verify(*(forward_list_1.begin() + 1) == 0);
    cat::verify(*(forward_list_1.begin() + 2) == 2);
    cat::verify(*(forward_list_1.begin() + 3) == 3);

    // Deep copy a `forward_list`.
    cat::forward_list<int4> forward_list_2 =
        forward_list_1.clone(allocator).verify();
    auto forward_it_1 = forward_list_1.begin();
    auto forward_it_2 = forward_list_2.begin();

    // Prove all elements copied correctly.
    while (forward_it_1 != forward_list_1.end()) {
        cat::verify(*forward_it_1 == *forward_it_2);
        ++forward_it_1;
        ++forward_it_2;
    }

    // Remove elements from `forward_list`.
    forward_list_1.erase_after(allocator, forward_list_1.begin());
    cat::verify(*(forward_list_1.begin() + 1) == 2);

    forward_list_1.pop_front(allocator);
    cat::verify(*forward_list_1.begin() == 2);

    // Test that the copy was deep, after modifying the original.
    cat::verify(*(forward_list_2.begin()) == 1);
    cat::verify(*(forward_list_2.begin() + 1) == 0);
    cat::verify(*(forward_list_2.begin() + 2) == 2);
    cat::verify(*(forward_list_2.begin() + 3) == 3);

    // Test `back_insert_iterator`.
    cat::list<int4> back_list;
    cat::back_insert_iterator back_iterator(back_list);
    cat::front_insert_iterator front_iterator(back_list);
    back_iterator.insert(allocator, 10);
    cat::verify(back_list.front() == 10);

    front_iterator.insert(allocator, 2);
    cat::verify(back_list.front() == 2);
    cat::verify(back_list.back() == 10);
}
