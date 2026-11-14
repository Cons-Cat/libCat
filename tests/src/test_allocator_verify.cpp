// Clang `-verify` checks of the `[[gsl::Pointer]]` and
// `[[clang::lifetimebound]]` annotations in `<cat/allocator>` and the
// allocators. Any `expected-error` checks must come last, because Clang stops
// its `-Wlifetime-safety` pass after the first error in a translation unit. Run
// with `just test safety`.

#include <cat/allocator>
#include <cat/allocator_huge_page>
#include <cat/allocator_linear>
#include <cat/allocator_musl>
#include <cat/allocator_named>
#include <cat/allocator_null>
#include <cat/allocator_page>
#include <cat/allocator_pool>
#include <cat/allocator_resizeable>
#include <cat/array>
#include <cat/span>

using namespace cat::literals;
using namespace cat::arithmetic;

// Positive cases: heap allocations outlive their allocator, and handles may
// borrow an allocator that outlives them.

auto
valid_page_allocation() -> int4* {
   cat::allocator_page page = cat::make_allocator_page();
   return page.alloc<int4>(1).verify();
}

auto
valid_huge_page_allocation() -> int4* {
   auto huge_page = cat::make_allocator_huge_page();
   return huge_page.alloc<int4>(1).verify();
}

auto
valid_musl_allocation() -> int4* {
   auto musl = cat::make_allocator_musl();
   return musl.alloc<int4>(1).verify();
}

auto
valid_null_allocation() -> cat::maybe_ptr<int4> {
   auto null = cat::make_allocator_null();
   return null.alloc<int4>(1);
}

auto
valid_dyn_allocation(cat::allocator_dyn dyn) -> int4* {
   return dyn.alloc<int4>(1).verify();
}

auto
valid_dyn_borrow(cat::allocator_linear& linear [[clang::lifetimebound]])
   -> cat::allocator_dyn {
   return cat::allocator_dyn(linear);
}

auto
valid_ref_borrow(cat::allocator_linear& linear [[clang::lifetimebound]])
   -> cat::allocator_ref<cat::allocator_linear> {
   return cat::allocator_ref<cat::allocator_linear>(linear);
}

// Negative cases: an arena allocator outlives its arena.

auto
dangling_linear_arena() -> cat::allocator_linear {
   cat::array<cat::byte, 64u> buffer;
   cat::span<cat::byte> arena = buffer;
   // expected-warning@+1 2 {{local variable 'arena'}}
   return cat::make_allocator_linear(arena);
}

auto
dangling_pool_arena() -> cat::allocator_pool<16u> {
   cat::array<cat::byte, 256u> buffer;
   cat::span<cat::byte> arena = buffer;
   // expected-warning@+1 2 {{local variable 'arena'}}
   return cat::make_allocator_pool<16u>(arena);
}

auto
dangling_resizeable_backing() {
   cat::allocator_page page = cat::make_allocator_page();
   // expected-warning@+2 2 {{local variable 'page'}}
   return cat::make_allocator_resizeable<cat::allocator_pool<16u>, 16u, 4_uki>(
      page
   );
}

// Negative cases: a handle outlives its allocator.

auto
dangling_ref(cat::span<cat::byte> arena)
   -> cat::allocator_ref<cat::allocator_linear> {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+1 2 {{local variable 'linear'}}
   return cat::allocator_ref<cat::allocator_linear>(linear);
}

auto
dangling_dyn(cat::span<cat::byte> arena) -> cat::allocator_dyn {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+1 2 {{local variable 'linear'}}
   return cat::allocator_dyn(linear);
}

auto
dangling_named(cat::span<cat::byte> arena) {
   cat::allocator_linear linear = cat::make_allocator_linear(arena);
   // expected-warning@+1 2 {{local variable 'linear'}}
   return cat::allocator_named<cat::allocator_linear>(linear, "named");
}
