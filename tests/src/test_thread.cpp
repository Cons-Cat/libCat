#include <cat/atomic>
#include <cat/bit>
#include <cat/linux>
#include <cat/page_allocator>
#include <cat/runtime>
#include <cat/string>
#include <cat/thread>

#include "../unit_tests.hpp"

// TODO: Test this without I/O.

namespace {

cat::atomic<int> atomic;

void
function() {
   for (idx i = 0; i < 3; ++i) {
      ++atomic;
   }
}

}  // namespace

TEST(test_thread) {
   atomic.store(0);
   cat::thread thread;
   cat::page_allocator allocator;

   thread.spawn(allocator, 2_uki, function).or_exit("Failed to make thread!");
   for (idx i; i < 3; ++i) {
      ++atomic;
   }

   thread.join().or_exit("Failed to join thread!");
   cat::verify(atomic.load() == 6);
}
