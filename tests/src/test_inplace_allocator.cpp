#include <cat/inplace_allocator>
#include <cat/math>

#include "../unit_tests.hpp"

$test(inplace_allocator) {
   cat::is_allocator auto allocator = cat::make_inplace_allocator<24u>();

   cat::verify(allocator.bytes_capacity() == 24u);
   cat::verify(allocator.bytes_used() == 0u);

   // A single allocation succeeds and takes the whole buffer.
   int4* p_handle = allocator.alloc<int4>(5).or_exit();
   cat::verify(*p_handle == 5);
   cat::verify(cat::is_aligned(p_handle, alignof(int4)));
   cat::verify(allocator.bytes_used() == 24u);

   // A second allocation fails because the buffer is one-shot.
   cat::verify(!allocator.alloc<int4>().has_value());

   // Freeing the outstanding allocation makes the buffer available again.
   allocator.free(p_handle);
   cat::verify(allocator.bytes_used() == 0u);

   int4* p_handle_2 = allocator.alloc<int4>(7).or_exit();
   cat::verify(*p_handle_2 == 7);
   cat::verify(!allocator.alloc<int4>().has_value());

   // `reset()` also releases the buffer.
   allocator.reset();
   cat::verify(allocator.bytes_used() == 0u);
   cat::maybe handle_3 = allocator.alloc<int4>();
   cat::verify(handle_3.has_value());

   // An allocation larger than the buffer fails.
   allocator.reset();
   cat::verify(!allocator.alloc_multi<int4>(100u).has_value());

   // A request that fits in the buffer reports the whole buffer's size.
   allocator.reset();
   cat::verify(allocator.nalloc<int4>().or_exit() == 24u);

   // Over-alignment is honored when the request still fits past the padding.
   cat::is_allocator auto over_aligned = cat::make_inplace_allocator<128u>();
   int4* p_over = over_aligned.align_alloc<int4>(64u, 9).or_exit();
   cat::verify(*p_over == 9);
   cat::verify(cat::is_aligned(p_over, 64u));

   // An over-aligned request that cannot fit past its padding fails.
   over_aligned.reset();
   cat::verify(!over_aligned.align_alloc_multi<int4>(64u, 17u).has_value());
}
