#include <cat/allocators>
#include <cat/bit>
#include <cat/linux>
#include <cat/runtime>
#include <cat/string>
#include <cat/thread>

void function(void*) {
    for (int4 i = 0; i < 15; ++i) {
        _ = cat::print_line("Moo?");
    }
    cat::exit();
}

// ASan causes this program to hang with a mysterious call to
// `AsanOnDeadlySignal()`.
[[gnu::no_sanitize_address]] void meow() {
    cat::Thread thread;
    cat::PageAllocator allocator;
    thread.create(allocator, 4_ki, function, nullptr)
        .or_panic(
            // "Failed to make thread!"
        );
    for (int4 i = 0; i < 10; ++i) {
        _ = cat::print_line("Boo!");
    }
    thread.join().or_panic(
        // "Failed to join thread!"
    );
    _ = cat::print_line("Finished!");
    cat::exit();
}
