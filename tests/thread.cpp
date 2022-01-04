#include <allocators>
#include <linux>
#include <thread>

void function(void*) {
    for (i4 i = 0; i < 15; i++) {
        write(2, u8"Moo?\n", 5).discard_result();
    }
    exit(0);
}

void meow() {
    Thread thread;
    PageAllocator allocator;
    thread.create(allocator, 4096, function, nullptr).or_panic();
    for (i4 i = 0; i < 10; i++) {
        write(2, u8"Boo!\n", 5).discard_result();
    }
    _ = thread.join().or_panic();
    write(2, u8"Finished!\n", 10).discard_result();
    exit(0);
}
