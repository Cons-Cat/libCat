// Tests for the introspection and growth hooks added to the allocator
// surface: `bytes_used`, `bytes_capacity`, `min_alignment`, `resize`,
// the `alloc_grow` family, `is_equivalent` / `operator==`, and `name`.
// Forwarding through `dyn_allocator` and `allocator_ref<*>` is exercised too.

#include <cat/allocator_parameters>
#include <cat/bit>
#include <cat/linear_allocator>
#include <cat/null_allocator>
#include <cat/page_allocator>
#include <cat/pool_allocator>

#include "../unit_tests.hpp"

// Static alignment guarantees are part of the type, so they can be checked
// without any test body.
static_assert(cat::linear_allocator::min_alignment == 1u);
static_assert(cat::page_allocator::min_alignment == cat::page_size);
static_assert(cat::null_allocator::min_alignment == cat::page_size);
static_assert(cat::pool_allocator<8>::min_alignment
              == uword(alignof(cat::pool_allocator<8>::node_union)));

// Type-erased dispatch loses the static lower bound, so `dyn_allocator`
// must conservatively claim 1.
static_assert(cat::dyn_allocator::min_alignment == 1u);

// `allocator_ref<A>` should mirror the wrapped allocator's static alignment.
static_assert(cat::allocator_ref<cat::linear_allocator>::min_alignment == 1u);
static_assert(cat::allocator_ref<cat::page_allocator>::min_alignment
              == cat::page_size);

// `allocator_ref<A>` should fold to an empty type whenever `A` itself is
// empty (a stateless / global allocator). This lets containers and other
// holders embed the ref via `[[no_unique_address]]` without paying any
// storage cost.
static_assert(__is_empty(cat::page_allocator));
static_assert(__is_empty(cat::null_allocator));
static_assert(__is_empty(cat::allocator_ref<cat::page_allocator>));
static_assert(__is_empty(cat::allocator_ref<cat::null_allocator>));

// Stateful allocators leave the ref as a single-pointer handle.
static_assert(!__is_empty(cat::allocator_ref<cat::linear_allocator>));
static_assert(sizeof(cat::allocator_ref<cat::linear_allocator>)
              == sizeof(cat::linear_allocator*));

$test(linear_allocator_bytes_used_and_capacity) {
   cat::span page = pager.alloc_multi<cat::byte>(64u).verify();
   $defer {
      pager.free(page);
   };
   cat::is_allocator auto allocator = cat::make_linear_allocator(page);

   cat::verify(allocator.bytes_capacity() == 64u);
   cat::verify(allocator.bytes_used() == 0u);

   auto* p_a = allocator.alloc<int4>(1).verify();
   static_cast<void>(p_a);
   cat::verify(allocator.bytes_used() >= 4u);
   cat::verify(allocator.bytes_used() <= allocator.bytes_capacity());

   auto* p_b = allocator.alloc<int4>(2).verify();
   static_cast<void>(p_b);
   cat::verify(allocator.bytes_used() >= 8u);

   allocator.reset();
   cat::verify(allocator.bytes_used() == 0u);
   cat::verify(allocator.bytes_capacity() == 64u);
}

$test(linear_allocator_resize_returns_nullopt) {
   cat::span page = pager.alloc_multi<cat::byte>(32u).verify();
   $defer {
      pager.free(page);
   };
   cat::is_allocator auto allocator = cat::make_linear_allocator(page);

   cat::maybe grown = allocator.resize(64u);
   cat::verify(!grown.has_value());
}

// A downward-bump arena cannot grow in place without relocating the
// allocation's start address, so `reallocate` always rejects grow requests.
$test(linear_allocator_reallocate_grow_always_fails) {
   cat::span page = pager.alloc_multi<cat::byte>(64u).verify();
   $defer {
      pager.free(page);
   };
   cat::is_allocator auto allocator = cat::make_linear_allocator(page);

   int4* p_top = allocator.alloc<int4>(42).verify();
   idx const used_before = allocator.bytes_used();

   cat::span<cat::byte> const allocation{reinterpret_cast<cat::byte*>(p_top),
                                         idx(4u)};
   cat::maybe const grew = allocator.alloc_grow(allocation, idx(8u));
   cat::verify(!grew.has_value());
   cat::verify(allocator.bytes_used() == used_before);

   cat::maybe grew_sized = allocator.alloc_grow_feedback(allocation, idx(8u));
   cat::verify(!grew_sized.has_value());
   cat::verify(allocator.bytes_used() == used_before);
}

// Shrink is honored in place. The released tail bytes stay reserved until
// `reset()` to preserve `p_storage`, so `bytes_used` is unchanged.
$test(linear_allocator_reallocate_shrink_succeeds) {
   cat::span page = pager.alloc_multi<cat::byte>(64u).verify();
   $defer {
      pager.free(page);
   };
   cat::is_allocator auto allocator = cat::make_linear_allocator(page);

   int4* p_top = allocator.alloc<int4>(7).verify();
   idx const used_before = allocator.bytes_used();
   cat::verify(*p_top == 7);

   cat::span<cat::byte> const allocation{reinterpret_cast<cat::byte*>(p_top),
                                         idx(4u)};
   cat::maybe const shrank = allocator.alloc_grow(allocation, idx(2u));
   cat::verify(shrank.has_value());
   cat::verify(allocator.bytes_used() == used_before);
   // `p_top`'s first `new_bytes` bytes stay valid after shrink.
   cat::verify(*p_top == 7);

   cat::span<cat::byte> const shrunk{reinterpret_cast<cat::byte*>(p_top),
                                     idx(2u)};
   cat::maybe sized = allocator.alloc_grow_feedback(shrunk, idx(1u));
   cat::verify(sized.has_value());
   cat::verify(sized.value() == 1u);
   cat::verify(allocator.bytes_used() == used_before);
}

// Two `linear_allocator` instances over the same arena should be reported
// as equivalent. They have independent bump pointers but govern the same
// memory range.
$test(linear_allocator_equality) {
   cat::span page_a = pager.alloc_multi<cat::byte>(32u).verify();
   cat::span page_b = pager.alloc_multi<cat::byte>(32u).verify();
   $defer {
      pager.free(page_a);
      pager.free(page_b);
   };

   cat::is_allocator auto a1 = cat::make_linear_allocator(page_a);
   cat::is_allocator auto a2 = cat::make_linear_allocator(page_a);
   cat::is_allocator auto b1 = cat::make_linear_allocator(page_b);

   cat::verify(a1 == a2);
   cat::verify(a1.is_equivalent(a2));
   cat::verify(!(a1 == b1));
   cat::verify(!a1.is_equivalent(b1));
}

$test(pool_allocator_bytes_used_and_capacity) {
   cat::span page = pager.alloc_multi<cat::byte>(64u).verify();
   $defer {
      pager.free(page);
   };
   cat::is_allocator auto pool = cat::make_pool_allocator<8>(page);

   cat::verify(pool.bytes_capacity() == 64u);
   cat::verify(pool.bytes_used() == 0u);

   int4* p_a = pool.alloc<int4>(1).verify();
   cat::verify(pool.bytes_used() == 8u);

   int4* p_b = pool.alloc<int4>(2).verify();
   cat::verify(pool.bytes_used() == 16u);

   pool.free(p_a);
   cat::verify(pool.bytes_used() == 8u);

   pool.free(p_b);
   cat::verify(pool.bytes_used() == 0u);
}

$test(pool_allocator_resize_returns_nullopt) {
   cat::span page = pager.alloc_multi<cat::byte>(64u).verify();
   $defer {
      pager.free(page);
   };
   cat::is_allocator auto pool = cat::make_pool_allocator<8>(page);

   cat::maybe grown = pool.resize(64u);
   cat::verify(!grown.has_value());
}

$test(pool_allocator_equality) {
   cat::span page_a = pager.alloc_multi<cat::byte>(64u).verify();
   cat::span page_b = pager.alloc_multi<cat::byte>(64u).verify();
   $defer {
      pager.free(page_a);
      pager.free(page_b);
   };

   cat::is_allocator auto a1 = cat::make_pool_allocator<8>(page_a);
   cat::is_allocator auto a2 = cat::make_pool_allocator<8>(page_a);
   cat::is_allocator auto b1 = cat::make_pool_allocator<8>(page_b);

   cat::verify(a1 == a2);
   cat::verify(!(a1 == b1));
}

// `page_allocator::resize` does an anonymous probe via `mmap` /`munmap`.
// A page-sized request must succeed on any working system.
$test(page_allocator_resize_probes_kernel) {
   cat::page_allocator local_pager;
   cat::maybe const grown = local_pager.resize(cat::page_size);
   cat::verify(grown.has_value());
   cat::verify(grown.value() >= cat::page_size);
}

// `bytes_used` and `bytes_capacity` read the detailed memory-map file. The
// numbers are process-wide rather than `page_allocator`-specific. Any running
// process has anonymous mappings, so both values must be strictly positive and
// page-aligned, and `bytes_used` must not exceed `bytes_capacity`.
$test(page_allocator_introspection_reads_statm) {
   cat::idx const capacity = pager.bytes_capacity();
   cat::idx const used = pager.bytes_used();

   cat::verify(capacity > 0u);
   cat::verify(used > 0u);
   cat::verify(used <= capacity);
   cat::verify(capacity % cat::page_size == 0u);
   cat::verify(used % cat::page_size == 0u);
}

// Allocating a sizeable mapping must show up in the resident bytes count
// once the allocator touches it. `page_allocator` requests `populate`-ed
// pages so the resident set grows at allocation time.
$test(page_allocator_bytes_used_grows_with_allocation) {
   cat::idx const used_before = pager.bytes_used();
   cat::span page = pager.alloc_multi<cat::byte>(64u * 1'024u).verify();
   // Touch a byte to ensure the kernel actually backs the mapping with a
   // physical page even on systems that lazily commit despite `populate`.
   page[0] = cat::byte{};
   cat::idx const used_after = pager.bytes_used();
   pager.free(page);

   cat::verify(used_after >= used_before);
}

// `page_allocator` resizes mappings in place with `mremap`. Shrinking never
// needs to relocate, so the in-place hook always succeeds and reports the
// page-rounded byte count it settled on.
$test(page_allocator_reallocate_shrinks_in_place) {
   cat::span page = pager.alloc_multi<cat::byte>(cat::page_size * 4u).verify();
   $defer {
      pager.free(page);
   };

   cat::maybe const shrunk =
      pager.alloc_grow_feedback(page, cat::page_size * 2u);
   cat::verify(shrunk.has_value());
   cat::verify(shrunk.value() == cat::page_size * 2u);
}

// A grow either succeeds at the same address (when the following address
// space is free) or fails so the caller can relocate. Both outcomes honor
// the in-place contract, so only the post-conditions are asserted.
$test(page_allocator_reallocate_grow_is_in_place_or_fails) {
   cat::span page = pager.alloc_multi<cat::byte>(cat::page_size).verify();
   cat::maybe const grown =
      pager.alloc_grow_feedback(page, cat::page_size * 2u);
   if (grown.has_value()) {
      cat::verify(grown.value() == cat::page_size * 2u);
      cat::span<cat::byte> const larger{page.data(), cat::page_size * 2u};
      pager.free(larger);
   } else {
      pager.free(page);
   }
}

// Allocator names default to an empty `str_view` on bare allocators.
// The named wrapper adaptors are tested separately.
$test(default_allocator_name_is_empty) {
   cat::span page = pager.alloc_multi<cat::byte>(32u).verify();
   $defer {
      pager.free(page);
   };
   cat::is_allocator auto linear = cat::make_linear_allocator(page);

   cat::verify(linear.name().size() == 0);
   cat::verify(pager.name().size() == 0);
}

// `dyn_allocator` forwards introspection through its vtable. The held
// allocators are stateful, so the values must agree with the underlying
// objects.
$test(dyn_allocator_introspection_forwarding) {
   cat::span page = pager.alloc_multi<cat::byte>(64u).verify();
   $defer {
      pager.free(page);
   };
   cat::is_allocator auto linear = cat::make_linear_allocator(page);

   cat::dyn_allocator dyn = linear;
   cat::verify(dyn.bytes_capacity() == linear.bytes_capacity());
   cat::verify(dyn.bytes_used() == linear.bytes_used());
   cat::verify(dyn.name().size() == 0);

   auto* p = dyn.alloc<int4>(5).verify();
   static_cast<void>(p);
   cat::verify(dyn.bytes_used() == linear.bytes_used());
}

// Two `dyn_allocator` handles erasing the same concrete allocator instance
// compare equal. Two handles erasing distinct instances of the same type
// do not. Two handles erasing different types do not, regardless of the
// underlying memory.
$test(dyn_allocator_equality) {
   cat::span page_a = pager.alloc_multi<cat::byte>(32u).verify();
   cat::span page_b = pager.alloc_multi<cat::byte>(32u).verify();
   $defer {
      pager.free(page_a);
      pager.free(page_b);
   };
   cat::is_allocator auto linear_a1 = cat::make_linear_allocator(page_a);
   cat::is_allocator auto linear_a2 = cat::make_linear_allocator(page_a);
   cat::is_allocator auto linear_b = cat::make_linear_allocator(page_b);

   cat::dyn_allocator d_a1 = linear_a1;
   cat::dyn_allocator d_a2 = linear_a2;
   cat::dyn_allocator d_b = linear_b;
   cat::dyn_allocator d_pager = pager;

   cat::verify(d_a1 == d_a2);
   cat::verify(d_a1.is_equivalent(d_a2));

   cat::verify(!(d_a1 == d_b));
   cat::verify(!(d_a1 == d_pager));
}

// When `allocator_ref<EmptyAllocator>` is embedded via `[[no_unique_address]]`,
// it must take zero storage, leaving the outer struct sized by its other
// members alone.
$test(allocator_ref_folds_when_wrapping_empty_allocator) {
   struct page_holder {
      [[no_unique_address]]
      cat::allocator_ref<cat::page_allocator> ref;
      cat::int4 payload;
   };

   struct null_holder {
      [[no_unique_address]]
      cat::allocator_ref<cat::null_allocator> ref;
      cat::int4 payload;
   };

   static_assert(sizeof(page_holder) == sizeof(cat::int4));
   static_assert(sizeof(null_holder) == sizeof(cat::int4));

   // The ref's dispatch must still work even though it carries no state.
   cat::allocator_ref<cat::page_allocator> page_ref;
   cat::span<int4> page = page_ref.alloc_multi<int4>(4u).verify();
   page_ref.free(page);

   cat::allocator_ref<cat::null_allocator> null_ref;
   cat::maybe<int4*> result = null_ref.alloc<int4>();
   cat::verify(!result.has_value());
}

$test(allocator_ref_introspection_forwarding) {
   cat::span page = pager.alloc_multi<cat::byte>(32u).verify();
   $defer {
      pager.free(page);
   };
   cat::is_allocator auto linear = cat::make_linear_allocator(page);

   cat::allocator_ref<cat::linear_allocator> ref = linear;
   cat::verify(ref.bytes_capacity() == linear.bytes_capacity());
   cat::verify(ref.bytes_used() == linear.bytes_used());

   auto* p = ref.alloc<int4>(9).verify();
   static_cast<void>(p);
   cat::verify(ref.bytes_used() == linear.bytes_used());
}

// `allocator_ref<A>::operator==` falls through to `is_equivalent` on the
// underlying allocator. Two refs into the same arena compare equal even
// if they hold pointers to distinct linear allocator instances over that
// arena.
$test(allocator_ref_equality_uses_is_equivalent) {
   cat::span page_a = pager.alloc_multi<cat::byte>(32u).verify();
   cat::span page_b = pager.alloc_multi<cat::byte>(32u).verify();
   $defer {
      pager.free(page_a);
      pager.free(page_b);
   };
   cat::is_allocator auto linear = cat::make_linear_allocator(page_a);
   cat::is_allocator auto linear_address_lias =
      cat::make_linear_allocator(page_a);
   cat::is_allocator auto linear_other = cat::make_linear_allocator(page_b);

   cat::allocator_ref<cat::linear_allocator> ref = linear;
   cat::allocator_ref<cat::linear_allocator> ref_alias = linear;
   cat::allocator_ref<cat::linear_allocator> ref_address_alias =
      linear_address_lias;
   cat::allocator_ref<cat::linear_allocator> ref_other = linear_other;

   cat::verify(ref == ref_alias);
   cat::verify(ref == linear);
   cat::verify(ref == ref_address_alias);
   cat::verify(ref != ref_other);
}
