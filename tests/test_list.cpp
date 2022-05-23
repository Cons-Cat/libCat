#include <cat/allocators>
#include <cat/list>

void meow() {
    cat::align_stack_pointer_32();
    cat::PageAllocator page_allocator;
    auto page = page_allocator.malloc(4_ki).or_panic();
    cat::LinearAllocator allocator = {&page_allocator.get(page), 4_ki};

    // Test insert.
    cat::List<int4> list_1;
    _ = list_1.insert(allocator, list_1.begin(), 3).or_panic();
    _ = list_1.insert(allocator, list_1.begin(), 2).or_panic();
    _ = list_1.insert(allocator, list_1.begin(), 1).or_panic();
    Result(list_1.front() == 1).or_panic();
    Result(list_1.back() == 3).or_panic();

    for (auto& node : list_1) {
        // Iterate.
        node = node;  // NOLINT
    }

    list_1.pop_front(allocator);
    Result(list_1.front() == 2).or_panic();
    list_1.pop_back(allocator);
    Result(list_1.back() == 2).or_panic();

    // Test push.
    cat::List<int4> list_2;
    _ = list_2.push_front(allocator, 0).or_panic();
    _ = list_2.push_back(allocator, 4).or_panic();
    Result(list_2.front() == 0).or_panic();
    Result(list_2.back() == 4).or_panic();
    _ = list_2.insert(allocator, ++list_2.begin(), 1);
    Result(list_2.front() == 0).or_panic();
    Result(*++list_2.begin() == 1).or_panic();

    for (auto& node : list_2) {
        // Iterate.
        node = node;  // NOLINT
    }

    // Test emplace.
    cat::List<int4> list_3;
    _ = list_3.emplace_front(allocator, 1).or_panic();
    _ = list_3.emplace_front(allocator, 2).or_panic();
    _ = list_3.emplace_back(allocator, 3).or_panic();
    _ = list_3.emplace(allocator, ++list_3.begin(), 4);
    Result(list_3.front() == 2).or_panic();
    Result(list_3.back() == 3).or_panic();
    Result((*(++list_3.begin())) == 4).or_panic();

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
    Result(*iter == 3);
    ++iter;
    Result(*iter == 2);

    // Test freeing nodes.
    list_1.erase(allocator, list_1.begin());
    for (int i = 0; i < 10; i++) {
        list_1.pop_front(allocator);
    }

    list_2.clear(allocator);

    // Test `ForwardList`.
    allocator.reset();
    cat::ForwardList<int4> forward_list_1;
    _ = forward_list_1.push_front(allocator, 0).or_panic();
    _ = forward_list_1.emplace_front(allocator, 1).or_panic();
    _ = forward_list_1.insert_after(allocator, forward_list_1.begin() + 1, 2)
            .or_panic();
    _ = forward_list_1.emplace_after(allocator, forward_list_1.end(), 3)
            .or_panic();

    Result(*forward_list_1.begin() == 1).or_panic();
    Result(*(forward_list_1.begin() + 1) == 0).or_panic();
    Result(*(forward_list_1.begin() + 2) == 2).or_panic();
    Result(*(forward_list_1.begin() + 3) == 3).or_panic();

    forward_list_1.erase_after(allocator, forward_list_1.begin());
    Result(*(forward_list_1.begin() + 1) == 2).or_panic();

    forward_list_1.pop_front(allocator);
    Result(*forward_list_1.begin() == 2).or_panic();

    _ = page_allocator.free(page);
    cat::exit();
}
