// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

// The POSIX threads ABI, laid out for GlibC on x86-64. libCat defines none of
// these symbols, so using any of them requires linking a pthread library such
// as GlibC's `libc.so`.
// This is only available with `CAT_USE_PTHREAD`.

#ifdef CAT_USE_PTHREAD

#include <cat/detail/machine_pause.hpp>

#include <cat/allocator_parameters>
#include <cat/array>
#include <cat/linux>
#include <cat/runtime>
#include <cat/tuple>

namespace nix {

using pthread_t = cat::uword;

// This is opaque to libCat. Only its size and alignment matter.
struct pthread_attr_t {
   alignas(8) cat::array<cat::byte, 56> storage;
};

static_assert(sizeof(pthread_attr_t) == 56);

// These return 0 on success, or otherwise a positive `errno` value.
extern "C" {
auto
pthread_attr_init(pthread_attr_t* _Nonnull p_attributes) noexcept -> int;

auto
pthread_attr_destroy(pthread_attr_t* _Nonnull p_attributes) noexcept -> int;

// The stack spans `[p_stack, p_stack + stack_size)`. GlibC carves its thread
// control block and static `thread_local` storage from the top of it, which
// `_dl_get_tls_static_info` measures.
auto
pthread_attr_setstack(
   pthread_attr_t* _Nonnull p_attributes, void* _Nonnull p_stack,
   cat::uword stack_size
) noexcept -> int;

auto
pthread_create(
   pthread_t* _Nonnull p_thread, pthread_attr_t const* _Nullable p_attributes,
   void* _Nullable (*_Nonnull p_start)(void* _Nullable),
   void* _Nullable p_argument
) noexcept -> int;

auto
pthread_join(pthread_t thread, void* _Nullable* _Nullable pp_result) noexcept
   -> int;

// GlibC's dynamic loader exports this as `GLIBC_PRIVATE`. It measures the
// static `thread_local` block, including the thread control block, that every
// thread carries. It is weak because other C libraries, such as musl, lack it.
[[gnu::weak]]
void
_dl_get_tls_static_info(  // NOLINT(bugprone-reserved-identifier)
   cat::idx* _Nonnull p_size, cat::idx* _Nonnull p_alignment
) noexcept;
}

// Convert a pthread function's positive `errno` result.
[[nodiscard]]
constexpr auto
pthread_error(int error) noexcept -> linux_error {
   return static_cast<linux_error>(-error);
}

namespace detail {

// What a `pthread_spawn` child starts from. It lives above the child's stack.
template <typename Arguments>
struct pthread_start {
   cat::atomic<cat::uint4> m_thread_id;
   Arguments m_arguments;
};

template <typename Arguments>
auto
pthread_entry(void* _Nullable p_storage) -> void* _Nullable {
   auto* const p_start = static_cast<pthread_start<Arguments>*>(p_storage);
   p_start->m_thread_id.store(sys_gettid().value, cat::memory_order::release);
   auto&& [callback, ... arguments] = p_start->m_arguments;
   $fwd(callback)($fwd(arguments)...);
   p_start->m_arguments.~Arguments();
#if !defined(CAT_THREAD_LOCAL_SIZE) || (CAT_THREAD_LOCAL_SIZE) != 0
   cat::__cxa_thread_finalize();
#endif
   return nullptr;
}

struct pthread_spawned {
   pthread_t handle;
   process_id id;
   // The child's stack is at the bottom of this allocation.
   cat::span<cat::byte> memory;
};

// Spawn `callback(arguments...)` on a POSIX thread whose stack is allocated
// from `allocator`, and fill `spawned`. This returns after the child has
// published its id.
template <typename... Args, cat::is_invocable<Args...> Callback>
auto
pthread_spawn(
   cat::is_allocator auto& allocator, cat::idx stack_size,
   pthread_spawned& spawned, Callback&& callback, Args&&... arguments
) -> scaredy_nix<void> {
   using arguments_type =
      typeof(cat::tuple{$fwd(callback), $fwd(arguments)...});
   using start_type = pthread_start<arguments_type>;

   // GlibC carves its static `thread_local` block from the top of the stack,
   // so add it on top of `stack_size`, as a raw `clone` does with libCat's.
   // musl only does that when the block is small, and otherwise maps it
   // separately, so it needs nothing extra.
   cat::idx tls_bytes = 0u;
   cat::idx tls_alignment = 0u;
   if (_dl_get_tls_static_info != nullptr) {
      _dl_get_tls_static_info(&tls_bytes, &tls_alignment);
   }
   cat::idx const pthread_stack_size = stack_size + tls_bytes + tls_alignment;

   constexpr cat::idx start_padding =
      alignof(start_type) - 1u + sizeof(start_type);
   cat::span<cat::byte> memory = $prop_as(
      allocator.template align_alloc_multi<cat::byte>(
         cat::max(16u, alignof(start_type)), pthread_stack_size + start_padding
      ),
      linux_error::nomem
   );

   cat::byte* const p_stack_bottom = memory.data();
   auto* const p_start = new (
      cat::align_up(p_stack_bottom + pthread_stack_size, alignof(start_type))
   ) start_type{
      {},
      arguments_type{$fwd(callback), $fwd(arguments)...}
   };

   pthread_attr_t attributes;
   int error = pthread_attr_init(&attributes);
   pthread_t handle = 0u;
   if (error == 0) {
      error =
         pthread_attr_setstack(&attributes, p_stack_bottom, pthread_stack_size);
      if (error == 0) {
         error = pthread_create(
            &handle, &attributes, &pthread_entry<arguments_type>, p_start
         );
      }
      auto _ = pthread_attr_destroy(&attributes);
   }

   if (error != 0) {
      p_start->~start_type();
      allocator.free_multi(memory);
      return pthread_error(error);
   }

   // `clone_flags::parent_set_tid` publishes a raw `clone` child's id before
   // `clone` returns, so wait to do the same.
   cat::uint1 count = 1u;
   for (;;) {
      cat::uint4 const id =
         p_start->m_thread_id.load(cat::memory_order::acquire);
      if (id != 0u) {
         spawned = {
            .handle = handle,
            .id = id,
            .memory = memory,
         };
         return cat::monostate;
      }
      if (count <= 16u) {
         cat::machine_pause(count);
         count = count * 2u;
      } else {
         auto _ = sys_sched_yield();
      }
   }
}

}  // namespace detail

}  // namespace nix

#endif
