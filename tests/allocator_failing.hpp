// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

// An allocator for testing how containers handle allocation failure.

#include <cat/allocator>

#include "unit_tests.hpp"

namespace {

// `allocator_failing` allocates from `g_tests_alloc`, counts its live
// allocations so that tests can find leaks, and can fail on command.
// `.fail_after(n)` lets `n` more allocations or in-place growths succeed and
// then fails every later one, until `.never_fail()`.
class allocator_failing : public cat::allocator_interface<allocator_failing> {
 public:
   static constexpr cat::ualign min_alignment = 16u;

   cat::idx live_allocations;
   cat::idx allocation_count;
   cat::idx growth_count;
   bool can_grow = true;

   constexpr allocator_failing() = default;
   allocator_failing(allocator_failing const&) = delete;

   void
   fail_after(cat::idx count) {
      m_successes_left = count;
   }

   void
   never_fail() {
      m_successes_left = cat::nullopt;
   }

   [[nodiscard]]
   auto
   aligned_allocate(cat::ualign alignment, cat::idx bytes) -> void* _Nullable {
      if (should_fail()) {
         return nullptr;
      }

      cat::maybe<cat::span<cat::byte>> const memory =
         g_tests_alloc.align_alloc_multi_uninit<cat::byte>(
            max(alignment, min_alignment), bytes
         );
      if (!memory.has_value()) {
         return nullptr;
      }

      ++live_allocations;
      ++allocation_count;
      return memory.value().data();
   }

   [[nodiscard]]
   auto
   allocate(cat::idx bytes) -> void* _Nullable {
      return aligned_allocate(min_alignment, bytes);
   }

   void
   deallocate(void const* _Nullable p_storage, cat::idx bytes) {
      if (p_storage == nullptr) {
         return;
      }

      cat::verify(live_allocations > 0u);
      live_allocations = cat::idx(live_allocations - 1u);
      g_tests_alloc.free_multi_uninit(
         cat::span<cat::byte>(
            static_cast<cat::byte*>(const_cast<void*>(p_storage)), bytes
         )
      );
   }

   [[nodiscard]]
   auto
   grow(cat::span<cat::byte> allocation, cat::idx new_bytes)
      -> cat::maybe<void> {
      if (!can_grow || should_fail()) {
         return cat::nullopt;
      }

      cat::maybe<void> const result =
         g_tests_alloc.alloc_grow(allocation, new_bytes);
      if (result.has_value()) {
         ++growth_count;
      }

      return result;
   }

 private:
   auto
   should_fail() -> bool {
      if (!m_successes_left.has_value()) {
         return false;
      }

      if (m_successes_left.value() == 0u) {
         return true;
      }

      m_successes_left = cat::idx(m_successes_left.value() - 1u);
      return false;
   }

   cat::maybe<cat::idx> m_successes_left;
};

// Run `operation` while `allocator` fails after 0, then 1, then 2, and so on,
// successful allocations, until `operation` returns `true`. `on_failure` runs
// after each attempt that failed, to check that the failure did no harm.
template <typename Operation, typename OnFailure>
void
sweep_allocation_failures(
   allocator_failing& allocator, Operation&& operation, OnFailure&& on_failure
) {
   for (cat::idx successes;; ++successes) {
      cat::verify(successes < 10'000u);
      allocator.fail_after(successes);
      bool const has_succeeded = operation();
      allocator.never_fail();
      if (has_succeeded) {
         return;
      }

      on_failure();
   }
}

// An element which counts how many of its kind are alive, so that tests can
// find elements which a failed operation leaked or destroyed twice.
struct counted {
   static inline cat::iword live = 0;

   cat::int4 value;

   constexpr counted() : value(0) {
      ++live;
   }

   explicit constexpr counted(cat::int4 input) : value(input) {
      ++live;
   }

   constexpr counted(counted const& other) : value(other.value) {
      ++live;
   }

   constexpr counted(counted&& other) noexcept : value(other.value) {
      ++live;
   }

   constexpr auto
   operator=(counted const& other) -> counted& = default;
   constexpr auto
   operator=(counted&& other) noexcept -> counted& = default;

   constexpr ~counted() {
      cat::verify(live > 0);
      --live;
   }

   [[nodiscard]]
   friend constexpr auto
   operator==(counted const& left, counted const& right) -> bool {
      return left.value == right.value;
   }

   [[nodiscard]]
   friend constexpr auto
   operator<(counted const& left, counted const& right) -> bool {
      return left.value < right.value;
   }
};

}  // namespace
