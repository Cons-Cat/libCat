#include <cat/bit>
#include <cat/linux>
#include <cat/page_allocator>
#include <cat/runtime>
#include <cat/string>
#include <cat/thread>

#include "../unit_tests.hpp"

// TODO: Test this without I/O.

void function(void*) {
    for (int4 i = 0; i < 15; ++i) {
        // _ = cat::println("Moo?");
    }
    cat::exit();
}

TEST(test_thread) {
    cat::Thread thread;
    cat::PageAllocator allocator;
    thread.create(allocator, 2_ki, function, nullptr)
        .or_exit("Failed to make thread!");
    for (int4 i = 0; i < 10; ++i) {
        // _ = cat::println("Boo!");
    }
    thread.join().or_exit("Failed to join thread!");
    // _ = cat::println("Finished!");
}
