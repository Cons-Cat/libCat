#include <allocators>
#include <linux>
#include <thread>

void function(void*) {
    for (int4 i = 0; i < 15; i++) {
        write(2, "Moo?\n", 5).discard_result();
    }
    exit();
}

void meow() {
    Thread thread;
    PageAllocator allocator;
    thread.create(allocator, 4096, function, nullptr).or_panic();
    for (int4 i = 0; i < 10; i++) {
        write(2, "Boo!\n", 5).discard_result();
    }
    _ = thread.join().or_panic();
    write(2, "Finished!\n", 10).discard_result();
    exit();
}
