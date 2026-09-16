// -*- mode: c++ -*-
// vim: set ft=cpp:

#include <cat/array>
#include <cat/atomic>
#include <cat/bit>
#include <cat/bitset>
#include <cat/bitset_shared>
#include <cat/limits>
#include <cat/linux>
#include <cat/memory>
#include <cat/musl_allocator>

namespace cat::detail {

namespace {

inline constexpr uword musl_unit = 16uz;
inline constexpr uword musl_in_band = 4uz;
inline constexpr uword musl_page_bytes = 4uz * 1'024uz;
inline constexpr uword musl_mmap_threshold = 131'052uz;
inline constexpr uword musl_maximum_alignment = 1uz << 35u;

template <bool is_shared>
using musl_mask_storage =
   conditional<is_shared, bitset_shared<32u>, bitset<32u>>;

template <bool is_shared>
[[gnu::always_inline]]
auto
musl_load_mask(musl_mask_storage<is_shared> const& mask) -> uint4 {
   if constexpr (is_shared) {
      return mask.load(0u, memory_order::acquire);
   } else {
      return bit_cast<uint4>(mask);
   }
}

template <bool is_shared>
[[gnu::always_inline]]
void
musl_store_mask(musl_mask_storage<is_shared>& mask, uint4 desired) {
   if constexpr (is_shared) {
      mask.store(0u, desired, memory_order::release);
   } else {
      mask = bit_cast<bitset<32u>>(desired);
   }
}

template <bool is_shared>
using musl_counter_storage = conditional<is_shared, atomic<uword>, uword>;

template <bool is_shared>
void
musl_add_counter(musl_counter_storage<is_shared>& counter, uword amount) {
   if constexpr (is_shared) {
      uword const old = counter.relaxed().fetch_add(amount);
      verify(old <= limits<uword>::max() - amount);
   } else {
      verify(counter <= limits<uword>::max() - amount);
      counter += amount;
   }
}

template <bool is_shared>
void
musl_subtract_counter(musl_counter_storage<is_shared>& counter, uword amount) {
   if constexpr (is_shared) {
      uword const old = counter.relaxed().fetch_sub(amount);
      verify(old >= amount);
   } else {
      verify(counter >= amount);
      counter -= amount;
   }
}

template <bool is_shared>
struct musl_alloc_lock;

template <>
struct musl_alloc_lock<false> {
   [[gnu::always_inline]]
   void
   lock() {
   }

   [[gnu::always_inline]]
   void
   unlock() {
   }
};

template <>
struct musl_alloc_lock<true> {
   nix::futex word{};

   void
   lock() {
      uint4 expected = 0u;
      if (
         word.m_value.compare_exchange_weak(
            expected, 1u, memory_order::acquire, memory_order::relaxed
         )
      ) {
         return;
      }

      for (;;) {
         uint4 const previous = word.m_value.acq_rel().exchange(2u);
         if (previous == 0u) {
            return;
         }
         auto const ignored = word.wait(2u);
         static_cast<void>(ignored);
      }
   }

   void
   unlock() {
      uint4 const previous = word.m_value.release().exchange(0u);
      verify(previous != 0u);
      if (previous == 2u) {
         auto const ignored = word.wake();
         static_cast<void>(ignored);
      }
   }
};

template <bool is_shared>
struct meta;

template <bool is_shared>
struct group {
   meta<is_shared>* _Nullable p_meta;
   unsigned char active_idx : 5;
   array<byte, musl_unit.raw - sizeof(meta<is_shared>*) - 1u> padding;
   byte storage[];
};

template <bool is_shared>
struct meta {
   meta* _Nullable p_previous;
   meta* _Nullable p_next;
   group<is_shared>* _Nullable p_memory;
   musl_mask_storage<is_shared> available_mask;
   musl_mask_storage<is_shared> freed_mask;
   uword::raw_type last_idx   : 5;
   uword::raw_type freeable   : 1;
   uword::raw_type size_class : 6;
   uword::raw_type map_length : (8u * sizeof(uword::raw_type)) - 12u;
};

static_assert(sizeof(group<false>) == musl_unit);
static_assert(sizeof(group<true>) == musl_unit);

template <bool is_shared>
struct meta_area {
   uint8 check;
   meta_area* _Nullable p_next;
   meta_area* _Nullable p_mapping_next;
   byte* _Nullable p_mapping_base;
   uword mapping_bytes;
   uint4 nslots;
   meta<is_shared> slots[];
};

inline constexpr array<uint2, 48u> size_classes = {
   1,     2,     3,     4,     5,     6,     7,     8,     9,     10,
   12,    15,    18,    20,    25,    31,    36,    42,    50,    63,
   72,    84,    102,   127,   146,   170,   204,   255,   292,   340,
   409,   511,   584,   682,   818,   1'023, 1'169, 1'364, 1'637, 2'047,
   2'340, 2'730, 3'276, 4'095, 4'680, 5'460, 6'552, 8'191,
};

inline constexpr array<array<unsigned char, 3u>, 9u> small_count_table = {
   array<unsigned char, 3u>{30, 30, 30},
     array<unsigned char, 3u>{31, 15, 15},
   array<unsigned char, 3u>{20, 10, 10},
     array<unsigned char, 3u>{31, 15, 7 },
   array<unsigned char, 3u>{25, 12, 6 },
     array<unsigned char, 3u>{21, 10, 5 },
   array<unsigned char, 3u>{18, 8,  4 },
     array<unsigned char, 3u>{31, 15, 7 },
   array<unsigned char, 3u>{28, 14, 6 },
};

inline constexpr array<unsigned char, 4u> medium_count_table = {28, 24, 20, 32};

template <bool is_shared>
struct musl_map_info {
   byte* _Nullable p_base = nullptr;
   uword bytes = 0u;
};

}  // namespace

template <bool is_shared>
struct musl_context {
   musl_alloc_lock<is_shared> mutex;
   uint8 secret;
   uint4 mmap_counter;
   meta<is_shared>* _Nullable p_free_meta_head;
   meta<is_shared>* _Nullable p_available_meta;
   uword available_meta_count;
   uword available_meta_area_count;
   uword meta_alloc_shift;
   meta_area<is_shared>* _Nullable p_meta_area_head;
   meta_area<is_shared>* _Nullable p_meta_area_tail;
   meta_area<is_shared>* _Nullable p_meta_mapping_head;
   byte* _Nullable p_available_meta_areas;
   byte* _Nullable p_current_meta_mapping;
   uword current_meta_mapping_bytes;
   array<meta<is_shared>* _Nullable, 48u> active;
   array<uword, 48u> usage_by_class;
   array<unsigned char, 32u> unmap_sequence;
   array<unsigned char, 32u> bounces;
   unsigned char sequence;
   musl_counter_storage<is_shared> live_bytes;
   musl_counter_storage<is_shared> mapped_bytes;

   void
   free_meta(meta<is_shared>* p_metadata);

   auto
   allocate_meta() -> meta<is_shared>* _Nullable;

   auto
   get_meta(byte const* p_allocation) -> meta<is_shared>*;

   void
   step_sequence();

   void
   record_sequence(uint4 size_class);

   void
   account_bounce(uint4 size_class);

   void
   decay_bounces(uint4 size_class);

   auto
   is_bouncing(uint4 size_class) const -> bool;

   auto
   try_available(meta<is_shared>* _Nullable* p_metadata) -> uint4;

   auto
   allocate_slot(uint4 size_class, uword request) -> int4;

   auto
   allocate_group(uint4 size_class, uword request)
      -> meta<is_shared>* _Nullable;

   auto
   allocate_memory_unaccounted(
      uword bytes, uword* _Nullable p_feedback_bytes = nullptr
   ) -> byte* _Nullable;

   [[gnu::always_inline]]
   auto
   allocate_memory(uword bytes, uword* _Nullable p_feedback_bytes = nullptr)
      -> byte* _Nullable;

   auto
   okay_to_free(meta<is_shared>* p_metadata) -> bool;

   auto
   nontrivial_free(meta<is_shared>* p_metadata, uint4 index)
      -> musl_map_info<is_shared>;

   auto
   free_group(meta<is_shared>* p_metadata) -> musl_map_info<is_shared>;

   void
   finish_unmap(musl_map_info<is_shared> mapping);

   auto
   deallocate_locked(
      byte* p_allocation, meta<is_shared>* p_metadata, uint4 index,
      uword old_bytes
   ) -> musl_map_info<is_shared>;

   auto
   deallocate_fast(
      byte* p_allocation, meta<is_shared>* p_metadata, uint4 index,
      uword old_bytes
   ) -> bool;

   auto
   prepare_free(byte* p_allocation) -> tuple<meta<is_shared>*, uint4, uword>;

   auto
   is_all_zero(byte* p_allocation) -> bool;

   auto
   aligned_allocate(
      uword alignment, uword bytes, uword* _Nullable p_feedback_bytes = nullptr
   ) -> byte* _Nullable;

   auto
   resize_in_place(
      byte*& p_allocation, uword old_allocation_bytes, uword new_bytes,
      bool may_move, uword* _Nullable p_feedback_bytes = nullptr
   ) -> bool;

   void
   release_mappings();
};

namespace {

auto
map_pages(uword bytes, nix::memory_protection_flags protections)
   -> byte* _Nullable {
   auto const mapped = nix::sys_mmap(
      nullptr, bytes, protections,
      nix::memory_flags::privately | nix::memory_flags::anonymous,
      nix::invalid_file_descriptor, 0u
   );
   if (mapped.is_empty()) {
      return nullptr;
   }
   return mapped.value();
}

void
unmap_pages(byte const* p_memory, uword bytes) {
   nix::sys_munmap(p_memory, bytes).assert();
}

auto
encoded_bytes(byte* p_memory) -> unsigned char* {
   return bit_cast<unsigned char*>(p_memory);
}

auto
encoded_bytes(byte const* p_memory) -> unsigned char const* {
   return bit_cast<unsigned char const*>(p_memory);
}

template <bool is_shared>
auto
make_context(uint8 secret = 0u) -> musl_context<is_shared>* _Nullable {
   byte* const p_mapping =
      map_pages(musl_page_bytes, nix::memory_protection_flags::read_write);
   if (p_mapping == nullptr) {
      return nullptr;
   }

   auto* const p_context = new (p_mapping) musl_context<is_shared>{};
   p_context->secret = secret;
   while (p_context->secret == 0u) {
      uword filled = 0u;
      while (filled != sizeof(p_context->secret)) {
         auto const result = nix::sys_getrandom(
            reinterpret_cast<unsigned char*>(&p_context->secret) + filled,
            sizeof(p_context->secret) - filled
         );
         if (result.is_empty() || result.value() == 0u) {
            unmap_pages(p_mapping, musl_page_bytes);
            return nullptr;
         }
         filled += result.value();
      }
   }
   musl_add_counter<is_shared>(p_context->mapped_bytes, musl_page_bytes);
   return p_context;
}

template <bool is_shared>
void
queue(meta<is_shared>* _Nullable* p_head, meta<is_shared>* p_metadata) {
   verify(p_metadata->p_next == nullptr);
   verify(p_metadata->p_previous == nullptr);
   if (*p_head != nullptr) {
      meta<is_shared>* const p_first = *p_head;
      p_metadata->p_next = p_first;
      p_metadata->p_previous = p_first->p_previous;
      p_metadata->p_next->p_previous = p_metadata;
      p_metadata->p_previous->p_next = p_metadata;
   } else {
      p_metadata->p_previous = p_metadata;
      p_metadata->p_next = p_metadata;
      *p_head = p_metadata;
   }
}

template <bool is_shared>
void
dequeue(meta<is_shared>* _Nullable* p_head, meta<is_shared>* p_metadata) {
   verify(*p_head != nullptr);
   verify(p_metadata->p_next != nullptr);
   if (p_metadata->p_next != p_metadata) {
      p_metadata->p_previous->p_next = p_metadata->p_next;
      p_metadata->p_next->p_previous = p_metadata->p_previous;
      if (*p_head == p_metadata) {
         *p_head = p_metadata->p_next;
      }
   } else {
      *p_head = nullptr;
   }
   p_metadata->p_previous = nullptr;
   p_metadata->p_next = nullptr;
}

template <bool is_shared>
auto
dequeue_head(meta<is_shared>* _Nullable* p_head) -> meta<is_shared>* _Nullable {
   meta<is_shared>* const p_metadata = *p_head;
   if (p_metadata != nullptr) {
      dequeue(p_head, p_metadata);
   }
   return p_metadata;
}

template <bool is_shared>
void
clear_meta(meta<is_shared>* p_metadata) {
   p_metadata->p_previous = nullptr;
   p_metadata->p_next = nullptr;
   p_metadata->p_memory = nullptr;
   musl_store_mask<is_shared>(p_metadata->available_mask, 0u);
   musl_store_mask<is_shared>(p_metadata->freed_mask, 0u);
   p_metadata->last_idx = 0u;
   p_metadata->freeable = 0u;
   p_metadata->size_class = 0u;
   p_metadata->map_length = 0u;
}

}  // namespace

template <bool is_shared>
void
musl_context<is_shared>::free_meta(meta<is_shared>* p_metadata) {
   clear_meta(p_metadata);
   queue(&p_free_meta_head, p_metadata);
}

template <bool is_shared>
auto
musl_context<is_shared>::allocate_meta() -> meta<is_shared>* _Nullable {
   auto& context = *this;
   if (
      meta<is_shared>* const p_reused = dequeue_head(&context.p_free_meta_head)
   ) {
      return p_reused;
   }

   if (context.available_meta_count == 0u) {
      if (context.available_meta_area_count == 0u) {
         verify(context.meta_alloc_shift < 52u);
         uword const mapping_pages = uword{2u} << context.meta_alloc_shift;
         verify(mapping_pages <= limits<uword>::max() / musl_page_bytes);
         uword const mapping_bytes = mapping_pages * musl_page_bytes;
         byte* const p_mapping =
            map_pages(mapping_bytes, nix::memory_protection_flags::none);
         if (p_mapping == nullptr) {
            return nullptr;
         }

         byte* const p_first_area = p_mapping + musl_page_bytes;
         auto const unprotected = nix::sys_mprotect(
            p_first_area, musl_page_bytes,
            nix::memory_protection_flags::read_write
         );
         if (unprotected.is_empty()) {
            unmap_pages(p_mapping, mapping_bytes);
            return nullptr;
         }

         musl_add_counter<is_shared>(context.mapped_bytes, mapping_bytes);
         context.p_available_meta_areas = p_first_area;
         context.available_meta_area_count = mapping_pages - 1u;
         context.p_current_meta_mapping = p_mapping;
         context.current_meta_mapping_bytes = mapping_bytes;
         ++context.meta_alloc_shift;
      }

      byte* const p_area_memory = context.p_available_meta_areas;
      verify(p_area_memory != nullptr);
      auto* const p_area = bit_cast<meta_area<is_shared>*>(p_area_memory);

      auto const unprotected = nix::sys_mprotect(
         p_area_memory, musl_page_bytes,
         nix::memory_protection_flags::read_write
      );
      if (unprotected.is_empty()) {
         return nullptr;
      }

      if (p_area_memory == context.p_current_meta_mapping + musl_page_bytes) {
         p_area->p_mapping_base = context.p_current_meta_mapping;
         p_area->mapping_bytes = context.current_meta_mapping_bytes;
         p_area->p_mapping_next = context.p_meta_mapping_head;
         context.p_meta_mapping_head = p_area;
      }

      --context.available_meta_area_count;
      context.p_available_meta_areas += musl_page_bytes;

      if (context.p_meta_area_tail != nullptr) {
         context.p_meta_area_tail->p_next = p_area;
      } else {
         context.p_meta_area_head = p_area;
      }
      context.p_meta_area_tail = p_area;
      p_area->check = context.secret;
      p_area->nslots = static_cast<uint4>(
         (musl_page_bytes - sizeof(meta_area<is_shared>))
         / sizeof(meta<is_shared>)
      );
      context.available_meta_count = p_area->nslots;
      context.p_available_meta = p_area->slots;
   }

   --context.available_meta_count;
   meta<is_shared>* const p_result = context.p_available_meta++;
   new (p_result) meta<is_shared>{};
   return p_result;
}

namespace {

template <bool is_shared>
auto
all_slots_mask(meta<is_shared> const& metadata) -> uint4 {
   return (uint4{2u} << metadata.last_idx) - 1u;
}

template <bool is_shared>
auto
activate_group(meta<is_shared>* p_metadata) -> uint4 {
   verify(musl_load_mask<is_shared>(p_metadata->available_mask) == 0u);
   uint4 const active =
      (uint4{2u} << uint4{p_metadata->p_memory->active_idx}) - 1u;
   uint4 mask = musl_load_mask<is_shared>(p_metadata->freed_mask);
   for (;;) {
      uint4 expected = mask;
      bool exchanged;
      if constexpr (is_shared) {
         exchanged = p_metadata->freed_mask.compare_exchange_weak(
            0u, expected, mask & ~active, memory_order::acq_rel,
            memory_order::acquire
         );
      } else if (
         musl_load_mask<is_shared>(p_metadata->freed_mask) == expected
      ) {
         musl_store_mask<is_shared>(p_metadata->freed_mask, mask & ~active);
         exchanged = true;
      } else {
         expected = musl_load_mask<is_shared>(p_metadata->freed_mask);
         exchanged = false;
      }
      if (exchanged) {
         break;
      }
      mask = expected;
   }
   mask &= active;
   musl_store_mask<is_shared>(p_metadata->available_mask, mask);
   return mask;
}

inline auto
slot_index(byte const* p_allocation) -> uint4 {
   return encoded_bytes(p_allocation)[-3] & 31u;
}

}  // namespace

template <bool is_shared>
auto
musl_context<is_shared>::get_meta(byte const* p_allocation)
   -> meta<is_shared>* {
   auto& context = *this;
   auto const* const p_encoded = encoded_bytes(p_allocation);
   verify((bit_cast<uword::raw_type>(p_allocation) & (musl_unit - 1u)) == 0u);

   uint2::raw_type raw_offset;
   // Misaligned load.
   __builtin_memcpy_inline(&raw_offset, p_encoded - 2u, sizeof(raw_offset));
   uword offset = raw_offset;
   uint4 const index = slot_index(p_allocation);
   if (p_encoded[-4] != 0u) {
      verify(offset == 0u);
      uint4::raw_type extended_offset;
      // Misaligned load.
      __builtin_memcpy_inline(
         &extended_offset, p_encoded - 8u, sizeof(extended_offset)
      );
      offset = extended_offset;
      verify(offset > 0xffffu);
      verify(offset < uword{limits<iword>::max()} / musl_unit);
   }

   auto const* const p_base = bit_cast<group<is_shared> const*>(
      p_allocation - (musl_unit * offset + musl_unit)
   );
   meta<is_shared> const* const p_metadata = p_base->p_meta;
   verify(p_metadata != nullptr);
   verify(p_metadata->p_memory == p_base);
   verify(index <= p_metadata->last_idx);
   uint4 const bit = uint4{1u} << index;
   verify((musl_load_mask<is_shared>(p_metadata->available_mask) & bit) == 0u);
   verify((musl_load_mask<is_shared>(p_metadata->freed_mask) & bit) == 0u);

   auto const* const p_area = bit_cast<meta_area<is_shared> const*>(
      bit_cast<uword::raw_type>(p_metadata) & -musl_page_bytes.raw
   );
   verify(p_area->check == context.secret);

   if (p_metadata->size_class < 48u) {
      verify(offset >= uword{size_classes[p_metadata->size_class]} * index);
      verify(
         offset < uword{size_classes[p_metadata->size_class]} * (index + 1u)
      );
   } else {
      verify(p_metadata->size_class == 63u);
   }
   if (p_metadata->map_length != 0u) {
      verify(
         offset <= p_metadata->map_length * musl_page_bytes / musl_unit - 1u
      );
   }
   return const_cast<meta<is_shared>*>(p_metadata);
}

namespace {

template <bool is_shared>
auto
stride(meta<is_shared> const* p_metadata) -> uword {
   if (p_metadata->last_idx == 0u && p_metadata->map_length != 0u) {
      return p_metadata->map_length * musl_page_bytes - musl_unit;
   }
   return musl_unit * size_classes[p_metadata->size_class];
}

auto
nominal_size(byte const* p_allocation, byte const* p_end) -> uword {
   auto const* const p_encoded_allocation = encoded_bytes(p_allocation);
   auto const* const p_encoded_end = encoded_bytes(p_end);
   uword reserved = uint1{p_encoded_allocation[-3]} >> 5u;
   if (reserved >= 5u) {
      verify(reserved == 5u);
      uint4::raw_type extended_reserved;
      // Misaligned load.
      __builtin_memcpy_inline(
         &extended_reserved, p_encoded_end - 4u, sizeof(extended_reserved)
      );
      reserved = extended_reserved;
      verify(reserved >= 5u);
      verify(p_encoded_end[-5] == 0u);
   }
   verify(reserved <= uword(p_end - p_allocation));
   verify(p_encoded_end[-reserved.raw] == 0u);
   verify(*p_encoded_end == 0u);
   return uword(p_end - reserved.raw - p_allocation);
}

void
set_size(byte* p_allocation, byte* p_end, uword bytes) {
   auto* const p_encoded_allocation = encoded_bytes(p_allocation);
   auto* const p_encoded_end = encoded_bytes(p_end);
   uword reserved = uword(p_end - p_allocation) - bytes;
   if (reserved != 0u) {
      p_encoded_end[-reserved.raw] = 0u;
   }
   if (reserved >= 5u) {
      verify(reserved <= limits<uint4>::max());
      uint4::raw_type const extended_reserved =
         static_cast<uint4::raw_type>(reserved);
      // Misaligned store.
      __builtin_memcpy_inline(
         p_encoded_end - 4u, &extended_reserved, sizeof(extended_reserved)
      );
      p_encoded_end[-5] = 0u;
      reserved = 5u;
   }
   p_encoded_allocation[-3] = static_cast<unsigned char>(
      (p_encoded_allocation[-3] & 31u) | (reserved << 5u)
   );
}

auto
feedback_size(
   byte const* p_allocation, byte const* p_end, uword requested_bytes
) -> uword {
   uword const available_bytes = uword(p_end - p_allocation);
   return available_bytes - requested_bytes >= 5u ? available_bytes - 5u
                                                  : available_bytes;
}

template <bool is_shared>
auto
enframe(
   meta<is_shared>* p_metadata, uint4 index, uword bytes, uint4 counter,
   uword* _Nullable p_feedback_bytes
) -> byte* {
   uword const slot_stride = stride(p_metadata);
   uword const slack = (slot_stride - musl_in_band - bytes) / musl_unit;
   byte* p_allocation = p_metadata->p_memory->storage + slot_stride * index;
   byte* const p_end = p_allocation + (slot_stride - musl_in_band);
   unsigned char* p_encoded = encoded_bytes(p_allocation);
   uint4 offset = counter;
   if (p_encoded[-3] != 0u) {
      uint2::raw_type previous_offset;
      // Misaligned load.
      __builtin_memcpy_inline(
         &previous_offset, p_encoded - 2u, sizeof(previous_offset)
      );
      offset = previous_offset + 1u;
   }
   offset &= 255u;
   verify(p_encoded[-4] == 0u);
   if (offset > slack) {
      uword mask = slack;
      mask |= mask >> 1u;
      mask |= mask >> 2u;
      mask |= mask >> 4u;
      offset &= static_cast<uint4>(mask);
      if (offset > slack) {
         offset -= static_cast<uint4>(slack + 1u);
      }
      verify(offset <= slack);
   }
   if (offset != 0u) {
      uint2::raw_type const raw_offset = static_cast<uint2::raw_type>(offset);
      // Misaligned store.
      __builtin_memcpy_inline(p_encoded - 2u, &raw_offset, sizeof(raw_offset));
      p_encoded[-3] = 7u << 5u;
      p_allocation += musl_unit * offset;
      p_encoded = encoded_bytes(p_allocation);
      unpoison_memory_region(p_allocation - 4u, 4u);
      p_encoded[-4] = 0u;
   }
   uword const group_offset =
      uword(p_allocation - p_metadata->p_memory->storage) / musl_unit;
   verify(group_offset <= 0xffffu);
   uint2::raw_type const raw_group_offset =
      static_cast<uint2::raw_type>(group_offset);
   // Misaligned store.
   __builtin_memcpy_inline(
      p_encoded - 2u, &raw_group_offset, sizeof(raw_group_offset)
   );
   p_encoded[-3] = static_cast<unsigned char>(index);
   if (p_feedback_bytes != nullptr) {
      bytes = feedback_size(p_allocation, p_end, bytes);
      *p_feedback_bytes = bytes;
   }
   unpoison_memory_region(p_allocation + bytes, 1u);
   unpoison_memory_region(p_end - 5u, 6u);
   set_size(p_allocation, p_end, bytes);
   unpoison_memory_region(p_allocation, bytes);
   return p_allocation;
}

auto
size_to_class(uword bytes) -> uint4 {
   uword units = (bytes + musl_in_band - 1u) >> 4u;
   if (units < 10u) {
      return static_cast<uint4>(units);
   }
   ++units;
   uint4 const value = static_cast<uint4>(units);
   uint4 result = ((28u - __builtin_clz(value.raw)) * 4u) + 8u;
   if (units > size_classes[result + 1u]) {
      result += 2u;
   }
   if (units > size_classes[result]) {
      ++result;
   }
   return result;
}

}  // namespace

template <bool is_shared>
void
musl_context<is_shared>::step_sequence() {
   if (sequence == 255u) {
      for (uint4 index = 0u; index < 32u; ++index) {
         unmap_sequence[index] = 0u;
      }
      sequence = 1u;
   } else {
      ++sequence;
   }
}

template <bool is_shared>
void
musl_context<is_shared>::record_sequence(uint4 size_class) {
   if (size_class - 7u < 32u) {
      unmap_sequence[size_class - 7u] = sequence;
   }
}

template <bool is_shared>
void
musl_context<is_shared>::account_bounce(uint4 size_class) {
   if (size_class - 7u < 32u) {
      uint4 const previous_sequence = unmap_sequence[size_class - 7u];
      if (
         previous_sequence != 0u
         && static_cast<unsigned char>(sequence - previous_sequence) < 10u
      ) {
         unsigned char& bounce = bounces[size_class - 7u];
         if (bounce + 1u < 100u) {
            ++bounce;
         } else {
            bounce = 150u;
         }
      }
   }
}

template <bool is_shared>
void
musl_context<is_shared>::decay_bounces(uint4 size_class) {
   if (size_class - 7u < 32u && bounces[size_class - 7u] != 0u) {
      --bounces[size_class - 7u];
   }
}

template <bool is_shared>
auto
musl_context<is_shared>::is_bouncing(uint4 size_class) const -> bool {
   return size_class - 7u < 32u && bounces[size_class - 7u] >= 100u;
}

template <bool is_shared>
auto
musl_context<is_shared>::try_available(meta<is_shared>* _Nullable* p_metadata)
   -> uint4 {
   meta<is_shared>* p_current = *p_metadata;
   if (p_current == nullptr) {
      return 0u;
   }

   uint4 mask = musl_load_mask<is_shared>(p_current->available_mask);
   if (mask == 0u) {
      if (musl_load_mask<is_shared>(p_current->freed_mask) == 0u) {
         dequeue(p_metadata, p_current);
         p_current = *p_metadata;
         if (p_current == nullptr) {
            return 0u;
         }
      } else {
         p_current = p_current->p_next;
         *p_metadata = p_current;
      }

      mask = musl_load_mask<is_shared>(p_current->freed_mask);
      if (mask == all_slots_mask(*p_current) && p_current->freeable != 0u) {
         p_current = p_current->p_next;
         *p_metadata = p_current;
         mask = musl_load_mask<is_shared>(p_current->freed_mask);
      }

      uint4 const active =
         (uint4{2u} << uint4{p_current->p_memory->active_idx}) - 1u;
      if ((mask & active) == 0u) {
         if (p_current->p_next != p_current) {
            p_current = p_current->p_next;
            *p_metadata = p_current;
         } else {
            uint4 count = p_current->p_memory->active_idx + 2u;
            uword const slot_bytes =
               musl_unit * size_classes[p_current->size_class];
            uword span_bytes = musl_unit + slot_bytes * count;
            while ((span_bytes ^ (span_bytes + slot_bytes - 1u))
                   < musl_page_bytes) {
               ++count;
               span_bytes += slot_bytes;
            }
            if (count > p_current->last_idx + 1u) {
               count = p_current->last_idx + 1u;
            }
            p_current->p_memory->active_idx =
               static_cast<unsigned char>(count - 1u);
         }
      }
      mask = activate_group(p_current);
      verify(mask != 0u);
      decay_bounces(static_cast<uint4>(p_current->size_class));
   }

   uint4 const first = mask & -mask.raw;
   musl_store_mask<is_shared>(p_current->available_mask, mask - first);
   return first;
}

template <bool is_shared>
auto
musl_context<is_shared>::allocate_group(uint4 size_class, uword request)
   -> meta<is_shared>* _Nullable {
   auto& context = *this;
   uword const slot_bytes = musl_unit * size_classes[size_class];
   uint4 table_index = 0u;
   uint4 count;
   meta<is_shared>* const p_metadata = allocate_meta();
   if (p_metadata == nullptr) {
      return nullptr;
   }
   uword usage = context.usage_by_class[size_class];
   uint4 active_idx;
   byte* p_memory;

   if (size_class < 9u) {
      while (table_index < 2u
             && 4u * small_count_table[size_class][table_index] > usage) {
         ++table_index;
      }
      count = small_count_table[size_class][table_index];
   } else {
      count = medium_count_table[size_class & 3u];
      while ((count & 1u) == 0u && 4u * count > usage) {
         count >>= 1u;
      }
      while (slot_bytes * count >= 65'536u * musl_unit) {
         count >>= 1u;
      }
   }

   if (count == 1u && slot_bytes * count + musl_unit <= musl_page_bytes / 2u) {
      count = 2u;
   }

   if (slot_bytes * count + musl_unit > musl_page_bytes / 2u) {
      bool const no_small = is_bouncing(size_class);
      account_bounce(size_class);
      step_sequence();

      if ((size_class & 1u) == 0u && size_class < 32u) {
         usage += context.usage_by_class[size_class + 1u];
      }

      if (4u * count > usage && !no_small) {
         if (
            (size_class & 3u) == 1u && slot_bytes * count > 8u * musl_page_bytes
         ) {
            count = 2u;
         } else if (
            (size_class & 3u) == 2u && slot_bytes * count > 4u * musl_page_bytes
         ) {
            count = 3u;
         } else if (
            (size_class & 3u) == 0u && slot_bytes * count > 8u * musl_page_bytes
         ) {
            count = 3u;
         } else if (
            (size_class & 3u) == 0u && slot_bytes * count > 2u * musl_page_bytes
         ) {
            count = 5u;
         }
      }

      uword needed = slot_bytes * count + musl_unit;
      needed = (needed + musl_page_bytes - 1u) & -musl_page_bytes.raw;

      if (!no_small && count <= 7u) {
         uword request_map = request + musl_in_band + musl_unit;
         request_map =
            (request_map + musl_page_bytes - 1u) & -musl_page_bytes.raw;
         if (
            request_map < slot_bytes + musl_unit
            || (request_map >= 4u * musl_page_bytes && 2u * count > usage)
         ) {
            count = 1u;
            needed = request_map;
         }
      }

      p_memory = map_pages(needed, nix::memory_protection_flags::read_write);
      if (p_memory == nullptr) {
         free_meta(p_metadata);
         return nullptr;
      }
      musl_add_counter<is_shared>(context.mapped_bytes, needed);
      p_metadata->map_length = needed.raw / musl_page_bytes.raw;
      ++context.mmap_counter;
      int4 calculated =
         static_cast<int4>((musl_page_bytes - musl_unit) / slot_bytes) - 1;
      if (calculated > static_cast<int4>(count - 1u)) {
         calculated = static_cast<int4>(count - 1u);
      }
      if (calculated < 0) {
         calculated = 0;
      }
      active_idx = static_cast<uint4>(calculated);
   } else {
      uint4 const parent_class =
         size_to_class(musl_unit + count * slot_bytes - musl_in_band);
      int4 const parent_index = allocate_slot(
         parent_class, musl_unit + count * slot_bytes - musl_in_band
      );
      if (parent_index < 0) {
         free_meta(p_metadata);
         return nullptr;
      }
      meta<is_shared>* const p_parent = context.active[parent_class];
      p_memory = enframe(
         p_parent, static_cast<uint4>(parent_index),
         musl_unit * size_classes[parent_class] - musl_in_band,
         context.mmap_counter, nullptr
      );
      p_metadata->map_length = 0u;
      unsigned char* const p_encoded = encoded_bytes(p_memory);
      p_encoded[-3] =
         static_cast<unsigned char>((p_encoded[-3] & 31u) | (6u << 5u));
      for (uint4 index = 0u; index <= count; ++index) {
         p_encoded[(musl_unit + index * slot_bytes - 4u).raw] = 0u;
      }
      active_idx = count - 1u;
   }

   context.usage_by_class[size_class] += count;
   musl_store_mask<is_shared>(
      p_metadata->available_mask, (uint4{2u} << active_idx) - 1u
   );
   musl_store_mask<is_shared>(
      p_metadata->freed_mask,
      (uint4{2u} << (count - 1u)) - 1u
         - musl_load_mask<is_shared>(p_metadata->available_mask)
   );
   p_metadata->p_memory = bit_cast<group<is_shared>*>(p_memory);
   p_metadata->p_memory->p_meta = p_metadata;
   p_metadata->p_memory->active_idx = static_cast<unsigned char>(active_idx);
   p_metadata->last_idx = (count - 1u).raw;
   p_metadata->freeable = 1u;
   p_metadata->size_class = size_class.raw;
   return p_metadata;
}

template <bool is_shared>
auto
musl_context<is_shared>::allocate_slot(uint4 size_class, uword request)
   -> int4 {
   uint4 const first = try_available(&active[size_class]);
   if (first != 0u) {
      return __builtin_ctz(first.raw);
   }

   meta<is_shared>* const p_group = allocate_group(size_class, request);
   if (p_group == nullptr) {
      return -1;
   }
   musl_store_mask<is_shared>(
      p_group->available_mask,
      musl_load_mask<is_shared>(p_group->available_mask) - 1u
   );
   queue(&active[size_class], p_group);
   return 0;
}

template <bool is_shared>
auto
musl_context<is_shared>::allocate_memory_unaccounted(
   uword bytes, uword* _Nullable p_feedback_bytes
) -> byte* _Nullable {
   auto& context = *this;
   if (bytes == 0u) {
      bytes = 1u;
   }
   if (bytes >= limits<uword>::max() / 2u - musl_page_bytes) {
      return nullptr;
   }

   meta<is_shared>* p_metadata;
   uint4 index;
   uint4 counter;
   if (bytes >= musl_mmap_threshold) {
      uword const needed =
         (bytes + musl_in_band + musl_unit + musl_page_bytes - 1u)
         & -musl_page_bytes.raw;
      byte* const p_mapping =
         map_pages(needed, nix::memory_protection_flags::read_write);
      if (p_mapping == nullptr) {
         return nullptr;
      }

      context.mutex.lock();
      step_sequence();
      p_metadata = allocate_meta();
      if (p_metadata == nullptr) {
         context.mutex.unlock();
         unmap_pages(p_mapping, needed);
         return nullptr;
      }
      musl_add_counter<is_shared>(context.mapped_bytes, needed);
      p_metadata->p_memory = bit_cast<group<is_shared>*>(p_mapping);
      p_metadata->p_memory->p_meta = p_metadata;
      p_metadata->last_idx = 0u;
      p_metadata->freeable = 1u;
      p_metadata->size_class = 63u;
      p_metadata->map_length = needed.raw / musl_page_bytes.raw;
      musl_store_mask<is_shared>(p_metadata->available_mask, 0u);
      musl_store_mask<is_shared>(p_metadata->freed_mask, 0u);
      ++context.mmap_counter;
      counter = context.mmap_counter;
      index = 0u;
      context.mutex.unlock();
   } else {
      context.mutex.lock();
      uint4 size_class = size_to_class(bytes);
      p_metadata = context.active[size_class];

      if (
         p_metadata == nullptr && size_class >= 4u && size_class < 32u
         && size_class != 6u && (size_class & 1u) == 0u
         && context.usage_by_class[size_class] == 0u
      ) {
         uword usage = context.usage_by_class[size_class | 1u];
         meta<is_shared>* const p_coarse = context.active[size_class | 1u];
         if (
            p_coarse == nullptr
            || (
               musl_load_mask<is_shared>(p_coarse->available_mask) == 0u
               && musl_load_mask<is_shared>(p_coarse->freed_mask) == 0u
            )
         ) {
            usage += 3u;
         }
         if (usage <= 12u) {
            size_class |= 1u;
         }
      }

      int4 const slot = allocate_slot(size_class, bytes);
      if (slot < 0) {
         context.mutex.unlock();
         return nullptr;
      }
      p_metadata = context.active[size_class];
      index = static_cast<uint4>(slot);
      counter = context.mmap_counter;
      context.mutex.unlock();
   }

   byte* const p_result =
      enframe(p_metadata, index, bytes, counter, p_feedback_bytes);
   return p_result;
}

template <bool is_shared>
[[gnu::always_inline]]
auto
musl_context<is_shared>::allocate_memory(
   uword bytes, uword* _Nullable p_feedback_bytes
) -> byte* _Nullable {
   byte* const p_result = allocate_memory_unaccounted(bytes, p_feedback_bytes);
   if (p_result != nullptr) {
      uword allocation_bytes = bytes == 0u ? 1u : bytes;
      if (p_feedback_bytes != nullptr) {
         allocation_bytes = *p_feedback_bytes;
      }
      musl_add_counter<is_shared>(live_bytes, allocation_bytes);
   }
   return p_result;
}

namespace {

template <bool is_shared>
auto
slot_bounds(meta<is_shared>* p_metadata, uint4 index) -> tuple<byte*, byte*> {
   uword const slot_stride = stride(p_metadata);
   byte* const p_start = p_metadata->p_memory->storage + slot_stride * index;
   return {p_start, p_start + (slot_stride - musl_in_band)};
}

}  // namespace

template <bool is_shared>
auto
musl_context<is_shared>::okay_to_free(meta<is_shared>* p_metadata) -> bool {
   uint4 const size_class = static_cast<uint4>(p_metadata->size_class);
   if (p_metadata->freeable == 0u) {
      return false;
   }
   if (
      size_class >= 48u
      || stride(p_metadata) < musl_unit * size_classes[size_class]
   ) {
      return true;
   }
   if (p_metadata->map_length == 0u) {
      return true;
   }
   if (p_metadata->p_next != p_metadata) {
      return true;
   }
   if (!is_bouncing(size_class)) {
      return true;
   }
   uword const count = p_metadata->last_idx + 1u;
   uword const usage = usage_by_class[size_class];
   return 9u * count <= usage && count < 20u;
}

template <bool is_shared>
auto
musl_context<is_shared>::free_group(meta<is_shared>* p_metadata)
   -> musl_map_info<is_shared> {
   auto& context = *this;
   musl_map_info<is_shared> result;
   uint4 const size_class = static_cast<uint4>(p_metadata->size_class);
   if (size_class < 48u) {
      context.usage_by_class[size_class] -= p_metadata->last_idx + 1u;
   }
   if (p_metadata->map_length != 0u) {
      step_sequence();
      record_sequence(size_class);
      result.p_base = bit_cast<byte*>(p_metadata->p_memory);
      result.bytes = uword{p_metadata->map_length} * musl_page_bytes;
   } else {
      byte* const p_memory = bit_cast<byte*>(p_metadata->p_memory);
      meta<is_shared>* const p_parent = get_meta(p_memory);
      uint4 const parent_index = slot_index(p_memory);
      p_metadata->p_memory->p_meta = nullptr;
      result = nontrivial_free(p_parent, parent_index);
   }
   free_meta(p_metadata);
   return result;
}

template <bool is_shared>
auto
musl_context<is_shared>::nontrivial_free(
   meta<is_shared>* p_metadata, uint4 index
) -> musl_map_info<is_shared> {
   auto& context = *this;
   uint4 const self = uint4{1u} << index;
   uint4 const size_class = static_cast<uint4>(p_metadata->size_class);
   uint4 const mask = musl_load_mask<is_shared>(p_metadata->freed_mask)
                      | musl_load_mask<is_shared>(p_metadata->available_mask);

   if (mask + self == all_slots_mask(*p_metadata) && okay_to_free(p_metadata)) {
      if (p_metadata->p_next != nullptr) {
         verify(size_class < 48u);
         bool const activate_new = context.active[size_class] == p_metadata;
         dequeue(&context.active[size_class], p_metadata);
         if (activate_new && context.active[size_class] != nullptr) {
            activate_group(context.active[size_class]);
         }
      }
      return free_group(p_metadata);
   }

   if (mask == 0u) {
      verify(size_class < 48u);
      if (context.active[size_class] != p_metadata) {
         queue(&context.active[size_class], p_metadata);
      }
   }
   if constexpr (is_shared) {
      p_metadata->freed_mask.fetch_or(0u, self, memory_order::release);
   } else {
      musl_store_mask<is_shared>(
         p_metadata->freed_mask,
         musl_load_mask<is_shared>(p_metadata->freed_mask) | self
      );
   }
   return {};
}

template <bool is_shared>
void
musl_context<is_shared>::finish_unmap(musl_map_info<is_shared> mapping) {
   if (mapping.bytes != 0u) {
      musl_subtract_counter<is_shared>(mapped_bytes, mapping.bytes);
      unmap_pages(mapping.p_base, mapping.bytes);
   }
}

template <bool is_shared>
auto
musl_context<is_shared>::deallocate_locked(
   byte* p_allocation, meta<is_shared>* p_metadata, uint4 index, uword old_bytes
) -> musl_map_info<is_shared> {
   musl_map_info<is_shared> const mapping = nontrivial_free(p_metadata, index);
   musl_subtract_counter<is_shared>(live_bytes, old_bytes);
   poison_memory_region(p_allocation, old_bytes);
   return mapping;
}

template <bool is_shared>
auto
musl_context<is_shared>::deallocate_fast(
   byte* p_allocation, meta<is_shared>* p_metadata, uint4 index, uword old_bytes
) -> bool {
   uint4 const self = uint4{1u} << index;
   uint4 const all = all_slots_mask(*p_metadata);
   poison_memory_region(p_allocation, old_bytes);
   for (;;) {
      uint4 freed = musl_load_mask<is_shared>(p_metadata->freed_mask);
      uint4 const available =
         musl_load_mask<is_shared>(p_metadata->available_mask);
      uint4 const mask = freed | available;
      verify((mask & self) == 0u);
      if (freed == 0u || mask + self == all) {
         return false;
      }
      if constexpr (is_shared) {
         uint4 expected = freed;
         if (!p_metadata->freed_mask.compare_exchange_weak(
                0u, expected, freed | self, memory_order::acq_rel,
                memory_order::acquire
             )) {
            continue;
         }
      } else {
         musl_store_mask<is_shared>(p_metadata->freed_mask, freed | self);
      }
      musl_subtract_counter<is_shared>(live_bytes, old_bytes);
      return true;
   }
}

template <bool is_shared>
auto
musl_context<is_shared>::prepare_free(byte* p_allocation)
   -> tuple<meta<is_shared>*, uint4, uword> {
   meta<is_shared>* const p_metadata = get_meta(p_allocation);
   uint4 const index = slot_index(p_allocation);
   auto const bounds = slot_bounds(p_metadata, index);
   uword const old_bytes = nominal_size(p_allocation, bounds.second());
   unsigned char* const p_encoded = encoded_bytes(p_allocation);
   p_encoded[-3] = 255u;
   uint2::raw_type const cleared_offset = 0u;
   // Misaligned store.
   __builtin_memcpy_inline(
      p_encoded - 2u, &cleared_offset, sizeof(cleared_offset)
   );
   return {p_metadata, index, old_bytes};
}

template <bool is_shared>
auto
musl_context<is_shared>::is_all_zero(byte* p_allocation) -> bool {
   meta<is_shared>* const p_metadata = get_meta(p_allocation);
   return p_metadata->size_class >= 48u
          || stride(p_metadata)
                < musl_unit * size_classes[p_metadata->size_class];
}

namespace {

auto
calloc_clear_bytes(byte* p_allocation, idx bytes) -> idx {
   if (bytes < musl_page_bytes) {
      return bytes;
   }

   byte* p_walk = p_allocation + bytes;
   uword clear_bytes = bit_cast<uword>(p_walk) & (musl_page_bytes - 1u);
   for (;;) {
      p_walk -= clear_bytes.raw;
      zero_memory(p_walk, idx(clear_bytes));
      if (idx(p_walk - p_allocation) < idx(musl_page_bytes)) {
         return idx(p_walk - p_allocation);
      }

      for (clear_bytes = musl_page_bytes; clear_bytes != 0u;
           clear_bytes -= (2u * sizeof(uint8)),
          p_walk -= (2u * sizeof(uint8))) {
         uint8::raw_type high;
         uint8::raw_type low;
         __builtin_memcpy_inline(&high, p_walk - sizeof(uint8), sizeof(uint8));
         __builtin_memcpy_inline(
            &low, p_walk - (2u * sizeof(uint8)), sizeof(uint8)
         );
         if ((high | low) != 0u) {
            break;
         }
      }
   }
}

}  // namespace

template <bool is_shared>
auto
musl_context<is_shared>::aligned_allocate(
   uword alignment, uword bytes, uword* _Nullable p_feedback_bytes
) -> byte* _Nullable {
   auto& context = *this;
   if (bytes == 0u) {
      bytes = 1u;
   }
   if (
      alignment == 0u || (alignment & -alignment.raw) != alignment
      || alignment >= musl_maximum_alignment
      || bytes > limits<uword>::max() - alignment
   ) {
      return nullptr;
   }
   if (alignment <= musl_unit) {
      return allocate_memory(bytes, p_feedback_bytes);
   }

   uword const expanded = bytes + alignment - musl_unit;
   byte* p_allocation = allocate_memory_unaccounted(expanded);
   if (p_allocation == nullptr) {
      return nullptr;
   }

   meta<is_shared>* const p_metadata = get_meta(p_allocation);
   uint4 const index = slot_index(p_allocation);
   uword const slot_stride = stride(p_metadata);
   byte* const p_start = p_metadata->p_memory->storage + slot_stride * index;
   byte* const p_end = p_metadata->p_memory->storage
                       + (slot_stride * (index + 1u) - musl_in_band);
   uword const adjustment =
      -bit_cast<uword::raw_type>(p_allocation) & (alignment - 1u);

   if (adjustment != 0u) {
      p_allocation += adjustment;
      unsigned char* const p_encoded_allocation = encoded_bytes(p_allocation);
      uword const offset =
         uword(p_allocation - p_metadata->p_memory->storage) / musl_unit;
      if (offset <= 0xffffu) {
         uint2::raw_type const raw_offset =
            static_cast<uint2::raw_type>(offset);
         // Misaligned store.
         __builtin_memcpy_inline(
            p_encoded_allocation - 2u, &raw_offset, sizeof(raw_offset)
         );
         p_encoded_allocation[-4] = 0u;
      } else {
         verify(offset <= limits<uint4>::max());
         uint2::raw_type const cleared_offset = 0u;
         // Misaligned store.
         __builtin_memcpy_inline(
            p_encoded_allocation - 2u, &cleared_offset, sizeof(cleared_offset)
         );
         uint4::raw_type const extended_offset =
            static_cast<uint4::raw_type>(offset);
         // Misaligned store.
         __builtin_memcpy_inline(
            p_encoded_allocation - 8u, &extended_offset, sizeof(extended_offset)
         );
         p_encoded_allocation[-4] = 1u;
      }
      p_encoded_allocation[-3] = static_cast<unsigned char>(index);

      uword const start_offset = uword(p_allocation - p_start) / musl_unit;
      if (start_offset <= 0xffffu) {
         uint2::raw_type const raw_start_offset =
            static_cast<uint2::raw_type>(start_offset);
         // Misaligned store.
         __builtin_memcpy_inline(
            encoded_bytes(p_start) - 2u, &raw_start_offset,
            sizeof(raw_start_offset)
         );
         encoded_bytes(p_start)[-3] = 7u << 5u;
      }
   }
   uword allocation_bytes = bytes;
   if (p_feedback_bytes != nullptr) {
      allocation_bytes = feedback_size(p_allocation, p_end, bytes);
      *p_feedback_bytes = allocation_bytes;
   }
   set_size(p_allocation, p_end, allocation_bytes);
   musl_add_counter<is_shared>(context.live_bytes, allocation_bytes);
   unpoison_memory_region(p_allocation, allocation_bytes);
   return p_allocation;
}

template <bool is_shared>
auto
musl_context<is_shared>::resize_in_place(
   byte*& p_allocation, uword old_allocation_bytes, uword new_bytes,
   bool may_move, uword* _Nullable p_feedback_bytes
) -> bool {
   auto& context = *this;
   meta<is_shared>* const p_metadata = get_meta(p_allocation);
   uint4 const index = slot_index(p_allocation);
   auto const bounds = slot_bounds(p_metadata, index);
   byte* const p_start = bounds.first();
   byte* const p_end = bounds.second();
   uword const old_bytes = nominal_size(p_allocation, p_end);
   verify(old_bytes == old_allocation_bytes);
   uword const available = uword(p_end - p_allocation);

   if (
      new_bytes <= available && new_bytes < musl_mmap_threshold
      && size_to_class(new_bytes) + 1u >= p_metadata->size_class
   ) {
      uword const allocation_bytes =
         p_feedback_bytes == nullptr
            ? new_bytes
            : feedback_size(p_allocation, p_end, new_bytes);
      if (allocation_bytes > old_bytes) {
         musl_add_counter<is_shared>(
            context.live_bytes, allocation_bytes - old_bytes
         );
         unpoison_memory_region(
            p_allocation + old_bytes, allocation_bytes - old_bytes
         );
      } else {
         musl_subtract_counter<is_shared>(
            context.live_bytes, old_bytes - allocation_bytes
         );
         poison_memory_region(
            p_allocation + allocation_bytes, old_bytes - allocation_bytes
         );
      }
      unpoison_memory_region(p_allocation + allocation_bytes, 1u);
      unpoison_memory_region(p_end - 5u, 6u);
      set_size(p_allocation, p_end, allocation_bytes);
      if (p_feedback_bytes != nullptr) {
         *p_feedback_bytes = allocation_bytes;
      }
      return true;
   }

   if (p_metadata->size_class >= 48u && new_bytes >= musl_mmap_threshold) {
      verify(p_metadata->size_class == 63u);
      uword const base = uword(p_allocation - p_start);
      uword const needed =
         (new_bytes + base + musl_unit + musl_in_band + musl_page_bytes - 1u)
         & -musl_page_bytes.raw;
      uword const old_mapping_bytes =
         uword{p_metadata->map_length} * musl_page_bytes;
      byte* p_new_mapping;
      if (old_mapping_bytes == needed) {
         p_new_mapping = bit_cast<byte*>(p_metadata->p_memory);
      } else {
         auto const remapped = nix::sys_mremap(
            p_metadata->p_memory, idx(old_mapping_bytes), idx(needed),
            may_move ? nix::mremap_flags::may_move : nix::mremap_flags::none
         );
         if (remapped.is_empty()) {
            return false;
         }
         p_new_mapping = remapped.value();
         if (needed > old_mapping_bytes) {
            musl_add_counter<is_shared>(
               context.mapped_bytes, needed - old_mapping_bytes
            );
         } else {
            musl_subtract_counter<is_shared>(
               context.mapped_bytes, old_mapping_bytes - needed
            );
         }
      }
      p_metadata->p_memory = bit_cast<group<is_shared>*>(p_new_mapping);
      p_metadata->p_memory->p_meta = p_metadata;
      p_metadata->map_length = needed.raw / musl_page_bytes.raw;
      p_allocation = p_metadata->p_memory->storage + base;
      byte* const p_new_end =
         p_metadata->p_memory->storage + (needed - musl_unit - musl_in_band);
      *p_new_end = byte(0u);
      uword const allocation_bytes =
         p_feedback_bytes == nullptr
            ? new_bytes
            : feedback_size(p_allocation, p_new_end, new_bytes);
      set_size(p_allocation, p_new_end, allocation_bytes);
      if (allocation_bytes > old_bytes) {
         musl_add_counter<is_shared>(
            context.live_bytes, allocation_bytes - old_bytes
         );
      } else {
         musl_subtract_counter<is_shared>(
            context.live_bytes, old_bytes - allocation_bytes
         );
      }
      unpoison_memory_region(p_allocation, allocation_bytes);
      if (p_feedback_bytes != nullptr) {
         *p_feedback_bytes = allocation_bytes;
      }
      return true;
   }
   return false;
}

template <bool is_shared>
void
musl_context<is_shared>::release_mappings() {
   auto& context = *this;
   for (meta_area<is_shared>* p_area = context.p_meta_area_head;
        p_area != nullptr; p_area = p_area->p_next) {
      for (uint4 index = 0u; index < p_area->nslots; ++index) {
         meta<is_shared>* const p_metadata = p_area->slots + index;
         if (p_metadata->p_memory != nullptr && p_metadata->map_length != 0u) {
            uword const bytes = uword{p_metadata->map_length} * musl_page_bytes;
            byte* const p_mapping = bit_cast<byte*>(p_metadata->p_memory);
            p_metadata->p_memory = nullptr;
            unmap_pages(p_mapping, bytes);
         }
      }
   }

   meta_area<is_shared>* p_mapping = context.p_meta_mapping_head;
   while (p_mapping != nullptr) {
      meta_area<is_shared>* const p_next = p_mapping->p_mapping_next;
      byte* const p_base = p_mapping->p_mapping_base;
      uword const bytes = p_mapping->mapping_bytes;
      unmap_pages(p_base, bytes);
      p_mapping = p_next;
   }
}

}  // namespace cat::detail

namespace cat {

template <bool is_shared>
auto
basic_musl_allocator<is_shared>::ensure_context() -> context* _Nullable {
   context* p_context = m_p_context.load(memory_order::acquire);
   if (p_context != nullptr) {
      return p_context;
   }
   context* const p_candidate = detail::make_context<is_shared>(m_secret);
   if (p_candidate == nullptr) {
      return nullptr;
   }
   context* p_expected = nullptr;
   if (!m_p_context.compare_exchange_strong(
          p_expected, p_candidate, memory_order::release, memory_order::acquire
       )) {
      detail::unmap_pages(
         bit_cast<byte*>(p_candidate), detail::musl_page_bytes
      );
      return p_expected;
   }
   return p_candidate;
}

template <bool is_shared>
basic_musl_allocator<is_shared>::basic_musl_allocator(
   basic_musl_allocator&& other
) noexcept {
   m_secret = other.m_secret;
   other.m_secret = 0u;
   m_p_context.store(
      other.m_p_context.exchange(nullptr, memory_order::acq_rel),
      memory_order::release
   );
}

template <bool is_shared>
auto
basic_musl_allocator<is_shared>::operator=(
   basic_musl_allocator&& other
) noexcept -> basic_musl_allocator& {
   if (this != &other) {
      reset();
      m_secret = other.m_secret;
      other.m_secret = 0u;
      m_p_context.store(
         other.m_p_context.exchange(nullptr, memory_order::acq_rel),
         memory_order::release
      );
   }
   return *this;
}

template <bool is_shared>
basic_musl_allocator<is_shared>::~basic_musl_allocator() {
   reset();
}

template <bool is_shared>
void
basic_musl_allocator<is_shared>::reset() {
   context* const p_context =
      m_p_context.exchange(nullptr, memory_order::acq_rel);
   if (p_context == nullptr) {
      return;
   }
   p_context->mutex.lock();
   p_context->release_mappings();
   detail::unmap_pages(bit_cast<byte*>(p_context), detail::musl_page_bytes);
}

template <bool is_shared>
auto
basic_musl_allocator<is_shared>::allocation_bytes(
   ualign alignment, idx requested_bytes
) -> maybe_non_zero<idx> {
   uword const alignment_bytes = alignment;
   uword const bytes = requested_bytes == 0u ? 1u : requested_bytes;
   if (
      alignment_bytes == 0u
      || (alignment_bytes & -alignment_bytes.raw) != alignment_bytes
      || alignment_bytes >= detail::musl_maximum_alignment
      || bytes > limits<uword>::max() - alignment_bytes
   ) {
      return nullopt;
   }
   return idx(bytes);
}

template <bool is_shared>
auto
basic_musl_allocator<is_shared>::allocate(idx requested_bytes)
   -> void* _Nullable {
   context* const p_context = ensure_context();
   if (p_context == nullptr) {
      return nullptr;
   }
   return p_context->allocate_memory(requested_bytes);
}

template <bool is_shared>
auto
basic_musl_allocator<is_shared>::aligned_allocate(
   ualign alignment, idx requested_bytes
) -> void* _Nullable {
   context* const p_context = ensure_context();
   if (p_context == nullptr) {
      return nullptr;
   }
   return p_context->aligned_allocate(alignment, requested_bytes);
}

template <bool is_shared>
auto
basic_musl_allocator<is_shared>::allocate_feedback(idx requested_bytes)
   -> maybe_sized_allocation<void*> {
   context* const p_context = ensure_context();
   if (p_context == nullptr) {
      return nullopt;
   }
   uword usable_bytes;
   byte* const p_result =
      p_context->allocate_memory(requested_bytes, &usable_bytes);
   if (p_result == nullptr) {
      return nullopt;
   }
   return sized_allocation<void*>{p_result, idx(usable_bytes)};
}

template <bool is_shared>
auto
basic_musl_allocator<is_shared>::aligned_allocate_feedback(
   ualign alignment, idx requested_bytes
) -> maybe_sized_allocation<void*> {
   context* const p_context = ensure_context();
   if (p_context == nullptr) {
      return nullopt;
   }
   uword usable_bytes;
   byte* const p_result =
      p_context->aligned_allocate(alignment, requested_bytes, &usable_bytes);
   if (p_result == nullptr) {
      return nullopt;
   }
   return sized_allocation<void*>{p_result, idx(usable_bytes)};
}

template <bool is_shared>
auto
basic_musl_allocator<is_shared>::allocate_zeroed(idx requested_bytes)
   -> void* _Nullable {
   context* const p_context = ensure_context();
   if (p_context == nullptr) {
      return nullptr;
   }
   byte* const p_result = p_context->allocate_memory(requested_bytes);
   if (p_result != nullptr && !p_context->is_all_zero(p_result)) {
      idx const bytes = requested_bytes == 0u ? 1u : requested_bytes;
      zero_memory(p_result, detail::calloc_clear_bytes(p_result, bytes));
   }
   return p_result;
}

template <bool is_shared>
auto
basic_musl_allocator<is_shared>::allocate_zeroed_feedback(idx requested_bytes)
   -> maybe_sized_allocation<void*> {
   maybe_sized_allocation<void*> result = allocate_feedback(requested_bytes);
   if (result.has_value()) {
      context* const p_context = m_p_context.load(memory_order::acquire);
      verify(p_context != nullptr);
      auto* const p_result = bit_cast<byte*>(result.value().first());
      if (!p_context->is_all_zero(p_result)) {
         zero_memory_scalar_explicit(
            p_result,
            detail::calloc_clear_bytes(p_result, result.value().second())
         );
      }
   }
   return result;
}

template <bool is_shared>
auto
basic_musl_allocator<is_shared>::aligned_allocate_zeroed(
   ualign alignment, idx requested_bytes
) -> void* _Nullable {
   context* const p_context = ensure_context();
   if (p_context == nullptr) {
      return nullptr;
   }
   byte* const p_result =
      p_context->aligned_allocate(alignment, requested_bytes);
   if (p_result != nullptr && !p_context->is_all_zero(p_result)) {
      idx const bytes = requested_bytes == 0u ? 1u : requested_bytes;
      zero_memory(p_result, detail::calloc_clear_bytes(p_result, bytes));
   }
   return p_result;
}

template <bool is_shared>
auto
basic_musl_allocator<is_shared>::aligned_allocate_zeroed_feedback(
   ualign alignment, idx requested_bytes
) -> maybe_sized_allocation<void*> {
   maybe_sized_allocation<void*> result =
      aligned_allocate_feedback(alignment, requested_bytes);
   if (result.has_value()) {
      context* const p_context = m_p_context.load(memory_order::acquire);
      verify(p_context != nullptr);
      auto* const p_result = bit_cast<byte*>(result.value().first());
      if (!p_context->is_all_zero(p_result)) {
         zero_memory_scalar_explicit(
            p_result,
            detail::calloc_clear_bytes(p_result, result.value().second())
         );
      }
   }
   return result;
}

template <bool is_shared>
void
basic_musl_allocator<is_shared>::deallocate(
   void const* p_storage, idx allocation_bytes
) {
   context* const p_context = m_p_context.load(memory_order::acquire);
   verify(p_context != nullptr);
   auto* const p_allocation =
      const_cast<byte*>(bit_cast<byte const*>(p_storage));
   auto const prepared = p_context->prepare_free(p_allocation);
   auto* const p_metadata = prepared.first();
   uint4 const index = prepared.second();
   uword const old_bytes = prepared.third();
   verify(old_bytes == (allocation_bytes == 0u ? 1u : allocation_bytes));

   if (p_context->deallocate_fast(p_allocation, p_metadata, index, old_bytes)) {
      return;
   }
   p_context->mutex.lock();
   detail::musl_map_info<is_shared> const mapping =
      p_context->deallocate_locked(p_allocation, p_metadata, index, old_bytes);
   p_context->mutex.unlock();
   p_context->finish_unmap(mapping);
}

template <bool is_shared>
auto
basic_musl_allocator<is_shared>::grow(span<byte> allocation, idx new_bytes)
   -> maybe<void> {
   context* const p_context = m_p_context.load(memory_order::acquire);
   verify(p_context != nullptr);
   byte* p_allocation = allocation.data();
   bool const resized = p_context->resize_in_place(
      p_allocation, allocation.size() == 0u ? 1u : allocation.size(),
      new_bytes == 0u ? 1u : new_bytes, false
   );
   return resized ? maybe<void>(monostate) : maybe<void>(nullopt);
}

template <bool is_shared>
auto
basic_musl_allocator<is_shared>::grow_feedback(
   span<byte> allocation, idx new_bytes
) -> maybe_non_zero<idx> {
   context* const p_context = m_p_context.load(memory_order::acquire);
   verify(p_context != nullptr);
   byte* p_allocation = allocation.data();
   uword usable_bytes;
   bool const resized = p_context->resize_in_place(
      p_allocation, allocation.size() == 0u ? 1u : allocation.size(),
      new_bytes == 0u ? 1u : new_bytes, false, &usable_bytes
   );
   if (!resized) {
      return nullopt;
   }
   return idx(usable_bytes);
}

template <bool is_shared>
auto
basic_musl_allocator<is_shared>::reallocate_memory(
   span<byte> allocation, idx new_bytes, uword* _Nullable p_feedback_bytes
) -> void* _Nullable {
   context* const p_context = m_p_context.load(memory_order::acquire);
   verify(p_context != nullptr);
   byte* p_allocation = allocation.data();
   uword const settled_bytes = new_bytes == 0u ? 1u : new_bytes;

   if (
      p_context->resize_in_place(
         p_allocation, allocation.size() == 0u ? 1u : allocation.size(),
         settled_bytes, true, p_feedback_bytes
      )
   ) {
      return p_allocation;
   }

   auto* const p_metadata = p_context->get_meta(p_allocation);
   uint4 const index = detail::slot_index(p_allocation);
   uword const old_bytes = detail::nominal_size(
      p_allocation, detail::slot_bounds(p_metadata, index).second()
   );
   byte* const p_new =
      p_context->allocate_memory(settled_bytes, p_feedback_bytes);
   if (p_new == nullptr) {
      return nullptr;
   }

   copy_memory(
      p_allocation, p_new,
      idx(old_bytes < settled_bytes ? old_bytes : settled_bytes)
   );
   deallocate(p_allocation, idx(old_bytes));
   return p_new;
}

template <bool is_shared>
auto
basic_musl_allocator<is_shared>::reallocate(
   span<byte> allocation, idx new_bytes
) -> void* _Nullable {
   return reallocate_memory(allocation, new_bytes, nullptr);
}

template <bool is_shared>
auto
basic_musl_allocator<is_shared>::reallocate_feedback(
   span<byte> allocation, idx new_bytes
) -> maybe_sized_allocation<void*> {
   uword usable_bytes;
   auto* const p_result =
      bit_cast<byte*>(reallocate_memory(allocation, new_bytes, &usable_bytes));
   if (p_result == nullptr) {
      return nullopt;
   }
   return sized_allocation<void*>{p_result, idx(usable_bytes)};
}

template <bool is_shared>
auto
basic_musl_allocator<is_shared>::bytes_used() const -> idx {
   context const* const p_context = m_p_context.load(memory_order::acquire);
   if (p_context == nullptr) {
      return 0u;
   }
   if constexpr (is_shared) {
      return idx(p_context->live_bytes.load(memory_order::acquire));
   } else {
      return idx(p_context->live_bytes);
   }
}

template <bool is_shared>
auto
basic_musl_allocator<is_shared>::bytes_capacity() const -> idx {
   context const* const p_context = m_p_context.load(memory_order::acquire);
   if (p_context == nullptr) {
      return 0u;
   }
   if constexpr (is_shared) {
      return idx(p_context->mapped_bytes.load(memory_order::acquire));
   } else {
      return idx(p_context->mapped_bytes);
   }
}

template <bool is_shared>
auto
basic_musl_allocator<is_shared>::is_equivalent(
   basic_musl_allocator const& other
) const -> bool {
   if (this == &other) {
      return true;
   }
   context* const p_context = m_p_context.load(memory_order::acquire);
   return p_context != nullptr
          && p_context == other.m_p_context.load(memory_order::acquire);
}

template class basic_musl_allocator<false>;
template class basic_musl_allocator<true>;

}  // namespace cat
