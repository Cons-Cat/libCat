#include <allocators>
#include <bit>
#include <linux>
#include <string>
#include <thread>

void function(void*) {
    for (int4 i = 0; i < 15; i++) {
        cat::print_line("Moo?").discard_result();
    }
    cat::exit();
}

void meow() {
    Thread thread;
    PageAllocator allocator;
    thread.create(allocator, 4_ki, function, nullptr)
        .or_panic("Failed to make thread!");
    for (int4 i = 0; i < 10; i++) {
        cat::print_line("Boo!").discard_result();
    }
    thread.join().or_panic("Failed to join thread!");
    cat::print_line("Finished!").discard_result();
    cat::exit();
}
