// Tests for `allocator_named<Inner>`, which forwards every allocator operation
// to `Inner` while attaching a runtime debug label.

#include <cat/allocator_linear>
#include <cat/allocator_named>
#include <cat/allocator_page>
#include <cat/allocator_parameters>
#include <cat/string>

#include "../unit_tests.hpp"

// Static alignment guarantees re-export from `Inner` so the alignment fast
// path in `meta_alloc` still sees the wrapped allocator's actual minimum.
static_assert(
   cat::allocator_named<cat::allocator_linear>::min_alignment
   == cat::allocator_linear::min_alignment
);
static_assert(
   cat::allocator_named<cat::allocator_page>::min_alignment
   == cat::allocator_page::min_alignment
);

// `allocator_named::name()` must return the exact `str_view` it was built
// with. The label is the wrapper's only added state.
$test(allocator_named_carries_runtime_name) {
   cat::span page = pager.alloc_multi<cat::byte>(64u).verify();
   $defer {
      pager.free(page);
   };
   cat::is_allocator auto inner = cat::make_allocator_linear(page);
   cat::allocator_named named{inner, cat::str_view{"my_arena"}};

   cat::verify(named.name() == cat::str_view{"my_arena"});
   cat::verify(named.name().size() == 8u);
}

// Every dispatch hook must forward to the wrapped allocator. Use the public
// allocation interface to verify the round trip and then watch the inner's
// own `bytes_used` move in lockstep with the wrapper's.
$test(allocator_named_forwards_alloc_and_introspection) {
   cat::span page = pager.alloc_multi<cat::byte>(64u).verify();
   $defer {
      pager.free(page);
   };
   cat::is_allocator auto inner = cat::make_allocator_linear(page);
   cat::allocator_named named{inner, cat::str_view{"forward"}};

   cat::verify(named.bytes_capacity() == inner.bytes_capacity());
   cat::verify(named.bytes_used() == 0u);

   [[maybe_unused]]
   auto* p = named.alloc<cat::int4>(7).verify();
   cat::verify(named.bytes_used() == inner.bytes_used());
   cat::verify(named.bytes_used() >= 4u);
}

// Equality ignores the debug label by design. Two `allocator_named` wrappers
// over the *same* inner allocator must compare equivalent even when carrying
// different names.
$test(allocator_named_equality_ignores_label) {
   cat::span page = pager.alloc_multi<cat::byte>(32u).verify();
   $defer {
      pager.free(page);
   };
   cat::is_allocator auto inner = cat::make_allocator_linear(page);
   cat::allocator_named named_a{inner, cat::str_view{"alpha"}};
   cat::allocator_named named_b{inner, cat::str_view{"beta"}};

   cat::verify(named_a.is_equivalent(named_b));
   cat::verify(named_a == named_b);
}

// Two `allocator_named` wrappers over *different* inner instances over
// different arenas must NOT compare equivalent, even if their labels match.
$test(allocator_named_equality_distinguishes_distinct_inners) {
   cat::span page_a = pager.alloc_multi<cat::byte>(32u).verify();
   cat::span page_b = pager.alloc_multi<cat::byte>(32u).verify();
   $defer {
      pager.free(page_a);
      pager.free(page_b);
   };
   cat::is_allocator auto inner_a = cat::make_allocator_linear(page_a);
   cat::is_allocator auto inner_b = cat::make_allocator_linear(page_b);
   cat::allocator_named wrap_a{inner_a, cat::str_view{"same"}};
   cat::allocator_named wrap_b{inner_b, cat::str_view{"same"}};

   cat::verify(!wrap_a.is_equivalent(wrap_b));
   cat::verify(!(wrap_a == wrap_b));
}

// `reset` must forward through the wrapper.
$test(allocator_named_reset_forwards) {
   cat::span page = pager.alloc_multi<cat::byte>(32u).verify();
   $defer {
      pager.free(page);
   };
   cat::is_allocator auto inner = cat::make_allocator_linear(page);
   cat::allocator_named named{inner, cat::str_view{"resettable"}};

   [[maybe_unused]]
   auto* p = named.alloc<cat::int4>(1).verify();
   cat::verify(named.bytes_used() > 0u);

   named.reset();
   cat::verify(named.bytes_used() == 0u);
}
