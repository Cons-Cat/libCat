// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/allocator>

namespace cat {

template <typename Derived>
// Handle and return types implied by the same flags as `meta_alloc`
template <typename T, bool is_inline, bool is_fail_safe, bool is_aligned,
          bool is_multiple, bool is_zeroed, bool has_feedback>
struct allocator_interface<Derived>::meta_alloc_alias_types {
   using underlying_handle =
      decltype(declval<Derived&>().template make_handle<T>(declval<T*>()));

   using maybe_allocation =
      conditional<has_feedback, maybe_sized_allocation<void*>, maybe_ptr<void>>;

   using handle_type = conditional<
      is_multiple,
      detail::multi_memory_handle<
         conditional<is_inline, detail::inline_memory_handle<underlying_handle>,
                     underlying_handle>>,
      conditional<is_inline, detail::inline_memory_handle<underlying_handle>,
                  underlying_handle>>;

   using return_handle = conditional<
      is_fail_safe,
      conditional<
         !is_inline,
         conditional<has_feedback,
                     conditional<is_multiple, maybe<tuple<span<T>, idx>>,
                                 maybe_sized_allocation<T*>>,
                     conditional<is_multiple, maybe_span<T>, maybe_ptr<T>>>,
         conditional<has_feedback, maybe<tuple<handle_type, idx>>,
                     maybe<handle_type>>>,
      conditional<
         !is_inline,
         conditional<
            has_feedback,
            conditional<is_multiple, tuple<span<T>, idx>, sized_allocation<T*>>,
            conditional<is_multiple, span<T>, T*>>,
         conditional<has_feedback, tuple<handle_type, idx>, handle_type>>>;
};

template <typename Derived>
// Only called from `meta_alloc` when `is_inline` is true; alias_types always
// use `true` for the inline slot so this is not instantiated per
// `is_inline`.
template <typename T, bool is_fail_safe, bool is_aligned, bool is_multiple,
          bool is_zeroed, bool has_feedback, typename... Args>
[[gnu::no_sanitize_address]]
constexpr auto
allocator_interface<Derived>::meta_alloc_inline_stack_allocate(
   idx allocation_bytes, idx allocation_count, Args&&... arguments) {
   using alias_types =
      meta_alloc_alias_types<T, true, is_fail_safe, is_aligned, is_multiple,
                             is_zeroed, has_feedback>;
   using return_handle = typename alias_types::return_handle;
   using handle_type = typename alias_types::handle_type;

   if (allocation_bytes < inline_buffer_size) {
      // Allocate memory on this stack frame.
      handle_type stack_handle;
      stack_handle.set_inlined(true);
      stack_handle.set_count(allocation_count);

      if constexpr (is_zeroed) {
         zero_memory(__builtin_addressof(stack_handle), inline_buffer_size);
      }

      if constexpr (is_multiple) {
         for (unsigned long i = 0u; i < allocation_count; ++i) {
            new (reinterpret_cast<T*>(&stack_handle) + i) T;
         }
      } else {
         if constexpr (is_zeroed) {
            stack_handle.set_inline_storage(T{});
         } else {
            stack_handle.set_inline_storage(T(fwd(arguments)...));
         }
      }

      // Return here to skip error handling, because an on-stack allocation
      // cannot fail.
      if constexpr (has_feedback) {
         return return_handle(
            tuple<handle_type, idx>{move(stack_handle), inline_buffer_size});
      } else {
         return return_handle(move(stack_handle));
      }
   }
   __builtin_unreachable();
}

template <typename Derived>
template <typename T, bool is_fail_safe, bool is_aligned, bool is_multiple,
          bool is_zeroed, bool has_feedback, typename... Args>
consteval auto
allocator_interface<Derived>::meta_alloc_consteval_inline_allocate(
   idx allocation_bytes, idx allocation_count, Args&&... arguments) {
   using alias_types =
      meta_alloc_alias_types<T, true, is_fail_safe, is_aligned, is_multiple,
                             is_zeroed, has_feedback>;
   using return_handle = typename alias_types::return_handle;
   using handle_type = typename alias_types::handle_type;

   if (allocation_bytes < inline_buffer_size) {
      handle_type stack_handle;
      stack_handle.set_inlined(true);
      stack_handle.set_count(allocation_count);

      if constexpr (is_zeroed) {
         zero_memory(__builtin_addressof(stack_handle), inline_buffer_size);
      }

      if constexpr (is_multiple) {
         for (unsigned long i = 0u; i < allocation_count; ++i) {
            new (reinterpret_cast<T*>(&stack_handle) + i) T;
         }
      } else {
         if constexpr (is_zeroed) {
            stack_handle.set_inline_storage(T{});
         } else {
            stack_handle.set_inline_storage(T(fwd(arguments)...));
         }
      }

      if constexpr (has_feedback) {
         return return_handle(
            tuple<handle_type, idx>{move(stack_handle), inline_buffer_size});
      } else {
         return return_handle(move(stack_handle));
      }
   }
   __builtin_unreachable();
}

template <typename Derived>
[[gnu::no_sanitize_address]]
constexpr auto
allocator_interface<Derived>::
   meta_alloc_obtain_maybe_memory_aligned_with_feedback(
      uword allocation_alignment, idx allocation_bytes)
      -> maybe_sized_allocation<void*> {
   maybe_sized_allocation<void*> maybe_memory;

   if constexpr (detail::has_aligned_allocate_feedback<Derived>) {
      maybe_memory = this->self().aligned_allocate_feedback(
         allocation_alignment, allocation_bytes);
   } else {
      if constexpr (detail::has_allocation_bytes<Derived>) {
         maybe actual_allocation_bytes = this->self().allocation_bytes(
            allocation_alignment, allocation_bytes);

         if (actual_allocation_bytes.has_value()) {
            void* p_allocation =
               this->self()
                  .aligned_allocate(allocation_alignment,
                                    actual_allocation_bytes.value())
                  .value();

            maybe_memory = maybe_sized_allocation<void*>{
               sized_allocation<void*>{p_allocation,
                                       actual_allocation_bytes.value()}
            };
         } else {
            maybe_memory = nullopt;
         }
      } else {
         maybe temp_memory = this->self().aligned_allocate(allocation_alignment,
                                                           allocation_bytes);
         if (temp_memory.has_value()) {
            maybe_memory = maybe_sized_allocation<void*>{
               sized_allocation<void*>{temp_memory.value(), allocation_bytes}
            };
         } else {
            maybe_memory = nullopt;
         }
      }
   }

   return maybe_memory;
}

template <typename Derived>
[[gnu::no_sanitize_address]]
constexpr auto
allocator_interface<Derived>::
   meta_alloc_obtain_maybe_memory_aligned_no_feedback(
      uword allocation_alignment, idx allocation_bytes) -> maybe_ptr<void> {
   maybe_ptr<void> maybe_memory;

   if constexpr (detail::has_aligned_allocate<Derived>) {
      maybe_memory =
         this->self().aligned_allocate(allocation_alignment, allocation_bytes);
   } else {
      maybe_memory = this->self().allocate(allocation_bytes);
      if (maybe_memory.has_value()) {
         assert(cat::is_aligned(maybe_memory.value(), allocation_alignment),
                "allocation_type is misaligned!");
      }
   }

   return maybe_memory;
}

template <typename Derived>
[[gnu::no_sanitize_address]]
constexpr auto
allocator_interface<Derived>::
   meta_alloc_obtain_maybe_memory_unaligned_with_feedback(
      uword allocation_alignment, idx allocation_bytes)
      -> maybe_sized_allocation<void*> {
   maybe_sized_allocation<void*> maybe_memory;

   if constexpr (detail::has_allocate_feedback<Derived>) {
      maybe_memory = this->self().allocate_feedback(allocation_bytes);
   } else {
      if constexpr (detail::has_aligned_allocate_feedback<Derived>) {
         maybe_memory =
            this->self().aligned_allocate_feedback(1u, allocation_bytes);
      } else if constexpr (detail::has_allocation_bytes<Derived>) {
         maybe size = this->self().allocation_bytes(1u, allocation_bytes);

         if (size.has_value()) {
            maybe_memory = maybe_sized_allocation<void*>(tuple{
               this->self().allocate(allocation_bytes).value(), size.value()});
         } else {
            maybe_memory = nullopt;
         }
      } else {
         auto temp_memory = this->self().aligned_allocate(allocation_alignment,
                                                          allocation_bytes);
         if (temp_memory.has_value()) {
            maybe_memory = maybe_sized_allocation<void*>(
               tuple{temp_memory.value(), allocation_bytes});
         } else {
            maybe_memory = nullopt;
         }
      }
   }

   return maybe_memory;
}

template <typename Derived>
[[gnu::no_sanitize_address]]
constexpr auto
allocator_interface<Derived>::
   meta_alloc_obtain_maybe_memory_unaligned_no_feedback(idx allocation_bytes)
      -> maybe_ptr<void> {
   maybe_ptr<void> maybe_memory;

   if constexpr (detail::has_allocate<Derived>) {
      maybe_memory = this->self().allocate(allocation_bytes);
   } else {
      maybe_memory = this->self().aligned_allocate(1u, allocation_bytes);
   }

   return maybe_memory;
}

template <typename Derived>
template <typename T, bool is_fail_safe, bool is_aligned, bool is_multiple,
          bool is_zeroed, bool has_feedback, typename... Args>
[[gnu::no_sanitize_address]]
constexpr auto
allocator_interface<Derived>::meta_alloc_finish_non_inline(
   meta_alloc_raw_maybe_allocation<has_feedback> maybe_memory,
   idx allocation_bytes, idx allocation_count, Args&&... arguments) {
   using alias_types =
      meta_alloc_alias_types<T, false, is_fail_safe, is_aligned, is_multiple,
                             is_zeroed, has_feedback>;
   using return_handle = typename alias_types::return_handle;
   T* p_allocation;
   if constexpr (has_feedback) {
      // The `.first()` element of the `sized_allocation` tuple is
      // a `void*`.
      p_allocation = static_cast<T*>(maybe_memory.value().first());

      auto const all_bytes = maybe_memory.value().second();
      // Unpoison all allocated memory so it can be used.
      unpoison_memory_region(p_allocation, all_bytes);

      // Possibly zero-out the allocation.
      if constexpr (is_zeroed) {
         zero_memory_scalar_explicit(p_allocation, all_bytes);
      }
   } else {
      p_allocation = static_cast<T*>(maybe_memory.value());
      unpoison_memory_region(p_allocation, allocation_bytes);

      // Possibly zero-out the allocation.
      if constexpr (is_zeroed) {
         // TODO: Find a way to efficiently and safely leverage SIMD
         // here.
         zero_memory_scalar_explicit(p_allocation, allocation_bytes);
      }
   }

   // Construct the `T`s inside the allocator.
   for (auto i = 0ull; i < allocation_count; ++i) {
      if constexpr (is_zeroed) {
         new (p_allocation + i) T;
      } else {
         new (p_allocation + i) T(fwd(arguments)...);
      }
   }

   // If this allocation has size feedback, store it.
   idx size_feedback;
   if constexpr (has_feedback) {
      size_feedback = maybe_memory.value().second();
   }

   if constexpr (has_feedback) {
      if constexpr (is_multiple) {
         return return_handle(
            tuple{span<T>(p_allocation, allocation_count), size_feedback});
      } else {
         return return_handle(
            sized_allocation<T*>{p_allocation, size_feedback});
      }
   } else {
      // No size feedback.
      if constexpr (is_multiple) {
         return return_handle(span<T>{p_allocation, allocation_count});
      } else {
         return return_handle(p_allocation);
      }
   }
}

template <typename Derived>
template <typename T, bool is_fail_safe, bool is_aligned, bool is_multiple,
          bool is_zeroed, bool has_feedback, typename... Args>
[[gnu::no_sanitize_address]]
constexpr auto
allocator_interface<Derived>::meta_alloc_finish_inline_overflow(
   meta_alloc_raw_maybe_allocation<has_feedback> maybe_memory,
   idx allocation_bytes, idx allocation_count, Args&&... arguments) {
   using alias_types =
      meta_alloc_alias_types<T, true, is_fail_safe, is_aligned, is_multiple,
                             is_zeroed, has_feedback>;
   using return_handle = typename alias_types::return_handle;
   using handle_type = typename alias_types::handle_type;
   using underlying_handle = typename alias_types::underlying_handle;
   T* p_allocation;
   if constexpr (has_feedback) {
      // The `.first()` element of the `sized_allocation` tuple is
      // a `void*`.
      p_allocation = static_cast<T*>(maybe_memory.value().first());

      auto const all_bytes = maybe_memory.value().second();
      // Unpoison all allocated memory so it can be used.
      unpoison_memory_region(p_allocation, all_bytes);

      // Possibly zero-out the allocation.
      if constexpr (is_zeroed) {
         zero_memory_scalar_explicit(p_allocation, all_bytes);
      }
   } else {
      p_allocation = static_cast<T*>(maybe_memory.value());
      unpoison_memory_region(p_allocation, allocation_bytes);

      // Possibly zero-out the allocation.
      if constexpr (is_zeroed) {
         // TODO: Find a way to efficiently and safely leverage SIMD
         // here.
         zero_memory_scalar_explicit(p_allocation, allocation_bytes);
      }
   }

   // Construct the `T`s inside the allocator.
   for (auto i = 0ull; i < allocation_count; ++i) {
      if constexpr (is_zeroed) {
         new (p_allocation + i) T;
      } else {
         new (p_allocation + i) T(fwd(arguments)...);
      }
   }

   // If this allocation has size feedback, store it.
   idx size_feedback;
   if constexpr (has_feedback) {
      size_feedback = maybe_memory.value().second();
   }

   underlying_handle const raw_handle =
      this->self().template make_handle<T>(p_allocation);
   handle_type handle(move(raw_handle));

   handle.set_inlined(false);

   if constexpr (is_multiple) {
      handle.set_count(allocation_count);
   }

   if constexpr (has_feedback) {
      return return_handle(
         tuple<handle_type, idx>{move(handle), size_feedback});
   } else {
      return return_handle(move(handle));
   }
}

template <typename Derived>
template <typename T, bool is_fail_safe, bool is_aligned, bool is_multiple,
          bool is_zeroed, bool has_feedback, typename... Args>
consteval auto
allocator_interface<Derived>::meta_alloc_consteval_allocate(
   uword allocation_alignment, idx allocation_count, Args&&... arguments) {
   using alias_types =
      meta_alloc_alias_types<T, false, is_fail_safe, is_aligned, is_multiple,
                             is_zeroed, has_feedback>;
   using return_handle = typename alias_types::return_handle;

   T* p_allocation = nullptr;

   // Clang rejects `new (std::align_val_t(...))` in constant expressions for
   // many cases. Default `new` is enough when the requested alignment is 1
   // (unalign path) or matches `alignof(T)` (align path for normal types).
   bool const plain_new_ok = (allocation_alignment == uword(1))
                             || (allocation_alignment == uword(alignof(T)));

   if constexpr (is_multiple) {
      if (plain_new_ok) {
         p_allocation = new T[static_cast<unsigned long>(allocation_count.raw)];
      } else {
         p_allocation = new (std::align_val_t(allocation_alignment.raw))
            T[allocation_count.raw];
      }
   } else {
      if constexpr (is_zeroed) {
         if (plain_new_ok) {
            p_allocation = new T;
         } else {
            p_allocation = new (std::align_val_t(allocation_alignment.raw)) T;
         }
      } else {
         if (plain_new_ok) {
            p_allocation = new T(arguments...);
         } else {
            p_allocation =
               new (std::align_val_t(allocation_alignment.raw)) T(arguments...);
         }
      }
   }

   for (unsigned long i = 0u; i < allocation_count; ++i) {
      new (p_allocation + i) T;
   }

   if constexpr (has_feedback) {
      if constexpr (is_multiple) {
         return return_handle(tuple{span<T>(p_allocation, allocation_count),
                                    allocation_count * sizeof(T)});
      } else {
         return return_handle(
            sized_allocation<T*>{p_allocation, allocation_count * sizeof(T)});
      }
   } else {
      if constexpr (is_multiple) {
         return return_handle(span<T>(p_allocation, allocation_count));
      } else {
         return return_handle(p_allocation);
      }
   }
}

template <typename Derived>
// `is_inline` tries the inline buffer before the heap,
// `is_fail_safe` maps allocation failure to maybe-style results,
// `is_aligned` selects aligned allocation entry points,
// `is_multiple` allocates arrays (span handles) instead of one `T`,
// `is_zeroed` uses implicit-lifetime zeroing instead of constructor args,
// `has_feedback` returns sized byte counts when the allocator supports them
template <typename T, bool is_inline, bool is_fail_safe, bool is_aligned,
          bool is_multiple, bool is_zeroed, bool has_feedback, typename... Args>
   requires((!is_multiple || (sizeof...(Args) <= 0))
            // Multi-allocations must be default-constructed.
            // Zeroed-out allocations must not take constructor arguments and
            // must use a trivial object type.
            && (!is_zeroed || (sizeof...(Args) == 0))
            && (!is_zeroed || is_implicit_lifetime<T>))
constexpr auto
allocator_interface<Derived>::meta_alloc(uword allocation_alignment,
                                         idx allocation_count,
                                         Args&&... arguments) {
   if constexpr (detail::has_max_allocation_bytes<Derived>) {
      static_assert(sizeof(T) <= Derived::max_allocation_bytes,
                    "This allocation is too large for this allocator!");
      assert((allocation_count * sizeof(T)) <= Derived::max_allocation_bytes,
             "This allocation is too larger for this allocator!");
   }

   idx const allocation_bytes = allocation_count * sizeof(T);

   if consteval {
      if constexpr (!is_inline) {
         return this->template meta_alloc_consteval_allocate<
            T, is_fail_safe, is_aligned, is_multiple, is_zeroed, has_feedback>(
            allocation_alignment, allocation_count, fwd(arguments)...);
      } else {
         return this->template meta_alloc_consteval_inline_allocate<
            T, is_fail_safe, is_aligned, is_multiple, is_zeroed, has_feedback>(
            allocation_bytes, allocation_count, fwd(arguments)...);
      }
   } else {
      using alias_types =
         meta_alloc_alias_types<T, is_inline, is_fail_safe, is_aligned,
                                is_multiple, is_zeroed, has_feedback>;
      if constexpr (is_inline) {
         if (allocation_bytes < inline_buffer_size) {
            return this->template meta_alloc_inline_stack_allocate<
               T, is_fail_safe, is_aligned, is_multiple, is_zeroed,
               has_feedback>(allocation_bytes, allocation_count,
                             fwd(arguments)...);
         }
      }

      meta_alloc_raw_maybe_allocation<has_feedback> maybe_memory;

      if constexpr (is_aligned) {
         if constexpr (has_feedback) {
            maybe_memory =
               this->meta_alloc_obtain_maybe_memory_aligned_with_feedback(
                  allocation_alignment, allocation_bytes);
         } else {
            maybe_memory =
               this->meta_alloc_obtain_maybe_memory_aligned_no_feedback(
                  allocation_alignment, allocation_bytes);
         }
      } else {
         if constexpr (has_feedback) {
            maybe_memory =
               this->meta_alloc_obtain_maybe_memory_unaligned_with_feedback(
                  allocation_alignment, allocation_bytes);
         } else {
            maybe_memory =
               this->meta_alloc_obtain_maybe_memory_unaligned_no_feedback(
                  allocation_bytes);
         }
      }
      if constexpr (is_fail_safe) {
         if (!maybe_memory.has_value()) {
            using return_handle = typename alias_types::return_handle;
            return return_handle(nullopt);
         }
      }
      if constexpr (!is_inline) {
         return this->template meta_alloc_finish_non_inline<
            T, is_fail_safe, is_aligned, is_multiple, is_zeroed, has_feedback>(
            move(maybe_memory), allocation_bytes, allocation_count,
            fwd(arguments)...);
      } else {
         return this->template meta_alloc_finish_inline_overflow<
            T, is_fail_safe, is_aligned, is_multiple, is_zeroed, has_feedback>(
            move(maybe_memory), allocation_bytes, allocation_count,
            fwd(arguments)...);
      }
   }
}

}  // namespace cat
