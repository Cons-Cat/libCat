// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/allocator>

namespace cat {

namespace detail {

// Extract the typed `T*` from a raw `meta_alloc` result and unpoison its
// prepared range. Returns the pointer paired with the prepared byte count. The
// feedback size when `has_feedback`, otherwise the requested
// `allocation_bytes`.
template <typename T, bool has_feedback>
[[gnu::no_sanitize_address, gnu::always_inline, gnu::nodebug]]
constexpr auto
meta_alloc_unpoison_memory(
   conditional<has_feedback, maybe_sized_allocation<void* _Nonnull>,
               maybe_ptr<void>> const& maybe_memory,
   idx allocation_bytes) -> tuple<T* _Nonnull, idx> {
   T* _Nonnull p_allocation;
   idx prepared_bytes;
   if constexpr (has_feedback) {
      // The `.first()` element of the `sized_allocation` tuple is a `void*`.
      p_allocation = static_cast<T* _Nonnull>(maybe_memory.value().first());
      prepared_bytes = maybe_memory.value().second();
   } else {
      p_allocation = static_cast<T* _Nonnull>(maybe_memory.value());
      prepared_bytes = allocation_bytes;
   }
   unpoison_memory_region(p_allocation, prepared_bytes);
   return tuple<T* _Nonnull, idx>{p_allocation, prepared_bytes};
}

}  // namespace detail

template <typename Derived>
// Handle and return types implied by the same flags as `meta_alloc`. Non-0
// `inline_size` enables small buffer optimization. Passing 0 disables it.
template <typename T, bool is_fail_safe, bool is_multiple, bool has_feedback,
          idx inline_size>
struct allocator_interface<Derived>::meta_alloc_alias_types {
   static constexpr bool is_inline = (inline_size != 0);

   using underlying_handle =
      decltype(declval<Derived&>().template make_handle<T>(
         declval<T* _Nonnull>()));

   using maybe_allocation =
      conditional<has_feedback, maybe_sized_allocation<void* _Nonnull>,
                  maybe_ptr<void>>;

   using handle_type = conditional<
      is_multiple,
      detail::multi_memory_handle<conditional<
         is_inline,
         detail::inline_memory_handle<underlying_handle, inline_size>,
         underlying_handle>>,
      conditional<is_inline,
                  detail::inline_memory_handle<underlying_handle, inline_size>,
                  underlying_handle>>;

   using return_handle = conditional<
      is_fail_safe,
      conditional<
         !is_inline,
         conditional<has_feedback,
                     conditional<is_multiple, maybe<tuple<span<T>, idx>>,
                                 maybe_sized_allocation<T* _Nonnull>>,
                     conditional<is_multiple, maybe_span<T>, maybe_ptr<T>>>,
         conditional<has_feedback, maybe<tuple<handle_type, idx>>,
                     maybe<handle_type>>>,
      conditional<
         !is_inline,
         conditional<has_feedback,
                     conditional<is_multiple, tuple<span<T>, idx>,
                                 sized_allocation<T* _Nonnull>>,
                     conditional<is_multiple, span<T>, T* _Nonnull>>,
         conditional<has_feedback, tuple<handle_type, idx>, handle_type>>>;
};

template <typename Derived>
// Only called from `meta_alloc` when `inline_size != 0`.
template <typename T, bool is_fail_safe, bool is_aligned, bool is_multiple,
          bool is_zeroed, bool has_feedback, idx inline_size, typename... Args>
[[gnu::no_sanitize_address]]
constexpr auto
allocator_interface<Derived>::meta_alloc_inline_stack_allocate(
   idx allocation_bytes, idx allocation_count, Args&&... arguments) {
   using alias_types = meta_alloc_alias_types<T, is_fail_safe, is_multiple,
                                              has_feedback, inline_size>;
   using return_handle = alias_types::return_handle;
   using handle_type = alias_types::handle_type;

   if (allocation_bytes <= inline_size) {
      // Allocate memory on this stack frame.
      handle_type stack_handle;
      stack_handle.set_inlined(true);
      stack_handle.set_count(allocation_count);

      if constexpr (is_zeroed) {
         zero_memory_explicit(__builtin_addressof(stack_handle), inline_size);
      }

      if constexpr (is_multiple) {
         for (idx i = 0u; i < allocation_count; ++i) {
            new (reinterpret_cast<T* _Nonnull>(&stack_handle) + i) T;
         }
      } else {
         if constexpr (is_zeroed) {
            stack_handle.set_inline_storage(T{});
         } else {
            stack_handle.set_inline_storage(T($fwd(arguments)...));
         }
      }

      // Return here to skip error handling, because an on-stack allocation
      // cannot fail.
      if constexpr (has_feedback) {
         return return_handle(
            tuple<handle_type, idx>{move(stack_handle), inline_size});
      } else {
         return return_handle(move(stack_handle));
      }
   }
}

template <typename Derived>
[[gnu::no_sanitize_address]]
constexpr auto
allocator_interface<Derived>::meta_alloc_aligned_feedback(
   ualign allocation_alignment, idx allocation_bytes)
   -> maybe_sized_allocation<void* _Nonnull> {
   maybe_sized_allocation<void* _Nonnull> maybe_memory;

   if constexpr (has_aligned_allocate_feedback) {
      maybe_memory = this->self().aligned_allocate_feedback(
         allocation_alignment, allocation_bytes);
   } else {
      if constexpr (has_allocation_bytes) {
         maybe actual_allocation_bytes = this->self().allocation_bytes(
            allocation_alignment, allocation_bytes);

         if (actual_allocation_bytes.has_value()) {
            maybe_ptr<void> const allocation = this->self().aligned_allocate(
               allocation_alignment, actual_allocation_bytes.value());

            maybe_memory = maybe_sized_allocation<void* _Nonnull>{
               sized_allocation<void* _Nonnull>{
                                                allocation.value(),
                                                actual_allocation_bytes.value(),
                                                }
            };
         } else {
            maybe_memory = nullopt;
         }
      } else {
         maybe temp_memory = this->self().aligned_allocate(allocation_alignment,
                                                           allocation_bytes);
         if (temp_memory.has_value()) {
            maybe_memory = maybe_sized_allocation<void* _Nonnull>{
               sized_allocation<void* _Nonnull>{
                                                temp_memory.value(),
                                                allocation_bytes, },
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
allocator_interface<Derived>::meta_alloc_aligned(ualign allocation_alignment,
                                                 idx allocation_bytes)
   -> maybe_ptr<void> {
   maybe_ptr<void> maybe_memory;

   if constexpr (has_aligned_allocate) {
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
allocator_interface<Derived>::meta_alloc_unaligned_feedback(
   ualign allocation_alignment, idx allocation_bytes)
   -> maybe_sized_allocation<void* _Nonnull> {
   maybe_sized_allocation<void* _Nonnull> maybe_memory;

   if constexpr (has_allocate_feedback) {
      maybe_memory = this->self().allocate_feedback(allocation_bytes);
   } else {
      if constexpr (has_aligned_allocate_feedback) {
         maybe_memory =
            this->self().aligned_allocate_feedback(1u, allocation_bytes);
      } else if constexpr (has_allocation_bytes) {
         maybe size = this->self().allocation_bytes(1u, allocation_bytes);

         if (size.has_value()) {
            maybe_ptr<void> const allocation =
               this->self().allocate(allocation_bytes);
            maybe_memory = maybe_sized_allocation<void* _Nonnull>(tuple{
               allocation.value(),
               size.value(),
            });
         } else {
            maybe_memory = nullopt;
         }
      } else {
         auto temp_memory = this->self().aligned_allocate(allocation_alignment,
                                                          allocation_bytes);
         if (temp_memory.has_value()) {
            maybe_memory = maybe_sized_allocation<void* _Nonnull>(
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
allocator_interface<Derived>::meta_alloc_unaligned(idx allocation_bytes)
   -> maybe_ptr<void> {
   maybe_ptr<void> maybe_memory;

   if constexpr (has_allocate) {
      maybe_memory = this->self().allocate(allocation_bytes);
   } else {
      maybe_memory = this->self().aligned_allocate(1u, allocation_bytes);
   }

   return maybe_memory;
}

template <typename Derived>
template <typename T, bool is_fail_safe, bool is_aligned, bool is_multiple,
          bool is_zeroed, bool has_feedback, idx inline_size, typename... Args>
[[gnu::no_sanitize_address]]
constexpr auto
allocator_interface<Derived>::meta_alloc_construct(
   meta_alloc_raw_maybe_allocation<has_feedback> maybe_memory,
   idx allocation_bytes, idx allocation_count, Args&&... arguments) {
   constexpr bool is_inline = (inline_size != 0);
   using alias_types = meta_alloc_alias_types<T, is_fail_safe, is_multiple,
                                              has_feedback, inline_size>;
   using return_handle = alias_types::return_handle;

   auto [p_allocation, prepared_bytes] =
      detail::meta_alloc_unpoison_memory<T, has_feedback>(maybe_memory,
                                                          allocation_bytes);

   // Zero-fill for the `calloc` family, unless the allocator already served the
   // request through the matching zeroed allocate hook.
   if constexpr (is_zeroed && !has_zeroed_hook<is_aligned, has_feedback>) {
      // TODO: Find a way to efficiently and safely leverage SIMD here.
      zero_memory_scalar_explicit(p_allocation, prepared_bytes);
   }

   // Construct the `T`s inside the allocator.
   for (auto i = 0ull; i < allocation_count; ++i) {
      if constexpr (is_zeroed) {
         new (p_allocation + i) T;
      } else {
         new (p_allocation + i) T($fwd(arguments)...);
      }
   }

   if constexpr (is_inline) {
      using handle_type = alias_types::handle_type;
      using underlying_handle = alias_types::underlying_handle;
      underlying_handle const raw_handle =
         this->self().template make_handle<T>(p_allocation);
      handle_type handle(move(raw_handle));
      handle.set_inlined(false);
      if constexpr (is_multiple) {
         handle.set_count(allocation_count);
      }
      if constexpr (has_feedback) {
         return return_handle(
            tuple<handle_type, idx>{move(handle), prepared_bytes});
      } else {
         return return_handle(move(handle));
      }
   } else {
      if constexpr (has_feedback) {
         if constexpr (is_multiple) {
            return return_handle(
               tuple{span<T>(p_allocation, allocation_count), prepared_bytes});
         } else {
            return return_handle(
               sized_allocation<T* _Nonnull>{p_allocation, prepared_bytes});
         }
      } else {
         if constexpr (is_multiple) {
            return return_handle(span<T>{p_allocation, allocation_count});
         } else {
            return return_handle(p_allocation);
         }
      }
   }
}

template <typename Derived>
// When `inline_size != 0`, try an inline storage buffer before the allocator.
//
// `is_fail_safe` maps allocation failure to `maybe` results.
//
// `is_aligned` selects aligned allocation entry points.
//
// `is_multiple` allocates arrays (span handles) instead of one `T`.
// allocates a `span<T>` instead of a scalar `T`.
//
// `is_zeroed` zero-initializes the allocation instead of constructing.
//
// `has_feedback` returns sized byte counts to report over-allocation.
template <typename T, bool is_fail_safe, bool is_aligned, bool is_multiple,
          bool is_zeroed, bool has_feedback, idx inline_size, typename... Args>
   requires((!is_multiple || (sizeof...(Args) <= 0))
            // Multi-allocations must be default-constructed. Zeroed-out
            // allocations must not take constructor arguments and must use a
            // trivial object type.
            && (!is_zeroed || (sizeof...(Args) == 0))
            && (!is_zeroed || is_implicit_lifetime<T>))
constexpr auto
allocator_interface<Derived>::meta_alloc(ualign allocation_alignment,
                                         idx allocation_count,
                                         Args&&... arguments) {
   constexpr bool is_inline = (inline_size != 0);

   if constexpr (has_max_allocation_bytes) {
      static_assert(sizeof(T) <= Derived::max_allocation_bytes,
                    "This allocation is too large for this allocator!");
      assert((allocation_count * sizeof(T)) <= Derived::max_allocation_bytes,
             "This allocation is too larger for this allocator!");
   }

   idx const allocation_bytes = allocation_count * sizeof(T);

   if constexpr (is_inline) {
      if (allocation_bytes <= inline_size) {
         return this->template meta_alloc_inline_stack_allocate<
            T, is_fail_safe, is_aligned, is_multiple, is_zeroed, has_feedback,
            inline_size>(allocation_bytes, allocation_count,
                         $fwd(arguments)...);
      }
   }

   using alias_types = meta_alloc_alias_types<T, is_fail_safe, is_multiple,
                                              has_feedback, inline_size>;
   using return_handle = alias_types::return_handle;

   if consteval {
      bool const plain_new_ok = (allocation_alignment == uword(1))
                                || (allocation_alignment == uword(alignof(T)));
      T* _Nonnull p_allocation;
      if constexpr (is_multiple) {
         p_allocation = plain_new_ok
                           ? new T[allocation_count]
                           : new (std::align_val_t(allocation_alignment.raw))
                                T[allocation_count];
      } else if constexpr (is_zeroed) {
         p_allocation = plain_new_ok
                           ? new T
                           : new (std::align_val_t(allocation_alignment.raw)) T;
      } else {
         p_allocation = plain_new_ok
                           ? new T(arguments...)
                           : new (std::align_val_t(allocation_alignment.raw))
                                T(arguments...);
      }
      idx const prepared_bytes = allocation_bytes;

      if constexpr (is_inline) {
         using handle_type = alias_types::handle_type;
         using underlying_handle = alias_types::underlying_handle;
         underlying_handle const raw_handle =
            this->self().template make_handle<T>(p_allocation);
         handle_type handle(move(raw_handle));
         handle.set_inlined(false);
         if constexpr (is_multiple) {
            handle.set_count(allocation_count);
         }
         if constexpr (has_feedback) {
            return return_handle(
               tuple<handle_type, idx>{move(handle), prepared_bytes});
         } else {
            return return_handle(move(handle));
         }
      } else {
         if constexpr (has_feedback) {
            if constexpr (is_multiple) {
               return return_handle(tuple{
                  span<T>(p_allocation, allocation_count),
                  prepared_bytes,
               });
            } else {
               return return_handle(
                  sized_allocation<T* _Nonnull>{p_allocation, prepared_bytes});
            }
         } else {
            if constexpr (is_multiple) {
               return return_handle(span<T>(p_allocation, allocation_count));
            } else {
               return return_handle(p_allocation);
            }
         }
      }
   } else {
      meta_alloc_raw_maybe_allocation<has_feedback> maybe_memory;

      if constexpr (is_zeroed && has_zeroed_hook<is_aligned, has_feedback>) {
         // The `calloc` family looks for optional zeroed allocation
         // optimization hooks.
         if constexpr (is_aligned) {
            if constexpr (has_feedback) {
               maybe_memory = this->self().aligned_allocate_zeroed_feedback(
                  allocation_alignment, allocation_bytes);
            } else {
               maybe_memory = this->self().aligned_allocate_zeroed(
                  allocation_alignment, allocation_bytes);
            }
         } else {
            if constexpr (has_feedback) {
               maybe_memory =
                  this->self().allocate_zeroed_feedback(allocation_bytes);
            } else {
               maybe_memory = this->self().allocate_zeroed(allocation_bytes);
            }
         }
      } else {
         // If the requested alignment is <= this allocator's minimum alignment,
         // we can skip explicit alignment hooks.
         bool const over_aligned =
            is_aligned && (allocation_alignment > Derived::min_alignment);
         if constexpr (has_feedback) {
            maybe_memory = over_aligned
                              ? this->meta_alloc_aligned_feedback(
                                   allocation_alignment, allocation_bytes)
                              : this->meta_alloc_unaligned_feedback(
                                   allocation_alignment, allocation_bytes);
         } else {
            maybe_memory = over_aligned
                              ? this->meta_alloc_aligned(allocation_alignment,
                                                         allocation_bytes)
                              : this->meta_alloc_unaligned(allocation_bytes);
         }
      }

      if constexpr (is_fail_safe) {
         if (!maybe_memory.has_value()) {
            return return_handle(nullopt);
         }
      }

      return this->template meta_alloc_construct<T, is_fail_safe, is_aligned,
                                                 is_multiple, is_zeroed,
                                                 has_feedback, inline_size>(
         move(maybe_memory), allocation_bytes, allocation_count,
         $fwd(arguments)...);
   }
}

}  // namespace cat
