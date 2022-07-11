#include <cat/allocators>
#include <cat/bit>
#include <cat/linux>
#include <cat/runtime>
#include <cat/string>
#include <cat/thread>

void function(void*) {
    for (int4 i = 0; i < 15; ++i) {
        _ = cat::println("Moo?");
    }
    cat::exit();
}

// ASan causes this program to hang with a mysterious call to
// `AsanOnDeadlySignal()`.
[[gnu::no_sanitize_address]] int main() {
    cat::Thread thread;
    cat::PageAllocator allocator;
    thread.create(allocator, 4_ki, function, nullptr)
        .or_exit(
            // "Failed to make thread!"
        );
    for (int4 i = 0; i < 10; ++i) {
        _ = cat::println("Boo!");
    }
    thread.join().or_exit(
        // "Failed to join thread!"
    );
    _ = cat::println("Finished!");
    cat::exit();
}
