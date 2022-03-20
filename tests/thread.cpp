#include <allocators>
#include <linux>
#include <string>
#include <thread>

void function(void*) {
    for (int4 i = 0; i < 15; i++) {
        std::print_line("Moo?").discard_result();
    }
    std::exit();
}

void meow() {
    nix::Process thread;
    PageAllocator allocator;
    thread.create(allocator, 4096, function, nullptr).or_panic();
    for (int4 i = 0; i < 10; i++) {
        std::print_line("Boo!").discard_result();
    }
    _ = thread.join().or_panic();
    std::print_line("Finished!").discard_result();
    std::exit();
}
