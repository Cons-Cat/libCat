#include <page_allocator.h>
#include <thread.h>
#include <unistd.h>

void function(void*) {
    for (i4 i = 0; i < 10; i++) {
        write(2, u8"Moo!\n", 5).discard_result();
    }
    write(2, u8"Boo!\n", 5).discard_result();
    exit(0);
}

void meow() {
    Thread thread;
    PageAllocator allocator;
    struct function_arguments {
    } args;
    thread.create(allocator, 4096, function, &args).or_panic();
    for (i4 i = 0; i < 10; i++) {
        write(2, u8"Foo!\n", 5).discard_result();
    }
    thread.join().discard_result();
    exit(0);
}
