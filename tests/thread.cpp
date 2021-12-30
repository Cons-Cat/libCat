#include <page_allocator.h>
#include <thread.h>
#include <unistd.h>

auto function(void*) -> isize {
    while (true) {
    }
    write(2, u8"Boo!\n", 5).discard_result();
    return 0;
}

void meow() {
    Thread thread;
    PageAllocator allocator;
    struct function_arguments {
    } args;
    thread.create(allocator, 4096, function, &args).or_panic();
    thread.join().or_panic();
    exit(0);
}
