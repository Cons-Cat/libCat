// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/resizable_allocator>
#include <cat/sanitizer>

namespace cat {

template <is_allocator Inner, is_allocator Backing, idx node_bytes_param,
          idx chunk_bytes_param>
resizable_allocator<Inner, Backing, node_bytes_param,
                    chunk_bytes_param>::~resizable_allocator() {
   this->reset();
}

template <is_allocator Inner, is_allocator Backing, idx node_bytes_param,
          idx chunk_bytes_param>
void
resizable_allocator<Inner, Backing, node_bytes_param,
                    chunk_bytes_param>::reset() {
   // The free list lives inside the chunks we are about to free, so
   // dropping the head is enough. No per-node bookkeeping needed.
   m_p_free_head = nullptr;
   chunk_header* p_walk = m_p_chunks;
   while (p_walk != nullptr) {
      chunk_header* const p_next = p_walk->p_next;
      idx const chunk_bytes_value = p_walk->chunk_bytes;
      p_walk->~chunk_header();
      dyn_allocator_friend::deallocate(*m_p_backing, p_walk, chunk_bytes_value);
      p_walk = p_next;
   }
   m_p_chunks = nullptr;
}

template <is_allocator Inner, is_allocator Backing, idx node_bytes_param,
          idx chunk_bytes_param>
constexpr auto
resizable_allocator<Inner, Backing, node_bytes_param,
                    chunk_bytes_param>::acquire_chunk(idx chunk_bytes_request)
   -> chunk_header* _Nullable {
   // Each chunk reserves a `chunk_header` prologue at the start of the
   // backing allocation and gives the rest of the slab to `Inner`.
   idx const total_bytes = chunk_bytes_request;
   void* p_backing = nullptr;
   if constexpr (requires(Backing& backing, ualign alignment,
                          idx allocation_bytes) {
                    dyn_allocator_friend::aligned_allocate(backing, alignment,
                                                           allocation_bytes);
                 }) {
      p_backing = dyn_allocator_friend::aligned_allocate(
         *m_p_backing, alignof(chunk_header), total_bytes);
   } else {
      p_backing = dyn_allocator_friend::allocate(*m_p_backing, total_bytes);
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
   // (e.g. `linear_allocator::reset` poisoning its arena) because `munmap`
   // does not clear ASan shadow.
   unpoison_memory_region(p_raw, total_bytes);

   // `chunk_header` contains `Inner inner`, so `alignof(chunk_header) >=
   // alignof(Inner)`. That makes `p_raw + sizeof(chunk_header)` already
   // aligned for whatever `Inner`'s storage assumes (e.g. `node_union` in
   // `pool_allocator`).
   chunk_header* const p_chunk = ::new (static_cast<void*>(p_raw)) chunk_header{
      nullptr, total_bytes, Inner{uintptr<void>(p_inner_storage), inner_bytes}
   };
   return p_chunk;
}

template <is_allocator Inner, is_allocator Backing, idx node_bytes_param,
          idx chunk_bytes_param>
[[gnu::malloc, clang::ownership_returns(libcat)]]
auto
resizable_allocator<Inner, Backing, node_bytes_param,
                    chunk_bytes_param>::allocate(idx /*allocation_bytes*/)
   -> void* _Nullable {
   // The wrapper bypasses `meta_alloc`'s unpoison path, so inner allocators
   // that pre-poison their arena (e.g. `linear_allocator`) would leave the
   // returned slot user-poisoned. Clear the shadow for the slot we hand out.

   // 1. Reuse a freed slot. Its address is stable across the wrapper's life.
   if (m_p_free_head != nullptr) {
      free_node* const p_slot = m_p_free_head;
      m_p_free_head = p_slot->p_next;
      return static_cast<void*>(p_slot);
   }

   // 2. Carve from the current chunk.
   if (m_p_chunks != nullptr) {
      auto* p_attempt = m_p_chunks->inner.allocate(node_bytes);
      if (p_attempt != nullptr) {
         unpoison_memory_region(p_attempt, node_bytes);
         return p_attempt;
      }
   }

   // 3. Out of room: pull a fresh chunk from `Backing` and carve from it.
   // Existing allocations stay where they are -- growth only adds a chunk.
   if (!this->resize(node_bytes).has_value()) {
      return nullptr;
   }
   auto* p_attempt = m_p_chunks->inner.allocate(node_bytes);
   if (p_attempt != nullptr) {
      unpoison_memory_region(p_attempt, node_bytes);
      return p_attempt;
   }
   return nullptr;
}

template <is_allocator Inner, is_allocator Backing, idx node_bytes_param,
          idx chunk_bytes_param>
[[clang::ownership_takes(libcat, 2)]]
void
resizable_allocator<Inner, Backing, node_bytes_param, chunk_bytes_param>::
   deallocate(void const* _Nonnull p_storage, idx /*allocation_bytes*/) {
   auto* p_slot = static_cast<free_node*>(const_cast<void*>(p_storage));
   p_slot->p_next = m_p_free_head;
   m_p_free_head = p_slot;
}

template <is_allocator Inner, is_allocator Backing, idx node_bytes_param,
          idx chunk_bytes_param>
auto
resizable_allocator<Inner, Backing, node_bytes_param,
                    chunk_bytes_param>::resize(idx minimum_bytes)
   -> maybe_non_zero<idx> {
   idx const request_bytes =
      static_cast<idx>(minimum_bytes + sizeof(chunk_header) > chunk_bytes
                          ? minimum_bytes + sizeof(chunk_header)
                          : chunk_bytes);
   chunk_header* const p_new_chunk = this->acquire_chunk(request_bytes);
   if (p_new_chunk == nullptr) {
      return nullopt;
   }
   p_new_chunk->p_next = m_p_chunks;
   m_p_chunks = p_new_chunk;
   return request_bytes;
}

template <is_allocator Inner, is_allocator Backing, idx node_bytes_param,
          idx chunk_bytes_param>
[[nodiscard]]
auto
resizable_allocator<Inner, Backing, node_bytes_param,
                    chunk_bytes_param>::bytes_used() const -> idx {
   idx total = idx(0u);
   for (chunk_header const* p_walk = m_p_chunks; p_walk != nullptr;
        p_walk = p_walk->p_next) {
      total += p_walk->inner.bytes_used();
   }
   idx free_slots = idx(0u);
   for (free_node const* p_walk = m_p_free_head; p_walk != nullptr;
        p_walk = p_walk->p_next) {
      ++free_slots;
   }
   idx const free_bytes = static_cast<idx>(free_slots * node_bytes);
   return total > free_bytes ? static_cast<idx>(total - free_bytes) : idx(0u);
}

template <is_allocator Inner, is_allocator Backing, idx node_bytes_param,
          idx chunk_bytes_param>
[[nodiscard]]
auto
resizable_allocator<Inner, Backing, node_bytes_param,
                    chunk_bytes_param>::bytes_capacity() const -> idx {
   idx total = idx(0u);
   for (chunk_header const* p_walk = m_p_chunks; p_walk != nullptr;
        p_walk = p_walk->p_next) {
      total += p_walk->inner.bytes_capacity();
   }
   return total;
}

}  // namespace cat
