#include <cat/allocator_parameters>
#include <cat/limits>
#include <cat/linux>
#include <cat/musl_allocator>
#include <cat/page_allocator>
#include <cat/sanitizer>
#include <cat/thread>

#include "../unit_tests.hpp"

namespace {

static_assert(cat::musl_allocator::min_alignment == 16u);
static_assert(cat::musl_allocator::min_allocation_bytes == 1u);

constexpr char child_armed = 1;
constexpr char child_returned = 3;

void
mark_child(nix::file_descriptor marker, char value) {
   nix::sys_write(marker, &value, 1u).verify();
}

template <typename Allocator>
concept exposes_deallocate_poisons =
   requires { Allocator::deallocate_poisons; };

template <typename Allocator>
concept exposes_deallocate_storage =
   requires(Allocator& allocator, void const* p_storage) {
      allocator.deallocate_storage(p_storage, 1u);
   };

static_assert(!exposes_deallocate_poisons<cat::musl_allocator>);
static_assert(!exposes_deallocate_storage<cat::musl_allocator>);
static_assert(
   !exposes_deallocate_storage<cat::allocator_ref<cat::musl_allocator>>
);
static_assert(!exposes_deallocate_storage<cat::dyn_allocator>);

constexpr idx mmap_threshold = 131'052u;
constexpr idx size_class_units[] = {
   1u,     2u,     3u,     4u,     5u,     6u,     7u,     8u,
   9u,     10u,    12u,    15u,    18u,    20u,    25u,    31u,
   36u,    42u,    50u,    63u,    72u,    84u,    102u,   127u,
   146u,   170u,   204u,   255u,   292u,   340u,   409u,   511u,
   584u,   682u,   818u,   1'023u, 1'169u, 1'364u, 1'637u, 2'047u,
   2'340u, 2'730u, 3'276u, 4'095u, 4'680u, 5'460u, 6'552u, 8'191u,
};

class musl_secret_engine {
 public:
   using result_type = cat::uint8;

   constexpr musl_secret_engine(result_type first, result_type remaining)
       : m_first(first), m_remaining(remaining) {
   }

   constexpr auto
   operator()() -> result_type {
      result_type const result = m_calls == 0u ? m_first : m_remaining;
      ++m_calls;
      return result;
   }

   static constexpr auto
   min() -> result_type {
      return 0u;
   }

   static constexpr auto
   max() -> result_type {
      return result_type::max();
   }

   constexpr auto
   calls() const -> idx {
      return m_calls;
   }

 private:
   result_type m_first;
   result_type m_remaining;
   idx m_calls = 0u;
};

class maybe_musl_secret_engine {
 public:
   explicit constexpr maybe_musl_secret_engine(cat::maybe<cat::uint8> result)
       : m_result(result) {
   }

   constexpr auto
   operator()() -> cat::maybe<cat::uint8> {
      ++m_calls;
      return m_result;
   }

   constexpr auto
   calls() const -> idx {
      return m_calls;
   }

 private:
   cat::maybe<cat::uint8> m_result;
   idx m_calls = 0u;
};

enum class entropy_error : cat::uint1::raw_type {
   unavailable,
};

class scaredy_musl_secret_engine {
 public:
   constexpr auto
   operator()() -> cat::scaredy<cat::uint8, entropy_error> {
      ++m_calls;
      return entropy_error::unavailable;
   }

   constexpr auto
   calls() const -> idx {
      return m_calls;
   }

 private:
   idx m_calls = 0u;
};

template <typename Allocator>
auto
allocate_bytes(Allocator& allocator, idx bytes) -> cat::byte* {
   return allocator.template alloc_multi_uninit<cat::byte>(bytes)
      .verify()
      .data();
}

template <typename Allocator>
void
deallocate_bytes(Allocator& allocator, void const* p_allocation, idx bytes) {
   allocator.free_multi_uninit(
      cat::span{
         const_cast<cat::byte*>(cat::bit_cast<cat::byte const*>(p_allocation)),
         bytes,
      }
   );
}

template <typename Allocator>
auto
aligned_allocate_bytes(Allocator& allocator, cat::ualign alignment, idx bytes)
   -> cat::byte* {
   return allocator
      .template align_alloc_multi_uninit<cat::byte>(alignment, bytes)
      .verify()
      .data();
}

template <typename Allocator>
void
shared_contention_worker_impl(
   Allocator& allocator, idx seed, idx iteration_count
) {
   for (idx iteration = 0u; iteration < iteration_count; ++iteration) {
      idx const bytes = (iteration + seed) % 257u + 1u;
      void* const p_allocation = allocate_bytes(allocator, bytes);
      cat::verify(p_allocation != nullptr);
      auto* const p_bytes = static_cast<cat::byte*>(p_allocation);
      p_bytes[0] = cat::byte(seed.raw);
      p_bytes[(bytes - 1u).raw] = cat::byte(iteration.raw);
      deallocate_bytes(allocator, p_allocation, bytes);
   }
}

void
shared_contention_worker(cat::musl_allocator_shared* p_allocator, idx seed) {
   shared_contention_worker_impl(*p_allocator, seed, 1'000u);
}

void
shared_dyn_contention_worker(cat::dyn_allocator* p_allocator, idx seed) {
   shared_contention_worker_impl(*p_allocator, seed, 4'000u);
}

void
shared_ref_contention_worker(
   cat::allocator_ref<cat::musl_allocator_shared>* p_allocator, idx seed
) {
   shared_contention_worker_impl(*p_allocator, seed, 4'000u);
}

void
corrupt_slot_index(nix::file_descriptor marker) {
   cat::musl_allocator allocator;
   void* const p_allocation = allocate_bytes(allocator, 32u);
   cat::verify(p_allocation != nullptr);
   auto* const p_bytes = static_cast<unsigned char*>(p_allocation);
   p_bytes[-3] = static_cast<unsigned char>((p_bytes[-3] & 0xe0u) | 31u);
   mark_child(marker, child_armed);
   deallocate_bytes(allocator, p_allocation, 32u);
   mark_child(marker, child_returned);
}

void
deallocate_with_wrong_size(nix::file_descriptor marker) {
   cat::musl_allocator allocator;
   void* const p_allocation = allocate_bytes(allocator, 32u);
   cat::verify(p_allocation != nullptr);
   mark_child(marker, child_armed);
   deallocate_bytes(allocator, p_allocation, 31u);
   mark_child(marker, child_returned);
}

void
double_free(nix::file_descriptor marker) {
   cat::musl_allocator allocator;
   void* const p_allocation = allocate_bytes(allocator, 32u);
   [[maybe_unused]]
   void* const p_neighbor = allocate_bytes(allocator, 32u);
   cat::verify(p_allocation != nullptr);
   cat::verify(p_neighbor != nullptr);
   deallocate_bytes(allocator, p_allocation, 32u);
   mark_child(marker, child_armed);
   deallocate_bytes(allocator, p_allocation, 32u);
   mark_child(marker, child_returned);
}

void
deallocate_misaligned_pointer(nix::file_descriptor marker) {
   cat::musl_allocator allocator;
   void* const p_allocation = allocate_bytes(allocator, 32u);
   cat::verify(p_allocation != nullptr);
   auto* const p_bytes = static_cast<cat::byte*>(p_allocation);
   mark_child(marker, child_armed);
   deallocate_bytes(allocator, p_bytes + 1u, 31u);
   mark_child(marker, child_returned);
}

void
deallocate_foreign_pointer(nix::file_descriptor marker) {
   cat::musl_allocator allocator;
   void* const p_context = allocate_bytes(allocator, 1u);
   cat::verify(p_context != nullptr);
   alignas(16) cat::byte foreign[32] = {};
   mark_child(marker, child_armed);
   deallocate_bytes(allocator, foreign, 32u);
   mark_child(marker, child_returned);
}

void
grow_with_wrong_size(nix::file_descriptor marker) {
   cat::musl_allocator allocator;
   void* const p_allocation = allocate_bytes(allocator, 32u);
   cat::verify(p_allocation != nullptr);
   cat::span allocation{static_cast<cat::byte*>(p_allocation), 31u};
   mark_child(marker, child_armed);
   [[maybe_unused]]
   cat::maybe<void> const result = allocator.alloc_grow(allocation, 32u);
   mark_child(marker, child_returned);
}

void
deallocate_after_reset(nix::file_descriptor marker) {
   cat::musl_allocator allocator;
   void* const p_allocation = allocate_bytes(allocator, 32u);
   cat::verify(p_allocation != nullptr);
   allocator.reset();
   mark_child(marker, child_armed);
   deallocate_bytes(allocator, p_allocation, 32u);
   mark_child(marker, child_returned);
}

void
corrupt_extended_offset(nix::file_descriptor marker) {
   cat::musl_allocator allocator;
   void* const p_allocation = allocate_bytes(allocator, 32u);
   cat::verify(p_allocation != nullptr);
   auto* const p_bytes = static_cast<unsigned char*>(p_allocation);
   p_bytes[-4] = 1u;
   mark_child(marker, child_armed);
   deallocate_bytes(allocator, p_allocation, 32u);
   mark_child(marker, child_returned);
}

void
corrupt_size_trailer(nix::file_descriptor marker) {
   cat::musl_allocator allocator;
   void* const p_allocation = allocate_bytes(allocator, 12u);
   cat::verify(p_allocation != nullptr);
   auto* const p_bytes = static_cast<unsigned char*>(p_allocation);
   p_bytes[12] = 1u;
   mark_child(marker, child_armed);
   deallocate_bytes(allocator, p_allocation, 12u);
   mark_child(marker, child_returned);
}

template <typename Allocator>
void
verify_cross_thread_release(Allocator& allocator) {
   cat::span allocation =
      allocator.template alloc_multi_uninit<cat::byte>(64u).verify();
   [[maybe_unused]]
   cat::byte* const p_storage = allocation.data();
   cat::atomic<bool> released;
   cat::thread thread;
   thread
      .spawn(
         pager, 1'024_uki,
         [&allocator, allocation, &released] {
            allocator.free_multi_uninit(allocation);
            released.store(true, cat::memory_order::release);
         }
      )
      .verify();
   while (!released.load(cat::memory_order::acquire)) {
      cat::relax_cpu();
   }
#if __has_feature(address_sanitizer)
   cat::verify(cat::__asan_address_is_poisoned(p_storage) != 0);
   cat::verify(cat::__asan_address_is_poisoned(p_storage + 63u) != 0);
#endif
   thread.join().verify();
   thread.free(pager);
}

void
verify_child_hardening(void (*p_test)(nix::file_descriptor)) {
   nix::file_descriptor pipe[2] = {};
   nix::sys_pipe(pipe).verify();

   nix::process child;
   child.spawn(pager, 1_umi, p_test, pipe[1]).verify();
   cat::int4 child_status;
   nix::sys_wait4(
      child.id(), &child_status, nix::wait_options_flags::none, nullptr
   )
      .verify();
   nix::sys_close(pipe[1]).verify();

   char markers[2] = {};
   idx const bytes = nix::sys_read(pipe[0], markers, 2u).verify();
   nix::sys_close(pipe[0]).verify();
   child.free(pager);
   cat::verify(child_status == 1u << 8u);
   cat::verify(bytes == 1u);
   cat::verify(markers[0] == child_armed);
}

template <typename Allocator>
void
verify_musl_allocator_equivalence() {
   Allocator first;
   Allocator second;
   Allocator const& first_self = first;
   Allocator const& second_self = second;
   cat::verify(first == first_self);
   cat::verify(second == second_self);
   cat::verify(!(first == second));

   void* const p_first = allocate_bytes(first, 32u);
   void* const p_second = allocate_bytes(second, 32u);
   cat::verify(!(first == second));

   Allocator moved{cat::move(first)};
   Allocator const& moved_self = moved;
   cat::verify(moved == moved_self);
   cat::verify(!(moved == first));
   cat::verify(!(moved == second));
   deallocate_bytes(moved, p_first, 32u);
   deallocate_bytes(second, p_second, 32u);

   cat::allocator_ref moved_ref = moved;
   cat::allocator_ref moved_alias = moved;
   cat::allocator_ref second_ref = second;
   cat::verify(moved_ref == moved_alias);
   cat::verify(!(moved_ref == second_ref));

   cat::dyn_allocator moved_dyn = moved;
   cat::dyn_allocator moved_dyn_alias = moved;
   cat::dyn_allocator second_dyn = second;
   cat::verify(moved_dyn == moved_dyn_alias);
   cat::verify(!(moved_dyn == second_dyn));

   moved.reset();
   cat::verify(moved == moved_self);
   cat::verify(!(moved == second));
   void* const p_recreated = allocate_bytes(moved, 48u);
   cat::verify(!(moved == second));
   cat::verify(moved_ref == moved_alias);
   cat::verify(moved_dyn == moved_dyn_alias);
   deallocate_bytes(moved, p_recreated, 48u);
}

}  // namespace

$test(musl_allocator_allocates_size_classes) {
   cat::musl_allocator allocator;
   void* const p_zero = allocate_bytes(allocator, 0u);
   cat::verify(p_zero != nullptr);
   deallocate_bytes(allocator, p_zero, 0u);

   constexpr idx sizes[] = {
      1u, 15u, 16u, 17u, 255u, 4_uki, 130_uki, 256_uki,
   };
   cat::byte* allocations[sizeof(sizes) / sizeof(sizes[0])] = {};

   for (idx index = 0u; index < sizeof(allocations) / sizeof(allocations[0]);
        ++index) {
      allocations[index] = allocate_bytes(allocator, sizes[index]);
      cat::verify(allocations[index] != nullptr);
      cat::verify(cat::is_aligned(allocations[index], 16u));
      cat::byte* const p_bytes = allocations[index];
      p_bytes[0] = cat::byte(index.raw);
      p_bytes[(sizes[index] - 1u).raw] = cat::byte(sizes[index].raw);
   }

   for (idx index = 0u; index < sizeof(allocations) / sizeof(allocations[0]);
        ++index) {
      deallocate_bytes(allocator, allocations[index], sizes[index]);
   }
   cat::verify(allocator.bytes_used() == 0u);
}

$test(musl_allocator_exhausts_size_class_boundaries) {
   cat::musl_allocator allocator;
   idx previous_max;

   for (idx class_index;
        class_index < sizeof(size_class_units) / sizeof(size_class_units[0]);
        ++class_index) {
      idx class_max = size_class_units[class_index] * 16u;
      class_max = idx(class_max - 4u);
      if (class_max >= mmap_threshold) {
         class_max = mmap_threshold - 1u;
      }
      idx const class_min = previous_max == 0u ? 1u : previous_max + 1u;
      constexpr idx probes_per_class = 2u;
      idx const sizes[probes_per_class.raw] = {class_min, class_max};
      for (idx probe; probe < probes_per_class; ++probe) {
         idx const bytes = sizes[probe];
         void* const p_allocation = allocate_bytes(allocator, bytes);
         cat::verify(p_allocation != nullptr);
         auto* const p_bytes = static_cast<cat::byte*>(p_allocation);
         p_bytes[0] = cat::byte(class_index.raw);
         p_bytes[(bytes - 1u).raw] = cat::byte(probe.raw);
         deallocate_bytes(allocator, p_allocation, bytes);
      }
      previous_max = class_max;
   }
   cat::verify(previous_max == mmap_threshold - 1u);
   cat::verify(allocator.bytes_used() == 0u);
}

$test(musl_allocator_exhausts_small_sizes) {
   cat::musl_allocator allocator;
   for (idx bytes = 1u; bytes <= 4_uki; ++bytes) {
      cat::byte* const p_allocation = allocate_bytes(allocator, bytes);
      cat::verify(p_allocation != nullptr);
      cat::byte* const p_bytes = p_allocation;
      p_bytes[0] = cat::byte(bytes.raw);
      p_bytes[(bytes - 1u).raw] = cat::byte((bytes >> 8u).raw);
      deallocate_bytes(allocator, p_allocation, bytes);
   }
   cat::verify(allocator.bytes_used() == 0u);
}

$test(musl_allocator_alignment) {
   cat::musl_allocator allocator;
   constexpr cat::ualign alignments[] = {
      1u, 16u, 32u, 64u, 256u, 4_uki, 64_uki, 1_umi,
   };

   for (cat::ualign alignment : alignments) {
      void* const p_allocation =
         aligned_allocate_bytes(allocator, alignment, 257u);
      cat::verify(p_allocation != nullptr);
      cat::verify(cat::is_aligned(p_allocation, alignment));
      auto* const p_bytes = static_cast<cat::byte*>(p_allocation);
      p_bytes[0] = cat::byte(1u);
      p_bytes[256] = cat::byte(2u);
      deallocate_bytes(allocator, p_allocation, 257u);
   }
}

$test(musl_allocator_feedback_hooks) {
   cat::musl_allocator allocator;

   auto [zero, zero_bytes] =
      allocator.salloc_multi_uninit<cat::byte>(0u).verify();
   cat::verify(zero.data() != nullptr);
   cat::verify(zero_bytes >= 1u);
   cat::verify(allocator.bytes_used() == zero_bytes);
   cat::span zero_full{zero.data(), zero_bytes};
   zero_full[idx(zero_bytes - 1u)] = cat::byte(1u);
   allocator.free_multi_uninit(zero_full);
   cat::verify(allocator.bytes_used() == 0u);

   auto [allocation, allocation_bytes] =
      allocator.salloc_multi_uninit<cat::byte>(257u).verify();
   cat::verify(allocation_bytes > 257u);
   cat::verify(allocator.bytes_used() == allocation_bytes);
   cat::span allocation_full{allocation.data(), allocation_bytes};
   allocation_full[idx(allocation_bytes - 1u)] = cat::byte(2u);
   allocator.free_multi_uninit(allocation_full);
   cat::verify(allocator.bytes_used() == 0u);

   auto [aligned, aligned_bytes] =
      allocator.align_salloc_multi_uninit<cat::byte>(4_uki, 513u).verify();
   cat::verify(aligned_bytes >= 513u);
   cat::verify(cat::is_aligned(aligned.data(), 4_uki));
   cat::verify(allocator.bytes_used() == aligned_bytes);
   cat::span aligned_full{aligned.data(), aligned_bytes};
   aligned_full[idx(aligned_bytes - 1u)] = cat::byte(3u);
   allocator.free_multi_uninit(aligned_full);
   cat::verify(allocator.bytes_used() == 0u);
}

$test(musl_allocator_feedback_capacity_boundaries) {
   cat::musl_allocator allocator;
   constexpr idx sizes[] = {
      1u,
      13u,
      17u,
      255u,
      256u,
      4_uki - 3u,
      mmap_threshold - 1u,
      mmap_threshold,
      200_uki,
   };
   for (idx requested : sizes) {
      auto [allocation, usable_bytes] =
         allocator.salloc_multi_uninit<cat::byte>(requested).verify();
      cat::verify(usable_bytes >= requested);
      cat::span full_allocation{allocation.data(), usable_bytes};
      full_allocation[idx(usable_bytes - 1u)] = cat::byte(requested.raw);
      allocator.free_multi_uninit(full_allocation);
   }

   constexpr cat::ualign alignments[] = {32u, 4_uki, 16_umi};
   for (cat::ualign alignment : alignments) {
      auto [allocation, usable_bytes] =
         allocator.align_salloc_multi_uninit<cat::byte>(alignment, 513u)
            .verify();
      cat::verify(cat::is_aligned(allocation.data(), alignment));
      cat::verify(usable_bytes >= 513u);
      cat::span full_allocation{allocation.data(), usable_bytes};
      full_allocation[idx(usable_bytes - 1u)] = cat::byte(4u);
      allocator.free_multi_uninit(full_allocation);
   }
   cat::verify(allocator.bytes_used() == 0u);
}

$test(musl_allocator_zeroes_reused_storage) {
   cat::musl_allocator allocator;
   constexpr idx bytes = 257u;
   void* const p_dirty = allocate_bytes(allocator, bytes);
   cat::verify(p_dirty != nullptr);
   cat::fill_memory(p_dirty, static_cast<unsigned char>(0xa5u), bytes);
   deallocate_bytes(allocator, p_dirty, bytes);

   void* const p_zeroed =
      allocator.alloc_multi<cat::byte>(bytes).verify().data();
   cat::verify(p_zeroed != nullptr);
   auto const* const p_bytes = static_cast<cat::byte const*>(p_zeroed);
   for (idx index = 0u; index < bytes; ++index) {
      cat::verify(p_bytes[index] == 0u);
   }
   deallocate_bytes(allocator, p_zeroed, bytes);
}

$test(musl_allocator_zeroed_alignment_and_feedback) {
   cat::musl_allocator allocator;
   constexpr idx bytes = 513u;

   void* const p_zeroed =
      allocator.align_alloc_multi<cat::byte>(4_uki, bytes).verify().data();
   cat::verify(p_zeroed != nullptr);
   cat::verify(cat::is_aligned(p_zeroed, 4_uki));
   auto const* const p_zeroed_bytes = static_cast<cat::byte const*>(p_zeroed);
   for (idx index; index < bytes; ++index) {
      cat::verify(p_zeroed_bytes[index] == 0u);
   }
   deallocate_bytes(allocator, p_zeroed, bytes);

   auto [feedback, feedback_bytes] =
      allocator.salloc_multi<cat::byte>(bytes).verify();
   cat::verify(feedback_bytes > bytes);
   cat::span feedback_full{feedback.data(), feedback_bytes};
   for (idx index; index < feedback_bytes; ++index) {
      cat::verify(feedback_full[index] == cat::byte(0u));
   }
   allocator.free_multi(feedback_full);

   auto [aligned, aligned_bytes] =
      allocator.align_salloc_multi<cat::byte>(64u, bytes).verify();
   cat::verify(aligned_bytes >= bytes);
   cat::verify(cat::is_aligned(aligned.data(), 64u));
   cat::span aligned_full{aligned.data(), aligned_bytes};
   for (idx index; index < aligned_bytes; ++index) {
      cat::verify(aligned_full[index] == cat::byte(0u));
   }
   allocator.free_multi(aligned_full);
}

$test(musl_allocator_grows_in_place) {
   cat::musl_allocator allocator;
   void* const p_allocation = allocate_bytes(allocator, 17u);
   cat::verify(p_allocation != nullptr);
   auto* const p_bytes = static_cast<cat::byte*>(p_allocation);
   p_bytes[0] = cat::byte(7u);
   p_bytes[16] = cat::byte(9u);

   cat::span allocation{p_bytes, 17u};
   cat::verify(allocator.alloc_grow(allocation, 24u).has_value());
   cat::verify(p_bytes[0] == cat::byte(7u));
   cat::verify(p_bytes[16] == cat::byte(9u));
   deallocate_bytes(allocator, p_allocation, 24u);
}

$test(musl_allocator_grow_shrink_and_feedback) {
   cat::musl_allocator allocator;
   void* const p_allocation = allocate_bytes(allocator, 80u);
   cat::verify(p_allocation != nullptr);
   auto* const p_bytes = static_cast<cat::byte*>(p_allocation);
   p_bytes[0] = cat::byte(3u);
   p_bytes[31] = cat::byte(5u);

   cat::span allocation{p_bytes, 80u};
   cat::verify(allocator.alloc_grow(allocation, 64u).has_value());
   cat::verify(p_bytes[0] == cat::byte(3u));
   cat::verify(p_bytes[31] == cat::byte(5u));

   cat::span shrunk{p_bytes, 64u};
   auto const grown = allocator.alloc_grow_feedback(shrunk, 72u);
   cat::verify(grown.has_value());
   cat::verify(grown.value() >= 72u);
   cat::verify(allocator.bytes_used() == grown.value());
   cat::span grown_allocation{p_bytes, grown.value()};
   grown_allocation[idx(grown.value() - 1u)] = cat::byte(7u);
   allocator.free_multi_uninit(grown_allocation);
}

$test(musl_allocator_reallocate_feedback_capacity) {
   cat::musl_allocator allocator;
   auto [initial, initial_bytes] =
      allocator.salloc_multi_uninit<cat::byte>(257u).verify();
   cat::verify(allocator.bytes_used() == initial_bytes);
   cat::span initial_full{initial.data(), initial_bytes};
   initial_full[0u] = cat::byte(9u);
   initial_full[idx(initial_bytes - 1u)] = cat::byte(11u);

   constexpr idx requested = 200_uki;
   auto [resized, resized_bytes] =
      allocator.resalloc_multi_uninit(initial.data(), initial_bytes, requested)
         .verify();
   cat::verify(resized_bytes >= requested);
   cat::verify(allocator.bytes_used() == resized_bytes);
   cat::span resized_full{resized.data(), resized_bytes};
   cat::verify(resized_full[0u] == cat::byte(9u));
   cat::verify(resized_full[idx(initial_bytes - 1u)] == cat::byte(11u));
   resized_full[idx(resized_bytes - 1u)] = cat::byte(13u);
   allocator.free_multi_uninit(resized_full);
   cat::verify(allocator.bytes_used() == 0u);
}

$test(musl_allocator_reallocates_and_preserves_data) {
   cat::musl_allocator allocator;
   void* const p_allocation = allocate_bytes(allocator, 64u);
   cat::verify(p_allocation != nullptr);
   auto* const p_bytes = static_cast<cat::byte*>(p_allocation);
   for (idx index = 0u; index < 64u; ++index) {
      p_bytes[index.raw] = cat::byte(index.raw);
   }

   void* const p_reallocated =
      allocator.realloc_multi_uninit_to(allocator, p_bytes, 64u, 200_uki)
         .verify()
         .data();
   cat::verify(p_reallocated != nullptr);
   auto const* const p_result = static_cast<cat::byte const*>(p_reallocated);
   for (idx index = 0u; index < 64u; ++index) {
      cat::verify(p_result[index.raw] == cat::byte(index.raw));
   }
   deallocate_bytes(allocator, p_reallocated, 200_uki);
}

$test(musl_allocator_reallocates_within_size_class) {
   cat::musl_allocator allocator;
   void* const p_allocation = allocate_bytes(allocator, 64u);
   cat::verify(p_allocation != nullptr);
   auto* const p_bytes = static_cast<cat::byte*>(p_allocation);
   for (idx index; index < 64u; ++index) {
      p_bytes[index.raw] = cat::byte(index.raw);
   }

   void* const p_grown =
      allocator.realloc_multi_uninit_to(allocator, p_bytes, 64u, 72u)
         .verify()
         .data();
   auto const* const p_grown_bytes = static_cast<cat::byte const*>(p_grown);
   for (idx index; index < 64u; ++index) {
      cat::verify(p_grown_bytes[index.raw] == cat::byte(index.raw));
   }

   void* const p_shrunk =
      allocator
         .realloc_multi_uninit_to(
            allocator, static_cast<cat::byte*>(p_grown), 72u, 64u
         )
         .verify()
         .data();
   auto const* const p_shrunk_bytes = static_cast<cat::byte const*>(p_shrunk);
   for (idx index; index < 64u; ++index) {
      cat::verify(p_shrunk_bytes[index.raw] == cat::byte(index.raw));
   }
   deallocate_bytes(allocator, p_shrunk, 64u);
}

$test(musl_allocator_mmap_and_mremap_boundaries) {
   cat::musl_allocator allocator;

   void* const p_slotted = allocate_bytes(allocator, mmap_threshold - 1u);
   cat::verify(p_slotted != nullptr);
   auto* const p_slotted_bytes = static_cast<cat::byte*>(p_slotted);
   p_slotted_bytes[0] = cat::byte(11u);
   p_slotted_bytes[(mmap_threshold - 2u).raw] = cat::byte(13u);
   deallocate_bytes(allocator, p_slotted, mmap_threshold - 1u);

   void* const p_mapped = allocate_bytes(allocator, mmap_threshold);
   cat::verify(p_mapped != nullptr);
   auto* const p_mapped_bytes = static_cast<cat::byte*>(p_mapped);
   for (idx index; index < 256u; ++index) {
      p_mapped_bytes[index.raw] = cat::byte(index.raw);
   }
   idx const mapped_capacity = allocator.bytes_capacity();

   constexpr idx grown_bytes = 256_uki;
   void* const p_grown =
      allocator
         .realloc_multi_uninit_to(
            allocator, p_mapped_bytes, mmap_threshold, grown_bytes
         )
         .verify()
         .data();
   cat::verify(p_grown != nullptr);
   auto const* const p_result = static_cast<cat::byte const*>(p_grown);
   for (idx index; index < 256u; ++index) {
      cat::verify(p_result[index.raw] == cat::byte(index.raw));
   }
   cat::verify(allocator.bytes_capacity() >= mapped_capacity);

   void* const p_shrunk = allocator
                             .realloc_multi_uninit_to(
                                allocator, static_cast<cat::byte*>(p_grown),
                                grown_bytes, mmap_threshold
                             )
                             .verify()
                             .data();
   cat::verify(p_shrunk != nullptr);
   auto const* const p_shrunk_bytes = static_cast<cat::byte const*>(p_shrunk);
   for (idx index; index < 256u; ++index) {
      cat::verify(p_shrunk_bytes[index.raw] == cat::byte(index.raw));
   }
   deallocate_bytes(allocator, p_shrunk, mmap_threshold);
   cat::verify(allocator.bytes_used() == 0u);
}

$test(musl_allocator_reset_and_move) {
   cat::musl_allocator source;
   void* const p_source = allocate_bytes(source, 512u);
   cat::verify(p_source != nullptr);
   cat::verify(source.bytes_used() == 512u);

   cat::musl_allocator moved{cat::move(source)};
   cat::verify(source.bytes_used() == 0u);
   cat::verify(moved.bytes_used() == 512u);
   deallocate_bytes(moved, p_source, 512u);

   cat::musl_allocator assigned;
   void* const p_released = allocate_bytes(assigned, 128u);
   cat::verify(p_released != nullptr);
   assigned = cat::move(moved);
   cat::verify(moved.bytes_capacity() == 0u);
   cat::verify(assigned.bytes_used() == 0u);

   void* const p_reset = allocate_bytes(assigned, 64u);
   cat::verify(p_reset != nullptr);
   assigned.reset();
   cat::verify(assigned.bytes_used() == 0u);
   cat::verify(assigned.bytes_capacity() == 0u);
}

$test(musl_allocator_reset_reuses_allocator) {
   cat::musl_allocator allocator = cat::make_musl_allocator();
   void* const p_first = allocate_bytes(allocator, 64u);
   cat::verify(p_first != nullptr);
   allocator.reset();
   cat::verify(allocator.bytes_used() == 0u);
   cat::verify(allocator.bytes_capacity() == 0u);

   void* const p_second = allocate_bytes(allocator, 128u);
   cat::verify(p_second != nullptr);
   deallocate_bytes(allocator, p_second, 128u);
   cat::verify(allocator.bytes_used() == 0u);
}

$test(musl_allocator_uses_injected_secrets) {
   musl_secret_engine first_engine{0x12345678'9abcdef0u, 1u};
   musl_secret_engine second_engine{0xfedcba98'76543210u, 1u};
   cat::musl_allocator first = cat::make_musl_allocator(first_engine);
   cat::musl_allocator second = cat::make_musl_allocator(second_engine);
   cat::verify(first_engine.calls() == 1u);
   cat::verify(second_engine.calls() == 1u);

   void* const p_first = allocate_bytes(first, 32u);
   void* const p_second = allocate_bytes(second, 32u);
   cat::verify(first_engine.calls() == 1u);
   cat::verify(second_engine.calls() == 1u);
   cat::verify(!first.is_equivalent(second));
   deallocate_bytes(first, p_first, 32u);
   deallocate_bytes(second, p_second, 32u);

   musl_secret_engine shared_engine{0x31415926'53589793u, 1u};
   cat::musl_allocator_shared shared =
      cat::make_musl_allocator_shared(shared_engine);
   void* const p_shared = allocate_bytes(shared, 48u);
   cat::verify(shared_engine.calls() == 1u);
   deallocate_bytes(shared, p_shared, 48u);
}

$test(musl_allocator_retries_zero_injected_secret) {
   musl_secret_engine engine{0u, 0x01234567'89abcdefu};
   cat::musl_allocator allocator = cat::make_musl_allocator(engine);
   cat::verify(engine.calls() == 2u);
   void* const p_allocation = allocate_bytes(allocator, 64u);
   cat::verify(engine.calls() == 2u);
   deallocate_bytes(allocator, p_allocation, 64u);
}

$test(musl_allocator_propagates_entropy_failure) {
   maybe_musl_secret_engine maybe_engine{cat::nullopt};
   auto maybe_allocator = cat::make_musl_allocator(maybe_engine);
   static_assert(
      cat::is_same<decltype(maybe_allocator), cat::maybe<cat::musl_allocator>>
   );
   cat::verify(maybe_allocator.is_empty());
   cat::verify(maybe_engine.calls() == 1u);

   scaredy_musl_secret_engine scaredy_engine;
   auto scaredy_allocator = cat::make_musl_allocator_shared(scaredy_engine);
   static_assert(cat::is_same<
                 decltype(scaredy_allocator),
                 cat::scaredy<cat::musl_allocator_shared, entropy_error>>);
   cat::verify(scaredy_allocator.is<entropy_error>());
   cat::verify(scaredy_engine.calls() == 1u);
}

$test(musl_allocator_default_factory_entropy) {
   static_assert(
      cat::is_same<decltype(cat::make_musl_allocator()), cat::musl_allocator>
   );
   static_assert(cat::is_same<
                 decltype(cat::make_musl_allocator_shared()),
                 cat::musl_allocator_shared>);
   cat::musl_allocator allocator = cat::make_musl_allocator();
   void* const p_allocation = allocate_bytes(allocator, 24u);
   cat::verify(p_allocation != nullptr);
   deallocate_bytes(allocator, p_allocation, 24u);
}

$test(musl_allocator_equivalence) {
   verify_musl_allocator_equivalence<cat::musl_allocator>();
   verify_musl_allocator_equivalence<cat::musl_allocator_shared>();
}

$test(musl_allocator_parameter_forwarding) {
   cat::musl_allocator allocator;
   cat::allocator_ref ref = allocator;

   cat::span<cat::byte> ref_allocation =
      ref.alloc_multi_uninit<cat::byte>(37u).verify();
   cat::verify(allocator.bytes_used() == 37u);
   ref.free_multi_uninit(ref_allocation);

   cat::dyn_allocator dyn = allocator;
   cat::span<cat::byte> dyn_allocation =
      dyn.alloc_multi_uninit<cat::byte>(73u).verify();
   cat::verify(allocator.bytes_used() == 73u);
   dyn.free_multi_uninit(dyn_allocation);
   cat::verify(allocator.bytes_used() == 0u);

   auto* const p_aligned = dyn.align_alloc<int4>(64u, 17).verify();
   cat::verify(*p_aligned == 17);
   cat::verify(cat::is_aligned(p_aligned, 64u));
   dyn.free(p_aligned);
}

$test(musl_allocator_deallocation_poisoning) {
   cat::musl_allocator allocator;
   auto allocation = allocator.alloc_multi_uninit<cat::byte>(32u).verify();
   cat::byte* const p_storage = allocation.data();

   allocator.free_multi_uninit(allocation);
   cat::verify(allocator.bytes_used() == 0u);
#if __has_feature(address_sanitizer)
   cat::verify(cat::__asan_address_is_poisoned(p_storage) != 0);
   cat::verify(cat::__asan_address_is_poisoned(p_storage + 31u) != 0);
#endif

   allocation = allocator.alloc_multi_uninit<cat::byte>(32u).verify();
#if __has_feature(address_sanitizer)
   cat::verify(cat::__asan_address_is_poisoned(allocation.data()) == 0);
   cat::verify(cat::__asan_address_is_poisoned(allocation.data() + 31u) == 0);
#endif
   allocation[0u] = cat::byte(1u);
   allocation[31u] = cat::byte(2u);
   allocator.free_multi_uninit(allocation);
}

$test(musl_allocator_shared_forwarded_release_ordering) {
   cat::musl_allocator_shared allocator;
   verify_cross_thread_release(allocator);
   cat::allocator_ref ref = allocator;
   verify_cross_thread_release(ref);
   cat::dyn_allocator dyn = allocator;
   verify_cross_thread_release(dyn);
   cat::verify(allocator.bytes_used() == 0u);
}

$test(musl_allocator_byte_counters) {
   cat::musl_allocator allocator;
   cat::verify(allocator.bytes_used() == 0u);
   cat::verify(allocator.bytes_capacity() == 0u);

   void* const p_small = allocate_bytes(allocator, 31u);
   void* const p_large = allocate_bytes(allocator, 200_uki);
   cat::verify(p_small != nullptr);
   cat::verify(p_large != nullptr);
   cat::verify(allocator.bytes_used() == 200_uki + 31u);
   cat::verify(allocator.bytes_capacity() >= allocator.bytes_used());

   deallocate_bytes(allocator, p_small, 31u);
   cat::verify(allocator.bytes_used() == 200_uki);
   deallocate_bytes(allocator, p_large, 200_uki);
   cat::verify(allocator.bytes_used() == 0u);
}

$test(musl_allocator_shared_contention) {
   cat::musl_allocator_shared allocator;
   cat::thread threads[8];

   for (idx index = 0u; index < 8u; ++index) {
      threads[index]
         .spawn(
            pager, 1'024_uki, shared_contention_worker, &allocator, index + 0u
         )
         .verify();
   }
   for (idx index = 0u; index < 8u; ++index) {
      threads[index].join().verify();
      threads[index].free(pager);
   }
   cat::verify(allocator.bytes_used() == 0u);
}

$test(musl_allocator_shared_lock_wakeup_stress) {
   constexpr idx thread_count = 16u;
   constexpr idx iteration_count = 5'000u;
   cat::musl_allocator_shared allocator;
   cat::atomic<uint4> ready;
   cat::atomic<bool> start;
   cat::thread threads[thread_count.raw];

   for (idx index; index < thread_count; ++index) {
      threads[index]
         .spawn(
            pager, 1'024_uki,
            [&allocator, &ready, &start, index, iteration_count] {
               ready.fetch_add(1u, cat::memory_order::release);
               while (!start.load(cat::memory_order::acquire)) {
                  cat::relax_cpu();
               }
               shared_contention_worker_impl(allocator, index, iteration_count);
            }
         )
         .verify();
   }
   while (ready.load(cat::memory_order::acquire) != thread_count) {
      cat::relax_cpu();
   }
   start.store(true, cat::memory_order::release);
   for (idx index; index < thread_count; ++index) {
      threads[index].join().verify();
      threads[index].free(pager);
   }
   cat::verify(allocator.bytes_used() == 0u);
}

$test(musl_allocator_shared_aligned_accounting_is_final) {
   constexpr idx allocation_bytes = 320u;
   cat::musl_allocator_shared allocator;
   void* const p_initial =
      aligned_allocate_bytes(allocator, 64u, allocation_bytes);
   deallocate_bytes(allocator, p_initial, allocation_bytes);
   cat::atomic<bool> start;
   cat::atomic<bool> done;
   cat::thread thread;
   thread
      .spawn(
         pager, 1'024_uki,
         [&allocator, &start, &done, allocation_bytes] {
            while (!start.load(cat::memory_order::acquire)) {
               cat::relax_cpu();
            }
            for (idx iteration; iteration < 20'000u; ++iteration) {
               void* const p_allocation =
                  aligned_allocate_bytes(allocator, 64u, allocation_bytes);
               deallocate_bytes(allocator, p_allocation, allocation_bytes);
            }
            done.store(true, cat::memory_order::release);
         }
      )
      .verify();
   start.store(true, cat::memory_order::release);
   while (!done.load(cat::memory_order::acquire)) {
      idx const used = allocator.bytes_used();
      cat::verify(used == 0u || used == allocation_bytes);
   }
   thread.join().verify();
   thread.free(pager);
   cat::verify(allocator.bytes_used() == 0u);
}

$test(musl_allocator_hot_group_transitions) {
   constexpr idx allocation_count = 256u;
   constexpr idx allocation_bytes = 32u;
   cat::musl_allocator_shared allocator;
   cat::byte* allocations[allocation_count.raw] = {};

   for (idx index; index < allocation_count; ++index) {
      allocations[index] = allocate_bytes(allocator, allocation_bytes);
   }
   for (idx index; index < allocation_count; index += 2u) {
      deallocate_bytes(allocator, allocations[index], allocation_bytes);
      allocations[index] = allocate_bytes(allocator, allocation_bytes);
   }
   for (idx index; index < allocation_count; ++index) {
      deallocate_bytes(allocator, allocations[index], allocation_bytes);
   }
   cat::verify(allocator.bytes_used() == 0u);
}

$test(musl_allocator_shared_forwarded_contention) {
   cat::musl_allocator_shared allocator;
   cat::dyn_allocator dyn = allocator;
   cat::allocator_ref ref = allocator;
   cat::thread threads[8];

   for (idx index = 0u; index < 8u; ++index) {
      if (index % 2u == 0u) {
         threads[index]
            .spawn(
               pager, 1'024_uki, shared_dyn_contention_worker, &dyn, index + 0u
            )
            .verify();
      } else {
         threads[index]
            .spawn(
               pager, 1'024_uki, shared_ref_contention_worker, &ref, index + 0u
            )
            .verify();
      }
   }
   for (idx index = 0u; index < 8u; ++index) {
      threads[index].join().verify();
      threads[index].free(pager);
   }
   cat::verify(allocator.bytes_used() == 0u);
}

$test(musl_allocator_shared_cross_thread_deallocation) {
   constexpr idx allocation_count = 1'024u;
   constexpr idx thread_count = 8u;
   cat::musl_allocator_shared allocator;
   cat::byte* allocations[allocation_count.raw] = {};

   for (idx index; index < allocation_count; ++index) {
      idx const bytes = index % 251u + 1u;
      allocations[index] = allocate_bytes(allocator, bytes);
      cat::verify(allocations[index] != nullptr);
   }

   cat::thread threads[thread_count.raw];
   for (idx thread_index; thread_index < thread_count; ++thread_index) {
      threads[thread_index]
         .spawn(
            pager, 1'024_uki,
            [&allocator, &allocations, thread_index, allocation_count,
             thread_count] {
               for (idx index = thread_index; index < allocation_count;
                    index += thread_count) {
                  idx const bytes = index % 251u + 1u;
                  deallocate_bytes(allocator, allocations[index], bytes);
               }
            }
         )
         .verify();
   }
   for (idx thread_index; thread_index < thread_count; ++thread_index) {
      threads[thread_index].join().verify();
      threads[thread_index].free(pager);
   }
   cat::verify(allocator.bytes_used() == 0u);
}

$test(musl_allocator_deterministic_stress) {
   constexpr idx slot_count = 128u;
   constexpr idx iteration_count = 20'000u;
   cat::musl_allocator allocator;
   cat::byte* allocations[slot_count.raw] = {};
   idx sizes[slot_count.raw] = {};
   cat::uword state = 0x9e3779b9u;

   for (idx iteration; iteration < iteration_count; ++iteration) {
      state = state * 1'664'525u + 1'013'904'223u;
      idx const slot = idx(state % slot_count);
      if (allocations[slot] != nullptr) {
         deallocate_bytes(allocator, allocations[slot], sizes[slot]);
         allocations[slot] = nullptr;
         sizes[slot] = 0u;
         continue;
      }

      state = state * 1'664'525u + 1'013'904'223u;
      idx const bytes = idx(state % 200'000u) + 1u;
      cat::byte* const p_allocation = allocate_bytes(allocator, bytes);
      cat::verify(p_allocation != nullptr);
      cat::byte* const p_bytes = p_allocation;
      p_bytes[0] = cat::byte(slot.raw);
      p_bytes[(bytes - 1u).raw] = cat::byte(iteration.raw);
      allocations[slot] = p_allocation;
      sizes[slot] = bytes;
   }

   for (idx slot; slot < slot_count; ++slot) {
      if (allocations[slot] != nullptr) {
         deallocate_bytes(allocator, allocations[slot], sizes[slot]);
      }
   }
   cat::verify(allocator.bytes_used() == 0u);
}

$test(musl_allocator_rejects_overflow) {
   cat::musl_allocator allocator;
   constexpr cat::ualign maximum_alignment{cat::uword{1u} << 35u};
   cat::verify(
      allocator.align_nalloc_multi<cat::byte>(maximum_alignment, 1u).is_empty()
   );
   cat::verify(allocator.alloc_multi_uninit<cat::byte>(cat::limits<idx>::max())
                  .is_empty());
   cat::verify(allocator
                  .align_alloc_multi_uninit<cat::byte>(maximum_alignment, 1u)
                  .is_empty());
   cat::verify(
      allocator
         .align_alloc_multi_uninit<cat::byte>(64u, cat::limits<idx>::max())
         .is_empty()
   );
}

$test(musl_allocator_hardening) {
   verify_child_hardening(corrupt_slot_index);
   verify_child_hardening(deallocate_with_wrong_size);
   verify_child_hardening(double_free);
   verify_child_hardening(deallocate_misaligned_pointer);
   verify_child_hardening(deallocate_foreign_pointer);
   verify_child_hardening(grow_with_wrong_size);
   verify_child_hardening(deallocate_after_reset);
   verify_child_hardening(corrupt_extended_offset);
   verify_child_hardening(corrupt_size_trailer);
}
