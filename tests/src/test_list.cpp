#include <cat/insert_iterators>
#include <cat/linear_allocator>
#include <cat/list>
#include <cat/page_allocator>

#include "../unit_tests.hpp"

#define TEST(test_name)                                \
    void test_name();                                  \
    [[gnu::constructor]]                               \
    void register_##test_name() {                      \
        /* all_tests.push_back(tests_linear_allocator, 0)*/ \
		/* .or_exit("Ran out of memory!");            */	\
		test_name();\
    }                                                  \
    void test_name()

TEST(test_list) {
	cat::exit(11);
	_ = cat::print("Hi\n");
	
    /*
    // Initialize an allocator.
    cat::PageAllocator paging_allocator;
    paging_allocator.reset();
    auto page = paging_allocator.alloc_multi<cat::Byte>(4_ki - 64).verify(
    test_assert_handler);
    defer(paging_allocator.free(page);)
    auto allocator =
    cat::LinearAllocator::backed_handle(paging_allocator, page);

    // Test insert.
    cat::List<int4> list_1;
    _ = list_1.insert(allocator, list_1.begin(), 3).verify(test_assert_handler);
    _ = list_1.insert(allocator, list_1.begin(), 2).verify(test_assert_handler);
    _ = list_1.insert(allocator, list_1.begin(), 1).verify(test_assert_handler);
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
    cat::List<int4> list_2;
    _ = list_2.push_front(allocator, 0).verify(test_assert_handler);
    _ = list_2.push_back(allocator, 4).verify(test_assert_handler);
    cat::verify(list_2.front() == 0);
    cat::verify(list_2.back() == 4);
    _ = list_2.insert(allocator, ++list_2.begin(), 1)
        .verify(test_assert_handler);
    cat::verify(list_2.front() == 0);
    cat::verify(*++list_2.begin() == 1);

    // Test iteration.
    for ([[maybe_unused]] int4 _ : list_2) {
    }

    // Test emplace.
    cat::List<int4> list_3;
    _ = list_3.emplace_front(allocator, 1).verify(test_assert_handler);
    _ = list_3.emplace_front(allocator, 2).verify(test_assert_handler);
    _ = list_3.emplace_back(allocator, 3).verify(test_assert_handler);
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

    list_2.clear(allocator);

    // Deep copy a `List`.
    _ = list_1.push_front(allocator, 3).verify(test_assert_handler);
    _ = list_1.push_front(allocator, 2).verify(test_assert_handler);
    _ = list_1.push_front(allocator, 1).verify(test_assert_handler);
    _ = list_1.push_front(allocator, 0).verify(test_assert_handler);
    cat::List list_5 =
    cat::List<int4>::cloned(allocator, list_1).verify(test_assert_handler);

    // Test that the copy was deep.
    list_1.clear(allocator);
    cat::verify(*list_5.begin() == 0);
    cat::verify(*(list_5.begin() + 1) == 1);
    cat::verify(*(list_5.begin() + 2) == 2);
    cat::verify(*(list_5.begin() + 3) == 3);

    // Test moving `List`.
    list_1.push_front(allocator, 2);
    list_1.push_front(allocator, 1);
    list_1.push_front(allocator, 0);
    cat::List<int4> list_4 = cat::move(list_1);  // NOLINT
    cat::verify(list_4.front() == 0);
    cat::verify(*(list_4.begin() + 1) == 1);
    cat::verify(*(list_4.begin() + 2) == 2);

    // Test initialized `List`.
    [[maybe_unused]] cat::List list_init_1 =
    cat::List<int4>::from(allocator, 1, 2, 3).verify(test_assert_handler);
    cat::List list_init_2 =
    cat::List<int4>::from(allocator, cat::value_list<int4, 0, 4>)
        .verify(test_assert_handler);
    for (int4 i : list_init_2) {
    cat::verify(i == 0);
    }

    // Test `ForwardList`.
    allocator.reset();
    cat::ForwardList<int4> forward_list_1;
    _ = forward_list_1.push_front(allocator, 0).verify(test_assert_handler);
    _ = forward_list_1.emplace_front(allocator, 1).verify(test_assert_handler);
    _ = forward_list_1.insert_after(allocator, forward_list_1.begin() + 1, 2)
        .verify(test_assert_handler);
    _ = forward_list_1.emplace_after(allocator, forward_list_1.end(), 3)
        .verify(test_assert_handler);

    cat::verify(*forward_list_1.begin() == 1);
    cat::verify(*(forward_list_1.begin() + 1) == 0);
    cat::verify(*(forward_list_1.begin() + 2) == 2);
    cat::verify(*(forward_list_1.begin() + 3) == 3);

    // Deep copy a `ForwardList`.
    cat::ForwardList<int4> forward_list_2;
    forward_list_2.clone(allocator, forward_list_1).verify(test_assert_handler);

    // Remove elements from `ForwardList`.
    forward_list_1.erase_after(allocator, forward_list_1.begin());
    cat::verify(*(forward_list_1.begin() + 1) == 2);

    forward_list_1.pop_front(allocator);
    cat::verify(*forward_list_1.begin() == 2);

    // Test that the copy was deep.
    cat::verify(*forward_list_2.begin() == 1);
    cat::verify(*(forward_list_2.begin() + 1) == 0);
    cat::verify(*(forward_list_2.begin() + 2) == 2);
    cat::verify(*(forward_list_2.begin() + 3) == 3);

    // Test `BackInsertIterator`.
    list_1.clear(allocator);
    cat::BackInsertIterator back_insert_iterator(list_1);
    cat::FrontInsertIterator front_insert_iterator(list_1);
    back_insert_iterator.insert(allocator, 10);
    cat::verify(list_1.front() == 10);

    front_insert_iterator.insert(allocator, 2);
    cat::verify(list_1.front() == 2);
    cat::verify(list_1.back() == 10);
    */
}
