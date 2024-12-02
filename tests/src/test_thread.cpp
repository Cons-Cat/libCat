#include <cat/atomic>
#include <cat/bit>
#include <cat/linux>
#include <cat/page_allocator>
#include <cat/runtime>
#include <cat/string>
#include <cat/thread>

#include "../unit_tests.hpp"

namespace {

constinit cat::atomic<int> atomic = 0;

thread_local int tls1 = 1;
thread_local int tls2 = 2;

// TODO: Why does this segfault with asan enabled?
[[gnu::no_sanitize_address]]
void
function() {
   for (idx i = 0; i < 3; ++i) {
      ++atomic;
      ++tls1;
      ++tls2;
   }
   cat::verify(tls1 == 4);
   cat::verify(tls2 == 5);
}

}  // namespace

TEST(test_thread) {
   cat::thread thread;
   cat::page_allocator allocator;

   thread.spawn(allocator, 2_uki, 2_uki, function).verify();
   for (idx i; i < 3; ++i) {
      ++atomic;
      // ++tls1;
   }
   // TODO: Initialize tls on the main thread.
   // cat::verify(tls1 == 4);

   thread.join().verify();
   cat::verify(atomic.load() == 6);
}
