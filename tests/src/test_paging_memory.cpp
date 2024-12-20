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
}  // namespace

test(paging_memory) {
   // Initialize an allocator.
   cat::page_allocator allocator;

   // Allocate a page.
   cat::span memory =
      allocator.alloc_multi<int4>(1'000u).or_exit("Failed to page memory!");
   // Free the page at the end of this program.
   defer {
      allocator.free(memory);
   };

   // Write to the page.
   memory[0] = 10;
   cat::verify(memory[0] == 10);

   // `allocation_type` with small-size optimization.
   auto small_memory_1 = allocator.inline_alloc<int4>().verify();
   allocator.get(small_memory_1) = 2;
   // Both values should be on stack, so these addresses are close
   // together.
   cat::verify(small_memory_1.is_inline());
   // The handle's address should be the same as the data's if it was
   // allocated on the stack.
   int4& intref = *reinterpret_cast<int4*>(&small_memory_1);
   intref = 10;
   cat::verify(allocator.get(small_memory_1) == 10);

   allocator.free(small_memory_1);

   auto small_memory_5 = allocator.inline_alloc_multi<int4>(1'000u).verify();
   // `small_memory_1` should be larger than the small storage buffer.
   cat::verify(!small_memory_5.is_inline());
   allocator.free(small_memory_1);

   // Small-size handles have unique storage.
   auto small_memory_2 = allocator.inline_alloc<int4>().verify();
   allocator.get(small_memory_2) = 1;
   auto small_memory_3 = allocator.inline_alloc<int4>().verify();
   allocator.get(small_memory_3) = 2;
   auto small_memory_4 = allocator.inline_alloc<int4>().verify();
   allocator.get(small_memory_4) = 3;
   cat::verify(allocator.get(small_memory_2) == 1);
   cat::verify(allocator.get(small_memory_3) == 2);
   cat::verify(allocator.get(small_memory_4) == 3);

   // Test constructor being called.
   cat::maybe testtype = allocator.alloc<test_page_type>();
   allocator.free(testtype.value());

   // That constructor increments `paging_counter_ctor`.
   cat::verify(paging_counter_ctor == 1);
   // That destructor increments `paging_counter_dtor`.
   cat::verify(paging_counter_dtor == 1);

   // Test multi-allocations.
   auto array_memory = allocator.alloc_multi<test_page_type>(9u).verify();
   // Those 9 constructors increment `paging_counter_ctor`.
   cat::verify(paging_counter_ctor == 10);

   allocator.free(array_memory);
   // Those 9 destructors increment `paging_counter_dtor`.
   cat::verify(paging_counter_dtor == 10);

   auto smalltesttype = allocator.inline_alloc<test_page_type>().verify();
   allocator.get(smalltesttype) = test_page_type();
   allocator.free(smalltesttype);

   // Aligned memory allocations.
   auto aligned_mem = allocator.align_alloc_multi<int4>(32u, 4u).verify();
   aligned_mem[0] = 10;
   cat::verify(aligned_mem[0] == 10);
   allocator.free(aligned_mem);
};
