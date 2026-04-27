#include <cat/array>
#include <cat/linear_allocator>
#include <cat/memory>
#include <cat/page_allocator>
#include <cat/span>

#include "../unit_tests.hpp"

test(reallocation_disjoint_intervals) {
   cat::array<unsigned char, 64> buf{};

   unsigned char* const p = buf.data();
   cat::verify(!cat::is_memory_overlapping(p, uword{8u}, p + 16, uword{8u}));
   cat::verify(cat::is_memory_overlapping(p, uword{16u}, p + 8, uword{8u}));
   cat::verify(!cat::is_memory_overlapping(p, uword{0u}, p, uword{8u}));
}

test(reallocation_disjoint_linear_ptr_realloc) {
   cat::page_allocator pager;
   cat::span page = pager.alloc_multi<cat::byte>(512u).verify();
   defer {
      pager.free(page);
   };
   cat::is_allocator auto allocator = cat::make_linear_allocator(page);

   int4* p = allocator.alloc<int4>().or_exit();
   *p = int4{42};

   p = allocator.realloc(p).or_exit();
   cat::verify(*p == int4{42});

   cat::span<int4> row = allocator.realloc_multi(p, idx{1u}, idx{4u}).or_exit();
   row[0] = int4{1};
   row[3] = int4{9};
   cat::verify(row[0] == int4{1});
   cat::verify(row[3] == int4{9});

   allocator.free_multi(row.data(), idx{4u});
}
