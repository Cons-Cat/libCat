// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/allocator_resizeable>
#include <cat/sanitizer>

namespace cat {

template <
   is_allocator Inner, is_allocator Backing, idx node_bytes_param,
   idx chunk_bytes_param>
allocator_resizeable<Inner, Backing, node_bytes_param, chunk_bytes_param>::
   ~allocator_resizeable() noexcept(!__has_feature(cxx_exceptions)) {
   this->reset();
}

template <
   is_allocator Inner, is_allocator Backing, idx node_bytes_param,
   idx chunk_bytes_param>
void
allocator_resizeable<
   Inner, Backing, node_bytes_param, chunk_bytes_param>::reset() {
   // The free list lives inside the chunks we are about to free, so
   // dropping the head is enough. No per-node bookkeeping needed.
   m_p_free_head = nullptr;
   chunk_header* p_walk = m_p_chunks;
   while (p_walk != nullptr) {
      chunk_header* const p_next = p_walk->p_next;
      idx const chunk_bytes_value = p_walk->chunk_bytes;
      p_walk->~chunk_header();
      allocator_dyn_friend::deallocate(*m_p_backing, p_walk, chunk_bytes_value);
      p_walk = p_next;
   }
   m_p_chunks = nullptr;
}

template <
   is_allocator Inner, is_allocator Backing, idx node_bytes_param,
   idx chunk_bytes_param>
constexpr auto
allocator_resizeable<Inner, Backing, node_bytes_param, chunk_bytes_param>::
   acquire_chunk(idx chunk_bytes_request) noexcept(false)
      -> chunk_header* _Nullable {
   // Each chunk reserves a `chunk_header` prologue at the start of the
   // backing allocation and gives the rest of the slab to `Inner`.
   idx const total_bytes = chunk_bytes_request;
   void* p_backing = nullptr;
   if constexpr (
      requires(Backing& backing, ualign alignment, idx allocation_bytes) {
         allocator_dyn_friend::aligned_allocate(
            backing, alignment, allocation_bytes
         );
      }
   ) {
      p_backing = allocator_dyn_friend::aligned_allocate(
         *m_p_backing, alignof(chunk_header), total_bytes
      );
   } else {
      p_backing = allocator_dyn_friend::allocate(*m_p_backing, total_bytes);
   }
   if (p_backing == nullptr) {
      return nullptr;
   }

   auto* p_raw = static_cast<unsigned char*>(p_backing);
   auto* p_inner_storage = p_raw + sizeof(chunk_header);
   idx const inner_bytes =
      narrow_cast<idx>(total_bytes - sizeof(chunk_header)).assert();

   // Drop any ASan shadow state the kernel handed back stale. A virtual
   // address returned by `mmap` may carry poison set by a prior occupant
   // (e.g. `allocator_linear::reset` poisoning its arena) because `munmap`
   // does not clear ASan shadow.
   unpoison_memory_region(p_raw, total_bytes);

   // `chunk_header` contains `Inner inner`, so `alignof(chunk_header) >=
   // alignof(Inner)`. That makes `p_raw + sizeof(chunk_header)` already
   // aligned for whatever `Inner`'s storage assumes (e.g. `node_union` in
   // `allocator_pool`).
   chunk_header* const p_chunk = new (p_raw) chunk_header{
      nullptr,
      total_bytes,
      Inner{uintptr<void>(p_inner_storage), inner_bytes},
   };
   return p_chunk;
}

template <
   is_allocator Inner, is_allocator Backing, idx node_bytes_param,
   idx chunk_bytes_param>
[[nodiscard]]
auto
allocator_resizeable<Inner, Backing, node_bytes_param, chunk_bytes_param>::
   find_chunk(span<byte> allocation) const noexcept -> chunk_header* _Nullable {
   uintptr<void> const allocation_begin = allocation.data();
   uintptr<void> const allocation_end = allocation_begin + allocation.size();

   for (chunk_header* p_walk = m_p_chunks; p_walk != nullptr;
        p_walk = p_walk->p_next) {
      uintptr<void> const chunk_begin =
         reinterpret_cast<byte*>(p_walk) + sizeof(chunk_header);
      uintptr<void> const chunk_end =
         reinterpret_cast<byte*>(p_walk) + p_walk->chunk_bytes;
      if (allocation_begin >= chunk_begin && allocation_end <= chunk_end) {
         return p_walk;
      }
   }

   return nullptr;
}

template <
   is_allocator Inner, is_allocator Backing, idx node_bytes_param,
   idx chunk_bytes_param>
[[nodiscard]]
auto
allocator_resizeable<Inner, Backing, node_bytes_param, chunk_bytes_param>::grow(
   span<byte> allocation, idx new_bytes
) noexcept -> maybe<void>
   requires(detail::inner_has_grow<Inner>)
{
   chunk_header* const p_chunk = this->find_chunk(allocation);
   if (p_chunk == nullptr) {
      return nullopt;
   }
   return p_chunk->inner.grow(allocation, new_bytes);
}

template <
   is_allocator Inner, is_allocator Backing, idx node_bytes_param,
   idx chunk_bytes_param>
[[nodiscard]]
auto
allocator_resizeable<Inner, Backing, node_bytes_param, chunk_bytes_param>::
   grow_feedback(span<byte> allocation, idx new_bytes) noexcept
   -> maybe_non_zero<idx>
   requires(
      detail::inner_has_grow<Inner> || detail::inner_has_grow_feedback<Inner>
   )
{
   chunk_header* const p_chunk = this->find_chunk(allocation);
   if (p_chunk == nullptr) {
      return nullopt;
   }
   if constexpr (detail::inner_has_grow_feedback<Inner>) {
      return p_chunk->inner.grow_feedback(allocation, new_bytes);
   } else {
      if (p_chunk->inner.grow(allocation, new_bytes).has_value()) {
         return new_bytes;
      }
      return nullopt;
   }
}

template <
   is_allocator Inner, is_allocator Backing, idx node_bytes_param,
   idx chunk_bytes_param>
[[gnu::malloc, clang::ownership_returns(libcat)]]
auto
allocator_resizeable<Inner, Backing, node_bytes_param, chunk_bytes_param>::
   allocate(idx /*allocation_bytes*/) noexcept -> void* _Nullable {
   // Reuse a freed slot.
   if (m_p_free_head != nullptr) {
      free_node* const p_slot = m_p_free_head;
      m_p_free_head = p_slot->p_next;
      return static_cast<void*>(p_slot);
   }

   if (m_p_chunks != nullptr) {
      auto* p_attempt = m_p_chunks->inner.allocate(node_bytes);
      if (p_attempt != nullptr) {
         unpoison_memory_region(p_attempt, node_bytes);
         return p_attempt;
      }
   }

   // When out of room, append more memory from `Backing`.
   if (this->resize(node_bytes).is_empty()) {
      return nullptr;
   }
   auto* p_attempt = m_p_chunks->inner.allocate(node_bytes);
   if (p_attempt != nullptr) {
      unpoison_memory_region(p_attempt, node_bytes);
      return p_attempt;
   }
   return nullptr;
}

template <
   is_allocator Inner, is_allocator Backing, idx node_bytes_param,
   idx chunk_bytes_param>
[[clang::ownership_takes(libcat, 2)]]
void
allocator_resizeable<Inner, Backing, node_bytes_param, chunk_bytes_param>::
   deallocate(void const* _Nonnull p_storage, idx /*allocation_bytes*/) {
   auto* p_slot = static_cast<free_node*>(const_cast<void*>(p_storage));
   p_slot->p_next = m_p_free_head;
   m_p_free_head = p_slot;
}

template <
   is_allocator Inner, is_allocator Backing, idx node_bytes_param,
   idx chunk_bytes_param>
auto
allocator_resizeable<Inner, Backing, node_bytes_param, chunk_bytes_param>::
   resize(idx minimum_bytes) noexcept(!__has_feature(cxx_exceptions))
      -> maybe_non_zero<idx> {
   idx const request_bytes = idx(
      minimum_bytes + sizeof(chunk_header) > chunk_bytes
         ? minimum_bytes + sizeof(chunk_header)
         : chunk_bytes
   );
   chunk_header* const p_new_chunk = this->acquire_chunk(request_bytes);
   if (p_new_chunk == nullptr) {
      return nullopt;
   }
   p_new_chunk->p_next = m_p_chunks;
   m_p_chunks = p_new_chunk;
   return request_bytes;
}

template <
   is_allocator Inner, is_allocator Backing, idx node_bytes_param,
   idx chunk_bytes_param>
[[nodiscard]]
auto
allocator_resizeable<Inner, Backing, node_bytes_param, chunk_bytes_param>::
   bytes_used() const noexcept -> idx {
   idx total = 0u;
   for (chunk_header const* p_walk = m_p_chunks; p_walk != nullptr;
        p_walk = p_walk->p_next) {
      total += p_walk->inner.bytes_used();
   }
   idx free_slots = 0u;
   for (free_node const* p_walk = m_p_free_head; p_walk != nullptr;
        p_walk = p_walk->p_next) {
      ++free_slots;
   }
   idx const free_bytes = idx(free_slots * node_bytes);
   return total > free_bytes ? idx(total - free_bytes) : 0u;
}

template <
   is_allocator Inner, is_allocator Backing, idx node_bytes_param,
   idx chunk_bytes_param>
[[nodiscard]]
auto
allocator_resizeable<Inner, Backing, node_bytes_param, chunk_bytes_param>::
   bytes_capacity() const noexcept -> idx {
   idx total = 0u;
   for (chunk_header const* p_walk = m_p_chunks; p_walk != nullptr;
        p_walk = p_walk->p_next) {
      total += p_walk->inner.bytes_capacity();
   }
   return total;
}

}  // namespace cat
