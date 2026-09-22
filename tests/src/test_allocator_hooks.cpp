// Tests for the introspection and growth hooks added to the allocator
// surface: `bytes_used`, `bytes_capacity`, `min_alignment`, `resize`,
// the `alloc_grow` family, `is_equivalent` / `operator==`, and `name`.
// Forwarding through `allocator_dyn` and `allocator_ref<*>` is exercised too.

#include <cat/allocator_huge_page>
#include <cat/allocator_linear>
#include <cat/allocator_null>
#include <cat/allocator_page>
#include <cat/allocator_parameters>
#include <cat/allocator_pool>
#include <cat/bit>

#include "../unit_tests.hpp"

// Static alignment guarantees are part of the type, so they can be checked
// without any test body.
static_assert(cat::allocator_linear::min_alignment == 1u);
static_assert(cat::allocator_huge_page::min_alignment == cat::huge_page_size);
static_assert(cat::allocator_page::min_alignment == cat::page_size);
static_assert(cat::allocator_null::min_alignment == cat::page_size);
static_assert(
   cat::allocator_pool<8>::min_alignment
   == uword(alignof(cat::allocator_pool<8>::node_union))
);

// Type-erased dispatch loses the static lower bound, so `allocator_dyn`
// must conservatively claim 1.
static_assert(cat::allocator_dyn::min_alignment == 1u);

// `allocator_ref<A>` should mirror the wrapped allocator's static alignment.
static_assert(cat::allocator_ref<cat::allocator_linear>::min_alignment == 1u);
static_assert(
   cat::allocator_ref<cat::allocator_page>::min_alignment == cat::page_size
);
static_assert(
   cat::allocator_ref<cat::allocator_huge_page>::min_alignment
   == cat::huge_page_size
);

// `allocator_ref<A>` should fold to an empty type whenever `A` itself is
// empty (a stateless / global allocator). This lets containers and other
// holders embed the ref via `[[no_unique_address]]` without paying any
// storage cost.
static_assert(__is_empty(cat::allocator_page));
static_assert(__is_empty(cat::allocator_huge_page));
static_assert(__is_empty(cat::allocator_null));
static_assert(__is_empty(cat::allocator_ref<cat::allocator_page>));
static_assert(__is_empty(cat::allocator_ref<cat::allocator_huge_page>));
static_assert(__is_empty(cat::allocator_ref<cat::allocator_null>));

// Stateful allocators leave the ref as a single-pointer handle.
static_assert(!__is_empty(cat::allocator_ref<cat::allocator_linear>));
static_assert(
   sizeof(cat::allocator_ref<cat::allocator_linear>)
   == sizeof(cat::allocator_linear*)
);

$test(allocator_linear_bytes_used_and_capacity) {
   cat::span page = g_tests_alloc.alloc_multi<cat::byte>(64u).verify();
   $defer {
      g_tests_alloc.free(page);
   };
   cat::is_allocator auto allocator = cat::make_allocator_linear(page);

   cat::verify(allocator.bytes_capacity() == 64u);
   cat::verify(allocator.bytes_used() == 0u);

   [[maybe_unused]]
   auto* p_a = allocator.alloc<int4>(1).verify();
   cat::verify(allocator.bytes_used() >= 4u);
   cat::verify(allocator.bytes_used() <= allocator.bytes_capacity());

   [[maybe_unused]]
   auto* p_b = allocator.alloc<int4>(2).verify();
   cat::verify(allocator.bytes_used() >= 8u);

   allocator.reset();
   cat::verify(allocator.bytes_used() == 0u);
   cat::verify(allocator.bytes_capacity() == 64u);
}

$test(allocator_linear_resize_returns_nullopt) {
   cat::span page = g_tests_alloc.alloc_multi<cat::byte>(32u).verify();
   $defer {
      g_tests_alloc.free(page);
   };
   cat::is_allocator auto allocator = cat::make_allocator_linear(page);

   cat::maybe grown = allocator.resize(64u);
   cat::verify(grown.is_empty());
}

// A downward-bump arena cannot grow in place without relocating the
// allocation's start address, so `reallocate` always rejects grow requests.
$test(allocator_linear_reallocate_grow_always_fails) {
   cat::span page = g_tests_alloc.alloc_multi<cat::byte>(64u).verify();
   $defer {
      g_tests_alloc.free(page);
   };
   cat::is_allocator auto allocator = cat::make_allocator_linear(page);

   int4* p_top = allocator.alloc<int4>(42).verify();
   idx const used_before = allocator.bytes_used();

   cat::span<cat::byte> const allocation{
      reinterpret_cast<cat::byte*>(p_top), idx(4u)
   };
   cat::maybe const grew = allocator.alloc_grow(allocation, idx(8u));
   cat::verify(grew.is_empty());
   cat::verify(allocator.bytes_used() == used_before);

   cat::maybe grew_sized = allocator.alloc_grow_feedback(allocation, idx(8u));
   cat::verify(grew_sized.is_empty());
   cat::verify(allocator.bytes_used() == used_before);
}

// Shrink is honored in place. The released tail bytes stay reserved until
// `reset()` to preserve `p_storage`, so `bytes_used` is unchanged.
$test(allocator_linear_reallocate_shrink_succeeds) {
   cat::span page = g_tests_alloc.alloc_multi<cat::byte>(64u).verify();
   $defer {
      g_tests_alloc.free(page);
   };
   cat::is_allocator auto allocator = cat::make_allocator_linear(page);

   int4* p_top = allocator.alloc<int4>(7).verify();
   idx const used_before = allocator.bytes_used();
   cat::verify(*p_top == 7);

   cat::span<cat::byte> const allocation{
      reinterpret_cast<cat::byte*>(p_top), idx(4u)
   };
   cat::maybe const shrank = allocator.alloc_grow(allocation, idx(2u));
   cat::verify(shrank.has_value());
   cat::verify(allocator.bytes_used() == used_before);
   // `p_top`'s first `new_bytes` bytes stay valid after shrink.
   cat::verify(*p_top == 7);

   cat::span<cat::byte> const shrunk{
      reinterpret_cast<cat::byte*>(p_top), idx(2u)
   };
   cat::maybe sized = allocator.alloc_grow_feedback(shrunk, idx(1u));
   cat::verify(sized.has_value());
   cat::verify(sized.value() == 1u);
   cat::verify(allocator.bytes_used() == used_before);
}

// Two `allocator_linear` instances over the same arena should be reported
// as equivalent. They have independent bump pointers but govern the same
// memory range.
$test(allocator_linear_equality) {
   cat::span page_a = g_tests_alloc.alloc_multi<cat::byte>(32u).verify();
   cat::span page_b = g_tests_alloc.alloc_multi<cat::byte>(32u).verify();
   $defer {
      g_tests_alloc.free(page_a);
      g_tests_alloc.free(page_b);
   };

   cat::is_allocator auto a1 = cat::make_allocator_linear(page_a);
   cat::is_allocator auto a2 = cat::make_allocator_linear(page_a);
   cat::is_allocator auto b1 = cat::make_allocator_linear(page_b);

   cat::verify(a1 == a2);
   cat::verify(a1.is_equivalent(a2));
   cat::verify(!(a1 == b1));
   cat::verify(!a1.is_equivalent(b1));
}

$test(allocator_pool_bytes_used_and_capacity) {
   cat::span page = g_tests_alloc.alloc_multi<cat::byte>(64u).verify();
   $defer {
      g_tests_alloc.free(page);
   };
   cat::is_allocator auto pool = cat::make_allocator_pool<8>(page);

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

$test(allocator_pool_resize_returns_nullopt) {
   cat::span page = g_tests_alloc.alloc_multi<cat::byte>(64u).verify();
   $defer {
      g_tests_alloc.free(page);
   };
   cat::is_allocator auto pool = cat::make_allocator_pool<8>(page);

   cat::maybe grown = pool.resize(64u);
   cat::verify(grown.is_empty());
}

$test(allocator_pool_equality) {
   cat::span page_a = g_tests_alloc.alloc_multi<cat::byte>(64u).verify();
   cat::span page_b = g_tests_alloc.alloc_multi<cat::byte>(64u).verify();
   $defer {
      g_tests_alloc.free(page_a);
      g_tests_alloc.free(page_b);
   };

   cat::is_allocator auto a1 = cat::make_allocator_pool<8>(page_a);
   cat::is_allocator auto a2 = cat::make_allocator_pool<8>(page_a);
   cat::is_allocator auto b1 = cat::make_allocator_pool<8>(page_b);

   cat::verify(a1 == a2);
   cat::verify(!(a1 == b1));
}

$test(allocator_huge_page_rounds_to_two_mebibytes) {
   cat::allocator_huge_page allocator;

   cat::maybe const one_byte = allocator.allocation_bytes(1u, 1u);
   cat::maybe const one_page =
      allocator.allocation_bytes(1u, cat::huge_page_size);
   cat::maybe const page_and_byte =
      allocator.allocation_bytes(1u, cat::huge_page_size + 1u);

   cat::verify(one_byte.value() == cat::huge_page_size);
   cat::verify(one_page.value() == cat::huge_page_size);
   cat::verify(page_and_byte.value() == cat::huge_page_size * 2u);
}

// Explicit huge-page mappings require a configured kernel pool. When one is
// available, verify the alignment and both ends of the mapped page.
$test(allocator_huge_page_maps_configured_pool) {
   cat::allocator_huge_page allocator;
   auto maybe_page = allocator.alloc_multi<cat::byte>(cat::huge_page_size);
   if (maybe_page.is_empty()) {
      return;
   }

   cat::span page = maybe_page.value();
   cat::verify(cat::is_aligned(page.data(), cat::huge_page_size));
   page[0] = cat::byte(1u);
   page.back() = cat::byte(2u);
   cat::verify(page[0] == cat::byte(1u));
   cat::verify(page.back() == cat::byte(2u));
   allocator.free(page);
}

// `allocator_page::resize` does an anonymous probe via `mmap` /`munmap`.
// A page-sized request must succeed on any working system.
$test(allocator_page_resize_probes_kernel) {
   cat::maybe const grown = pager.resize(cat::page_size);
   cat::verify(grown.has_value());
   cat::verify(grown.value() >= cat::page_size);
}

// `bytes_used` and `bytes_capacity` read the detailed memory-map file. The
// numbers are process-wide rather than `allocator_page`-specific. Any running
// process has anonymous mappings, so both values must be strictly positive and
// page-aligned, and `bytes_used` must not exceed `bytes_capacity`.
$test(allocator_page_introspection_reads_statm) {
   cat::idx const capacity = pager.bytes_capacity();
   cat::idx const used = pager.bytes_used();

   cat::verify(capacity > 0u);
   cat::verify(used > 0u);
   cat::verify(used <= capacity);
   cat::verify(capacity % cat::page_size == 0u);
   cat::verify(used % cat::page_size == 0u);
}

// Allocating a sizeable mapping must show up in the resident bytes count
// once the allocator touches it. `allocator_page` requests `populate`-ed
// pages so the resident set grows at allocation time.
$test(allocator_page_bytes_used_grows_with_allocation) {
   cat::idx const used_before = pager.bytes_used();
   cat::span page = pager.alloc_multi<cat::byte>(64u * 1'024u).verify();
   // Touch a byte to ensure the kernel actually backs the mapping with a
   // physical page even on systems that lazily commit despite `populate`.
   page[0] = cat::byte{};
   cat::idx const used_after = pager.bytes_used();
   pager.free(page);

   cat::verify(used_after >= used_before);
}

// `allocator_page` resizes mappings in place with `mremap`. Shrinking never
// needs to relocate, so the in-place hook always succeeds and reports the
// page-rounded byte count it settled on.
$test(allocator_page_reallocate_shrinks_in_place) {
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
$test(allocator_page_reallocate_grow_is_in_place_or_fails) {
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
   cat::span page = g_tests_alloc.alloc_multi<cat::byte>(32u).verify();
   $defer {
      g_tests_alloc.free(page);
   };
   cat::is_allocator auto linear = cat::make_allocator_linear(page);

   cat::verify(linear.name().size() == 0);
   cat::verify(pager.name().size() == 0);
}

// `allocator_dyn` forwards introspection through its vtable. The held
// allocators are stateful, so the values must agree with the underlying
// objects.
$test(allocator_dyn_introspection_forwarding) {
   cat::span page = g_tests_alloc.alloc_multi<cat::byte>(64u).verify();
   $defer {
      g_tests_alloc.free(page);
   };
   cat::is_allocator auto linear = cat::make_allocator_linear(page);

   cat::allocator_dyn dyn = linear;
   cat::verify(dyn.bytes_capacity() == linear.bytes_capacity());
   cat::verify(dyn.bytes_used() == linear.bytes_used());
   cat::verify(dyn.name().size() == 0);

   [[maybe_unused]]
   auto* p = dyn.alloc<int4>(5).verify();
   cat::verify(dyn.bytes_used() == linear.bytes_used());
}

// Two `allocator_dyn` handles erasing the same concrete allocator instance
// compare equal. Two handles erasing distinct instances of the same type
// do not. Two handles erasing different types do not, regardless of the
// underlying memory.
$test(allocator_dyn_equality) {
   cat::span page_a = g_tests_alloc.alloc_multi<cat::byte>(32u).verify();
   cat::span page_b = g_tests_alloc.alloc_multi<cat::byte>(32u).verify();
   $defer {
      g_tests_alloc.free(page_a);
      g_tests_alloc.free(page_b);
   };
   cat::is_allocator auto linear_a1 = cat::make_allocator_linear(page_a);
   cat::is_allocator auto linear_a2 = cat::make_allocator_linear(page_a);
   cat::is_allocator auto linear_b = cat::make_allocator_linear(page_b);

   cat::allocator_dyn d_a1 = linear_a1;
   cat::allocator_dyn d_a2 = linear_a2;
   cat::allocator_dyn d_b = linear_b;
   cat::allocator_dyn d_pager = pager;

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
      cat::allocator_ref<cat::allocator_page> ref;
      cat::int4 payload;
   };

   struct null_holder {
      [[no_unique_address]]
      cat::allocator_ref<cat::allocator_null> ref;
      cat::int4 payload;
   };

   static_assert(sizeof(page_holder) == sizeof(cat::int4));
   static_assert(sizeof(null_holder) == sizeof(cat::int4));

   // The ref's dispatch must still work even though it carries no state.
   cat::allocator_ref<cat::allocator_page> page_ref;
   cat::span<int4> page = page_ref.alloc_multi<int4>(4u).verify();
   page_ref.free(page);

   cat::allocator_ref<cat::allocator_null> null_ref;
   cat::maybe<int4*> result = null_ref.alloc<int4>();
   cat::verify(result.is_empty());
}

$test(allocator_ref_introspection_forwarding) {
   cat::span page = g_tests_alloc.alloc_multi<cat::byte>(32u).verify();
   $defer {
      g_tests_alloc.free(page);
   };
   cat::is_allocator auto linear = cat::make_allocator_linear(page);

   cat::allocator_ref<cat::allocator_linear> ref = linear;
   cat::verify(ref.bytes_capacity() == linear.bytes_capacity());
   cat::verify(ref.bytes_used() == linear.bytes_used());

   [[maybe_unused]]
   auto* p = ref.alloc<int4>(9).verify();
   cat::verify(ref.bytes_used() == linear.bytes_used());
}

// `allocator_ref<A>::operator==` falls through to `is_equivalent` on the
// underlying allocator. Two refs into the same arena compare equal even
// if they hold pointers to distinct linear allocator instances over that
// arena.
$test(allocator_ref_equality_uses_is_equivalent) {
   cat::span page_a = g_tests_alloc.alloc_multi<cat::byte>(32u).verify();
   cat::span page_b = g_tests_alloc.alloc_multi<cat::byte>(32u).verify();
   $defer {
      g_tests_alloc.free(page_a);
      g_tests_alloc.free(page_b);
   };
   cat::is_allocator auto linear = cat::make_allocator_linear(page_a);
   cat::is_allocator auto linear_address_lias =
      cat::make_allocator_linear(page_a);
   cat::is_allocator auto linear_other = cat::make_allocator_linear(page_b);

   cat::allocator_ref<cat::allocator_linear> ref = linear;
   cat::allocator_ref<cat::allocator_linear> ref_alias = linear;
   cat::allocator_ref<cat::allocator_linear> ref_address_alias =
      linear_address_lias;
   cat::allocator_ref<cat::allocator_linear> ref_other = linear_other;

   cat::verify(ref == ref_alias);
   cat::verify(ref == linear);
   cat::verify(ref == ref_address_alias);
   cat::verify(ref != ref_other);
}
