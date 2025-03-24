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

void
function_1() {
   for (idx i = 0; i < 3; ++i) {
      ++atomic;
      ++tls1;
      ++tls2;
   }
   cat::verify(tls1 == 4);
   cat::verify(tls2 == 5);
}

void
function_2() {
   ++atomic;
}

}  // namespace

test(thread) {
   cat::thread threads[5];
   cat::page_allocator allocator;

   threads[0].spawn(allocator, 2_uki, 2_uki, function_1).verify();
   threads[1].spawn(allocator, 2_uki, 2_uki, &function_2).verify();

   threads[2]
      .spawn(allocator, 2_uki, 2_uki,
             [] {
                ++atomic;
             })
      .verify();

   threads[3]
      .spawn(
         allocator, 2_uki, 2_uki,
         [](int) {
            ++atomic;
         },
         1)
      .verify();

   threads[4]
      .spawn(
         allocator, 2_uki, 2_uki,
         +[] {
            ++atomic;
         })
      .verify();

   for (idx i; i < 3; ++i) {
      ++atomic;
      // ++tls1;
   }
   // TODO: Initialize tls on the main thread.
   // cat::verify(tls1 == 4);

   threads[0].join().verify();
   threads[1].join().verify();
   threads[2].join().verify();
   threads[3].join().verify();
   threads[4].join().verify();
   cat::verify(atomic.load() == 10);
}
