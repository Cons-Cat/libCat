#include <cat/allocator_parameters>
#include <cat/bit>
#include <cat/linear_allocator>
#include <cat/page_allocator>
#include <cat/pool_allocator>

#include "../unit_tests.hpp"

template <typename Allocator>
concept has_bytes_used =
   requires(Allocator const& allocator) { allocator.bytes_used(); };

$test(dyn_allocator_linear) {
   // `linear_allocator` provides native `aligned_allocate` and
   // `aligned_allocate_feedback`, so this exercises the direct-dispatch paths
   // of the `dyn` vtable.
   cat::span page = pager.alloc_multi<cat::byte>(1_uki).verify();
   $defer {
      pager.free(page);
   };
   cat::is_allocator auto linear = cat::make_linear_allocator(page);

   cat::dyn_allocator dyn = linear;

   int4* p_a = dyn.alloc<int4>(7).verify();
   cat::verify(*p_a == 7);
   dyn.free(p_a);

   int4* p_b = dyn.alloc<int4>(11).verify();
   cat::verify(*p_b == 11);

   // Native `aligned_allocate` path.
   auto* p_aligned = dyn.align_alloc<int4>(64u, 9).verify();
   cat::verify(*p_aligned == 9);
   cat::verify(cat::is_aligned(p_aligned, 64u));

   // Feedback path. `linear_allocator` only provides
   // `aligned_allocate_feedback`, so the unaligned `inline_alloc` for an `int1`
   // exercises the `allocate_feedback` fallback shim in `dyn_allocator`.
   auto handle = dyn.inline_alloc<int1>(3).verify();
   dyn.free(handle);

   // `reset` is forwarded through the vtable to the held linear allocator.
   dyn.reset();
   int4* p_after = dyn.alloc<int4>(13).verify();
   cat::verify(*p_after == 13);
}

$test(dyn_allocator_pool) {
   // `pool_allocator` has none of the aligned or feedback hooks, so every
   // aligned/feedback call through `dyn_allocator` exercises a fallback shim.
   cat::span page = pager.alloc_multi<cat::byte>(128u).verify();
   $defer {
      pager.free(page);
   };
   cat::is_allocator auto pool = cat::make_pool_allocator<8>(page);

   cat::dyn_allocator dyn = pool;

   int4* p_a = dyn.alloc<int4>(21).verify();
   cat::verify(*p_a == 21);
   dyn.free(p_a);

   // `pool_allocator` hands out 8-byte slots that are naturally 8-byte aligned,
   // so the `aligned_allocate` fallback's runtime assertion is satisfied.
   auto* p_aligned = dyn.align_alloc<int4>(8u, 5).verify();
   cat::verify(*p_aligned == 5);
   cat::verify(cat::is_aligned(p_aligned, 8u));

   dyn.reset();
   int4* p_after = dyn.alloc<int4>(33).verify();
   cat::verify(*p_after == 33);
}

$test(dyn_allocator_page) {
   // `page_allocator` provides `aligned_allocate` but neither feedback hook, so
   // the feedback paths exercise their shims.
   cat::dyn_allocator dyn = pager;

   cat::span<int4> values = dyn.alloc_multi<int4>(3u).verify();
   cat::verify(values.size() == 3);
   dyn.free(values);

   auto* p_aligned = dyn.align_alloc<int4>(64u, 17).verify();
   cat::verify(*p_aligned == 17);
   cat::verify(cat::is_aligned(p_aligned, 64u));

   // `inline_alloc` routes through the feedback shim because `page_allocator`
   // has no `allocate_feedback` of its own.
   auto handle = dyn.inline_alloc<int4>(99).verify();
   dyn.free(handle);
}

$test(basic_dyn_allocator_methods) {
   static_assert(
      cat::is_allocator<cat::basic_dyn_allocator<cat::dyn_reallocate>>
   );
   static_assert(
      !has_bytes_used<cat::basic_dyn_allocator<cat::dyn_reallocate>>
   );

   cat::basic_dyn_allocator<cat::dyn_reallocate> dyn = pager;
   int4* p_value = dyn.alloc<int4>(27).verify();
   cat::verify(*p_value == 27);
   dyn.free(p_value);

   cat::dyn_allocator full = pager;
   cat::basic_dyn_allocator<cat::dyn_reallocate> narrowed = full;
   int4* p_narrowed = narrowed.alloc<int4>(31).verify();
   cat::verify(*p_narrowed == 31);
   narrowed.free(p_narrowed);
}

$test(realloc_uninit) {
   {
      int4* p_value = pager.alloc<int4>(17).verify();
      p_value = pager.realloc_uninit(p_value).verify();
      $defer {
         pager.free_uninit(p_value);
      };
      cat::verify(*p_value == 17);
   }

   {
      cat::span<int4> values = pager.alloc_multi_uninit<int4>(2u).verify();
      values[0] = 23;
      values[1] = 29;
      values =
         pager.realloc_multi_uninit(values.data(), values.size(), 4u).verify();
      $defer {
         pager.free_multi_uninit(values);
      };
      cat::verify(values[0] == 23);
      cat::verify(values[1] == 29);
   }

   {
      int4* p_feedback = pager.alloc<int4>(43).verify();
      auto [p_resized, allocation_bytes] =
         pager.resalloc_uninit(p_feedback).verify();
      $defer {
         pager.free_uninit(p_resized);
      };
      cat::verify(*p_resized == 43);
      cat::verify(allocation_bytes >= sizeof(int4));
   }

   {
      cat::span<int4> feedback_values =
         pager.alloc_multi_uninit<int4>(2u).verify();
      feedback_values[0] = 47;
      feedback_values[1] = 53;
      auto [resized_values, resized_bytes] =
         pager
            .resalloc_multi_uninit(
               feedback_values.data(), feedback_values.size(), 4u
            )
            .verify();
      $defer {
         pager.free_multi_uninit(resized_values);
      };
      cat::verify(resized_values[0] == 47);
      cat::verify(resized_values[1] == 53);
      cat::verify(resized_bytes >= sizeof(int4) * 4u);
   }

   {
      auto handle = pager.inline_alloc<int4>(31).verify();
      handle = pager.inline_realloc_uninit(handle).verify();
      $defer {
         pager.free_uninit(handle);
      };
      cat::verify(pager.get(handle) == 31);
   }

   {
      auto handle = pager.inline_alloc<int4>(59).verify();
      auto [resized_handle, allocation_bytes] =
         pager.inline_resalloc_uninit(handle).verify();
      $defer {
         pager.free_uninit(resized_handle);
      };
      cat::verify(pager.get(resized_handle) == 59);
      cat::verify(allocation_bytes >= sizeof(int4));
   }

   {
      auto multi_handle = pager.inline_alloc_multi_uninit<int4>(2u).verify();
      cat::span inline_values = pager.get(multi_handle);
      inline_values[0] = 37;
      inline_values[1] = 41;
      auto resized_handle =
         pager.inline_realloc_multi_uninit(multi_handle, 4u).verify();
      $defer {
         pager.free_uninit(resized_handle);
      };
      inline_values = pager.get(resized_handle);
      cat::verify(inline_values[0] == 37);
      cat::verify(inline_values[1] == 41);
   }
}

$test(allocator_ref) {
   // `cat::allocator_ref<Allocator>` is the static-dispatch counterpart to
   // `dyn_allocator`: it stores a non-owning pointer to the held allocator and
   // forwards `allocator_interface` calls through to it.
   static_assert(cat::is_allocator<cat::allocator_ref<cat::page_allocator>>);

   cat::span page = pager.alloc_multi<cat::byte>(1_uki).verify();
   $defer {
      pager.free(page);
   };
   cat::is_allocator auto linear = cat::make_linear_allocator(page);

   cat::allocator_ref<cat::linear_allocator> ref = linear;

   int4* p_int = ref.alloc<int4>(42).verify();
   cat::verify(*p_int == 42);

   // `ref` is a non-owning handle, so allocations made through it and through
   // the wrapped allocator come from the same arena and produce distinct
   // addresses.
   int4* p_direct = linear.alloc<int4>(43).verify();
   cat::verify(*p_direct == 43);
   cat::verify(p_direct != p_int);

   // Reset is forwarded, so the held allocator's bump pointer is back at the
   // start of the arena. The new allocation through `ref` lands back at
   // `p_int`'s address, observable from the wrapped allocator's state.
   ref.reset();
   int4* p_after = ref.alloc<int4>(7).verify();
   cat::verify(*p_after == 7);
   cat::verify(p_after == p_int);
}

// Constructing an `allocator_ref` from another `allocator_ref` must not
// nest. CTAD's deduction guide picks the inner allocator's type, and the
// copy constructor binds straight to the same underlying allocator without
// adding a pointer hop.
$test(allocator_ref_unwraps_self) {
   cat::span page = pager.alloc_multi<cat::byte>(1_uki).verify();
   $defer {
      pager.free(page);
   };
   cat::is_allocator auto linear = cat::make_linear_allocator(page);

   cat::allocator_ref<cat::linear_allocator> ref = linear;
   cat::allocator_ref deduced = ref;
   static_assert(
      cat::is_same<decltype(deduced), cat::allocator_ref<cat::linear_allocator>>
   );

   // Allocations through `deduced` and `ref` come from the same arena, so a
   // `reset` through one observes through the other.
   int4* p_a = deduced.alloc<int4>(1).verify();
   ref.reset();
   int4* p_b = deduced.alloc<int4>(2).verify();
   cat::verify(p_a == p_b);
}

// Constructing a `dyn_allocator` from another `dyn_allocator` must not nest
// the held `dyn_ptr`. The copy constructor shares the inner pointer, so a
// call through the second instance reaches the underlying allocator with one
// `dyn_invoke` hop, not two.
$test(dyn_allocator_unwraps_self) {
   cat::span page = pager.alloc_multi<cat::byte>(1_uki).verify();
   $defer {
      pager.free(page);
   };
   cat::is_allocator auto linear = cat::make_linear_allocator(page);

   cat::dyn_allocator outer = linear;
   cat::dyn_allocator copy = outer;

   int4* p_a = copy.alloc<int4>(5).verify();
   outer.reset();
   int4* p_b = copy.alloc<int4>(6).verify();
   cat::verify(p_a == p_b);
}
