#include <cat/linear_allocator>
#include <cat/page_allocator>
#include <cat/ring>

#include "../unit_tests.hpp"
#include "cat/debug"

TEST(test_ring) {
    // Initialize an allocator.
    cat::page_allocator pager;
    pager.reset();
    auto page = pager.opq_alloc_multi<cat::byte>(4_ki - 32).or_exit();
    defer(pager.free(page);)
    auto allocator =
        cat::linear_allocator::backed_handle(pager, page);

    cat::Ring<int4> ring_int4;
    cat::verify(ring_int4.size() == 0);
    cat::verify(ring_int4.capacity() == 0);

    // Push onto ring.
    _ = ring_int4.reserve(allocator, 4).verify();
    ring_int4.push_back(1);
    ring_int4.push_back(3);
    ring_int4.push_back(2);
    ring_int4.push_back(0);

    cat::verify(ring_int4[0] == 1);
    cat::verify(ring_int4[1] == 3);
    cat::verify(ring_int4[2] == 2);
    cat::verify(ring_int4[3] == 0);

    // Wrapping `.push_back()`.
    ring_int4.push_back(20);
    cat::verify(ring_int4[0] == 20);

    // Set element.
    ring_int4[1] = 10;
    cat::verify(ring_int4[1] == 10);

    ring_int4.at(1).value() = 5;
    cat::verify(ring_int4[1] == 5);
}
