#include <cat/array>
#include <cat/math>
#include <cat/page_allocator>
#include <cat/utility>

#include "../unit_tests.hpp"

namespace {
constinit int4 paging_counter_ctor = 0;
constinit int4 paging_counter_dtor = 0;

struct test_page_type {
   test_page_type() {
      ++paging_counter_ctor;
   }

   ~test_page_type() {
      ++paging_counter_dtor;
   }
};

void
reset_paging_counters() {
   paging_counter_ctor = 0;
   paging_counter_dtor = 0;
}
}  // namespace

$test(paging_memory_alloc_multi) {
   cat::page_allocator allocator;

   cat::span page =
      allocator.alloc_multi<int4>(1'000u).or_exit("Failed to page memory!");
   $defer {
      allocator.free(page);
   };

   page[0] = 10;
   cat::verify(page[0] == 10);
}

$test(paging_memory_inline_alloc) {
   cat::page_allocator allocator;

   auto small_memory = allocator.inline_alloc<int4>().verify();
   $defer {
      allocator.free(small_memory);
   };

   allocator.get(small_memory) = 2;
   cat::verify(small_memory.is_inline());
   int4& intref = *reinterpret_cast<int4*>(&small_memory);
   intref = 10;
   cat::verify(allocator.get(small_memory) == 10);
}

$test(paging_memory_inline_alloc_multi_overflow) {
   cat::page_allocator allocator;

   auto large_inline_request =
      allocator.inline_alloc_multi<int4>(1'000u).verify();
   $defer {
      allocator.free(large_inline_request);
   };

   cat::verify(!large_inline_request.is_inline());
}

$test(paging_memory_inline_unique_storage) {
   cat::page_allocator allocator;

   auto small_memory_1 = allocator.inline_alloc<int4>().verify();
   auto small_memory_2 = allocator.inline_alloc<int4>().verify();
   auto small_memory_3 = allocator.inline_alloc<int4>().verify();
   $defer {
      allocator.free(small_memory_1);
      allocator.free(small_memory_2);
      allocator.free(small_memory_3);
   };

   allocator.get(small_memory_1) = 1;
   allocator.get(small_memory_2) = 2;
   allocator.get(small_memory_3) = 3;
   cat::verify(allocator.get(small_memory_1) == 1);
   cat::verify(allocator.get(small_memory_2) == 2);
   cat::verify(allocator.get(small_memory_3) == 3);
}

$test(paging_memory_ctor_dtor_single) {
   cat::page_allocator allocator;
   reset_paging_counters();

   cat::maybe testtype = allocator.alloc<test_page_type>();
   allocator.free(testtype.value());

   cat::verify(paging_counter_ctor == 1);
   cat::verify(paging_counter_dtor == 1);
}

$test(paging_memory_ctor_dtor_multi) {
   cat::page_allocator allocator;
   reset_paging_counters();

   auto array_memory = allocator.alloc_multi<test_page_type>(9u).verify();
   cat::verify(paging_counter_ctor == 9);

   allocator.free(array_memory);
   cat::verify(paging_counter_dtor == 9);
}

$test(paging_memory_inline_ctor_dtor) {
   cat::page_allocator allocator;

   auto smalltesttype = allocator.inline_alloc<test_page_type>().verify();
   allocator.get(smalltesttype) = test_page_type();
   allocator.free(smalltesttype);
}

$test(paging_memory_aligned) {
   cat::page_allocator allocator;

   auto aligned_mem = allocator.align_alloc_multi<int4>(32u, 4u).verify();
   $defer {
      allocator.free(aligned_mem);
   };

   aligned_mem[0] = 10;
   cat::verify(aligned_mem[0] == 10);
}
