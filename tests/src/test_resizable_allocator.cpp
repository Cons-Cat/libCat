// Tests for `resizable_allocator<Inner, Backing>`: lazy free-list reuse on
// deallocate and chunked growth via the `Backing` allocator on resize. The
// wrapper is exercised over both `pool_allocator` and `linear_allocator` as
// inner types, with `page_allocator` as the backing source.

#include <cat/allocator_parameters>
#include <cat/linear_allocator>
#include <cat/null_allocator>
#include <cat/page_allocator>
#include <cat/pool_allocator>
#include <cat/resizable_allocator>

#include "../unit_tests.hpp"

using wrapper_pool_16 = cat::resizable_allocator<
   cat::pool_allocator<16>, cat::page_allocator, cat::idx(16u), cat::page_size>;
using wrapper_linear_32 = cat::resizable_allocator<
   cat::linear_allocator, cat::page_allocator, cat::idx(32u), cat::page_size>;

// Static re-exports: the wrapper mirrors `Inner`'s alignment so the alignment
// fast path in `meta_alloc` sees the wrapped allocator's guarantee.
static_assert(
   wrapper_pool_16::min_alignment == cat::pool_allocator<16>::min_alignment
);
static_assert(wrapper_pool_16::node_bytes == 16u);
static_assert(wrapper_pool_16::chunk_bytes == cat::page_size);

static_assert(
   wrapper_linear_32::min_alignment == cat::linear_allocator::min_alignment
);
static_assert(wrapper_linear_32::node_bytes == 32u);

// A bare wrapper holds nothing yet: the chunk list and free list are both
// empty, so the introspection hooks read zero. The first `allocate` then
// auto-grows by pulling a chunk from the backing, so it succeeds and reports
// non-zero capacity afterward.
$test(resizable_allocator_starts_empty) {
   wrapper_pool_16 wrapper{pager};

   cat::verify(wrapper.bytes_capacity() == 0u);
   cat::verify(wrapper.bytes_used() == 0u);

   void* p = cat::dyn_allocator_friend::allocate(wrapper, cat::idx(16u));
   cat::verify(p != nullptr);
   cat::verify(wrapper.bytes_capacity() > 0u);
   cat::verify(wrapper.bytes_used() == 16u);
}

// When the backing allocator cannot supply a chunk, `allocate` has nowhere to
// grow into and must report failure rather than dereferencing a null chunk.
$test(resizable_allocator_allocate_fails_when_backing_exhausted) {
   cat::null_allocator null_backing;
   cat::resizable_allocator<
      cat::pool_allocator<16>, cat::null_allocator, cat::idx(16u),
      cat::page_size>
      wrapper{null_backing};

   void* p = cat::dyn_allocator_friend::allocate(wrapper, cat::idx(16u));
   cat::verify(p == nullptr);
   cat::verify(wrapper.bytes_capacity() == 0u);
}

// `resize` is the wrapper's growth hook. After a single resize the
// capacity must report at least one chunk's worth of inner storage, and
// `allocate` must hand a non-null slot back.
$test(resizable_allocator_resize_then_allocate) {
   wrapper_pool_16 wrapper{pager};

   cat::maybe const grown = wrapper.resize(cat::idx(16u));
   cat::verify(grown.has_value());
   cat::verify(grown.value() >= cat::page_size);

   cat::verify(wrapper.bytes_capacity() > 0u);

   void* p = cat::dyn_allocator_friend::allocate(wrapper, cat::idx(16u));
   cat::verify(p != nullptr);
   cat::verify(wrapper.bytes_used() == 16u);
}

// Free-list reuse is the wrapper's defining behavior: a freed slot is pushed
// onto an intrusive LIFO list and the next allocate pops it back out before
// asking the inner allocator. The reused address must match.
$test(resizable_allocator_freelist_reuse_returns_freed_slot) {
   wrapper_pool_16 wrapper{pager};
   cat::verify(wrapper.resize(cat::idx(16u)).has_value());

   void* p_a = cat::dyn_allocator_friend::allocate(wrapper, cat::idx(16u));
   void* p_b = cat::dyn_allocator_friend::allocate(wrapper, cat::idx(16u));
   cat::verify(p_a != nullptr);
   cat::verify(p_b != nullptr);
   cat::verify(p_a != p_b);
   cat::verify(wrapper.bytes_used() == 32u);

   cat::dyn_allocator_friend::deallocate(wrapper, p_a, cat::idx(16u));
   cat::verify(wrapper.bytes_used() == 16u);

   // LIFO: the most recently freed slot is the first one handed back.
   void* p_c = cat::dyn_allocator_friend::allocate(wrapper, cat::idx(16u));
   cat::verify(p_c == p_a);
   cat::verify(wrapper.bytes_used() == 32u);
}

// `bytes_used` must subtract the free-list size from the inner allocator's
// own bookkeeping, so freeing slots monotonically lowers it.
$test(resizable_allocator_bytes_used_tracks_freelist) {
   wrapper_pool_16 wrapper{pager};
   cat::verify(wrapper.resize(cat::idx(48u)).has_value());

   void* p_1 = cat::dyn_allocator_friend::allocate(wrapper, cat::idx(16u));
   void* p_2 = cat::dyn_allocator_friend::allocate(wrapper, cat::idx(16u));
   void* p_3 = cat::dyn_allocator_friend::allocate(wrapper, cat::idx(16u));
   cat::verify(wrapper.bytes_used() == 48u);

   cat::dyn_allocator_friend::deallocate(wrapper, p_1, cat::idx(16u));
   cat::verify(wrapper.bytes_used() == 32u);
   cat::dyn_allocator_friend::deallocate(wrapper, p_2, cat::idx(16u));
   cat::verify(wrapper.bytes_used() == 16u);
   cat::dyn_allocator_friend::deallocate(wrapper, p_3, cat::idx(16u));
   cat::verify(wrapper.bytes_used() == 0u);
}

// Calling `resize` twice should produce two distinct chunks. Capacity
// must therefore at least double, and each subsequent allocation can come
// from either chunk depending on the wrapper's policy.
$test(resizable_allocator_chunked_growth_adds_capacity) {
   wrapper_pool_16 wrapper{pager};
   cat::verify(wrapper.resize(cat::idx(16u)).has_value());
   cat::idx const capacity_after_one = wrapper.bytes_capacity();
   cat::verify(capacity_after_one > 0u);

   cat::verify(wrapper.resize(cat::idx(16u)).has_value());
   cat::idx const capacity_after_two = wrapper.bytes_capacity();
   cat::verify(capacity_after_two >= capacity_after_one * 2u);
}

// `reset` drops the free list and returns every chunk to the backing
// allocator. After reset, capacity and used both report zero and the
// wrapper is ready to grow again.
$test(resizable_allocator_reset_releases_chunks) {
   wrapper_pool_16 wrapper{pager};
   cat::verify(wrapper.resize(cat::idx(16u)).has_value());
   cat::verify(wrapper.resize(cat::idx(16u)).has_value());
   void* p = cat::dyn_allocator_friend::allocate(wrapper, cat::idx(16u));
   cat::verify(p != nullptr);
   cat::verify(wrapper.bytes_used() == 16u);

   wrapper.reset();
   cat::verify(wrapper.bytes_capacity() == 0u);
   cat::verify(wrapper.bytes_used() == 0u);

   cat::verify(wrapper.resize(cat::idx(16u)).has_value());
   void* p_after_reset =
      cat::dyn_allocator_friend::allocate(wrapper, cat::idx(16u));
   cat::verify(p_after_reset != nullptr);
}

// The wrapper composes the same way over `linear_allocator`. The inner
// allocator's `allocate` returns a bumped pointer, but the free-list reuse
// is the wrapper's own bookkeeping and still recycles the freed slot.
$test(resizable_allocator_over_linear_allocator) {
   wrapper_linear_32 wrapper{pager};
   cat::verify(wrapper.resize(cat::idx(32u)).has_value());

   void* p_1 = cat::dyn_allocator_friend::allocate(wrapper, cat::idx(32u));
   void* p_2 = cat::dyn_allocator_friend::allocate(wrapper, cat::idx(32u));
   cat::verify(p_1 != nullptr);
   cat::verify(p_2 != nullptr);
   cat::verify(p_1 != p_2);
   cat::verify(wrapper.bytes_used() == 64u);

   cat::dyn_allocator_friend::deallocate(wrapper, p_2, cat::idx(32u));
   void* p_3 = cat::dyn_allocator_friend::allocate(wrapper, cat::idx(32u));
   cat::verify(p_3 == p_2);
}

// `is_equivalent` is defined as "share backing pointer and chunk-list head".
// A fresh wrapper over the same backing source has a distinct chunk list, so
// equivalence must be false.
$test(resizable_allocator_equality) {
   wrapper_pool_16 wrapper_a{pager};
   cat::verify(wrapper_a.resize(cat::idx(16u)).has_value());

   wrapper_pool_16 wrapper_b{pager};
   cat::verify(wrapper_b.resize(cat::idx(16u)).has_value());

   cat::verify(wrapper_a.is_equivalent(wrapper_a));
   cat::verify(!wrapper_a.is_equivalent(wrapper_b));
}
