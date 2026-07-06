// Tests for `named_allocator<Inner>`, which forwards every allocator operation
// to `Inner` while attaching a runtime debug label.

#include <cat/allocator_parameters>
#include <cat/linear_allocator>
#include <cat/named_allocator>
#include <cat/page_allocator>

#include "../unit_tests.hpp"

// Static alignment guarantees re-export from `Inner` so the alignment fast
// path in `meta_alloc` still sees the wrapped allocator's actual minimum.
static_assert(
   cat::named_allocator<cat::linear_allocator>::min_alignment
   == cat::linear_allocator::min_alignment
);
static_assert(
   cat::named_allocator<cat::page_allocator>::min_alignment
   == cat::page_allocator::min_alignment
);

// `named_allocator::name()` must return the exact `str_view` it was built
// with. The label is the wrapper's only added state.
$test(named_allocator_carries_runtime_name) {
   cat::span page = pager.alloc_multi<cat::byte>(64u).verify();
   $defer {
      pager.free(page);
   };
   cat::is_allocator auto inner = cat::make_linear_allocator(page);
   cat::named_allocator named{inner, cat::str_view{"my_arena"}};

   cat::verify(named.name() == cat::str_view{"my_arena"});
   cat::verify(named.name().size() == 8u);
}

// Every dispatch hook must forward to the wrapped allocator. Use the public
// allocation interface to verify the round trip and then watch the inner's
// own `bytes_used` move in lockstep with the wrapper's.
$test(named_allocator_forwards_alloc_and_introspection) {
   cat::span page = pager.alloc_multi<cat::byte>(64u).verify();
   $defer {
      pager.free(page);
   };
   cat::is_allocator auto inner = cat::make_linear_allocator(page);
   cat::named_allocator named{inner, cat::str_view{"forward"}};

   cat::verify(named.bytes_capacity() == inner.bytes_capacity());
   cat::verify(named.bytes_used() == 0u);

   auto* p = named.alloc<cat::int4>(7).verify();
   static_cast<void>(p);
   cat::verify(named.bytes_used() == inner.bytes_used());
   cat::verify(named.bytes_used() >= 4u);
}

// Equality ignores the debug label by design. Two `named_allocator` wrappers
// over the *same* inner allocator must compare equivalent even when carrying
// different names.
$test(named_allocator_equality_ignores_label) {
   cat::span page = pager.alloc_multi<cat::byte>(32u).verify();
   $defer {
      pager.free(page);
   };
   cat::is_allocator auto inner = cat::make_linear_allocator(page);
   cat::named_allocator named_a{inner, cat::str_view{"alpha"}};
   cat::named_allocator named_b{inner, cat::str_view{"beta"}};

   cat::verify(named_a.is_equivalent(named_b));
   cat::verify(named_a == named_b);
}

// Two `named_allocator` wrappers over *different* inner instances over
// different arenas must NOT compare equivalent, even if their labels match.
$test(named_allocator_equality_distinguishes_distinct_inners) {
   cat::span page_a = pager.alloc_multi<cat::byte>(32u).verify();
   cat::span page_b = pager.alloc_multi<cat::byte>(32u).verify();
   $defer {
      pager.free(page_a);
      pager.free(page_b);
   };
   cat::is_allocator auto inner_a = cat::make_linear_allocator(page_a);
   cat::is_allocator auto inner_b = cat::make_linear_allocator(page_b);
   cat::named_allocator wrap_a{inner_a, cat::str_view{"same"}};
   cat::named_allocator wrap_b{inner_b, cat::str_view{"same"}};

   cat::verify(!wrap_a.is_equivalent(wrap_b));
   cat::verify(!(wrap_a == wrap_b));
}

// `reset` must forward through the wrapper.
$test(named_allocator_reset_forwards) {
   cat::span page = pager.alloc_multi<cat::byte>(32u).verify();
   $defer {
      pager.free(page);
   };
   cat::is_allocator auto inner = cat::make_linear_allocator(page);
   cat::named_allocator named{inner, cat::str_view{"resettable"}};

   auto* p = named.alloc<cat::int4>(1).verify();
   static_cast<void>(p);
   cat::verify(named.bytes_used() > 0u);

   named.reset();
   cat::verify(named.bytes_used() == 0u);
}
