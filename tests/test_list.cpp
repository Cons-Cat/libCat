#include <cat/allocators>
#include <cat/list>

void meow() {
    cat::align_stack_pointer_32();
    cat::PageAllocator page_allocator;
    auto page = page_allocator.malloc(4_ki).value();
    cat::LinearAllocator allocator = {&page_allocator.get(page), 1_ki};

    cat::List<int4> list_1;
    _ = list_1.insert(allocator, list_1.begin(), 3);
    _ = list_1.insert(allocator, list_1.begin(), 2);
    _ = list_1.insert(allocator, list_1.begin(), 1);
    Result(list_1.front() == 1).or_panic();
    Result(list_1.back() == 3).or_panic();
    _ = list_1.crbegin();

    _ = page_allocator.free(page);
    cat::exit();
}
