// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/detail/word_lock.hpp>

#include <cat/allocator>
#include <cat/atomic>
#include <cat/bit>
#include <cat/chrono>
#include <cat/linux>
#include <cat/new>
#include <cat/propagate>
#include <cat/utility>

namespace cat::detail::mutex_parking_lot {

// The hash table's bucket count is the number of threads multiplied by three,
// rounded up to the nearest power of two. This coefficient keeps memory
// overhead low, at a few hundred bytes per thread, while keeping hash
// collisions rare.
inline constexpr idx load_factor = 3;

enum class [[clang::enum_extensibility(
   closed
)]] unpark_token : uint1::raw_type {
   normal,
   handoff,
};

enum class [[clang::enum_extensibility(closed)]] park_status : uint1::raw_type {
   unparked,
   invalid,
   timed_out,
};

struct park_result {
   park_status m_status;
   unpark_token m_token;
};

struct unpark_result {
   idx m_unparked_threads;
   bool m_have_more_threads;
   bool m_be_fair;
   bool m_parked_with_timeout;
};

class fair_timeout {
 public:
   fair_timeout(clock_steady::time_point timeout, uint4 seed, bool has_timeout)
       : m_timeout(timeout), m_seed(seed), m_has_timeout(has_timeout) {
   }

   auto
   should_timeout() -> bool {
      maybe<clock_steady::time_point> const now = clock_steady::now();
      if (!now.has_value()) {
         return false;
      }

      if (!m_has_timeout || now.value() > m_timeout) {
         uint4 const nanoseconds = generate() % 1'000'000u;
         m_timeout = now.value()
                     + units::nanosecond_int8(nanoseconds * units::nanosecond);
         m_has_timeout = true;
         return true;
      }

      return false;
   }

 private:
   auto
   generate() -> uint4 {
      // Xorshift32 RNG advances per bucket fairness jitter.
      // TODO: Should we provide an `xorshift_engine`?
      m_seed = m_seed ^ (m_seed << 13);
      m_seed = m_seed ^ (m_seed >> 17);
      m_seed = m_seed ^ (m_seed << 5);
      return m_seed;
   }

   clock_steady::time_point m_timeout;
   uint4 m_seed;
   bool m_has_timeout;
};

struct thread_data;

struct alignas(64) bucket {
   bucket(clock_steady::time_point timeout, uint4 seed, bool has_timeout)
       : m_timeout(timeout, seed, has_timeout) {
   }

   word_lock m_mutex;
   thread_data* m_p_queue_head = nullptr;
   thread_data* m_p_queue_tail = nullptr;
   fair_timeout m_timeout;
};

struct hash_table {
   bucket* m_p_entries;
   idx m_size;
   uint1 m_hash_bits;
   idx m_allocation_bytes;
};

inline atomic<hash_table*> p_hash_table;
inline atomic<idx> num_threads;

struct parker {
   void
   prepare_park() {
      m_futex.m_value.relaxed() = 1;
   }

   void
   park() {
      while (m_futex.m_value.acquire() != 0) {
         scaredy result = m_futex.wait(1);
         if (result.has_value()) {
            continue;
         }
         nix::linux_error const error = result.error();
         if (
            error != nix::linux_error::again && error != nix::linux_error::intr
         ) {
            result.verify();
         }
      }
   }

   template <is_clock Clock, is_duration Duration>
   auto
   park_until(time_point<Clock, Duration> timeout) -> bool {
      typename Clock::time_point const deadline =
         typename Clock::time_point(timeout);
      while (m_futex.m_value.acquire() != 0) {
         typename Clock::time_point const now = Clock::now().assert();
         if (now >= deadline) {
            return false;
         }

         nix::futex_timespec const remaining =
            nix::make_timespec(deadline - now);
         scaredy result = m_futex.wait(1, &remaining);
         if (result.has_value()) {
            continue;
         }

         nix::linux_error const error = result.error();
         if (error == nix::linux_error::timedout) {
            return false;
         }
         if (
            error != nix::linux_error::again && error != nix::linux_error::intr
         ) {
            result.verify();
         }
      }
      return true;
   }

   auto
   unpark_lock() -> nix::futex* {
      m_futex.m_value.release() = 0;
      return &m_futex;
   }

   static void
   unpark(nix::futex* p_futex) {
      // The waiter may have already observed the store and left, so waking
      // nobody is fine.
      auto _ = p_futex->wake();
   }

   nix::futex m_futex;
};

struct thread_data {
   thread_data() {
      num_threads.relaxed().fetch_add(1);
   }

   ~thread_data() {
      num_threads.relaxed().fetch_sub(1);
   }

   parker m_thread_parker;
   atomic<uword> m_key;
   thread_data* m_p_next_in_queue = nullptr;
   unpark_token m_unpark = unpark_token::normal;
   uword m_park_token = 0;
   bool m_parked_with_timeout = false;
};

inline thread_local thread_data local_thread_data;

template <is_allocator Allocator>
auto
allocate_hash_table(allocator_ref<Allocator> allocator, idx thread_count)
   -> maybe<hash_table*> {
   assert(thread_count > 0);

   idx const size = bit_ceil(thread_count * load_factor);
   uint1 const hash_bits = bit_width(size) - 1;
   idx const entries_offset =
      (sizeof(hash_table) + alignof(bucket) - 1) & ~(alignof(bucket) - 1);
   idx const mapping_bytes = entries_offset + size * sizeof(bucket);
   constexpr ualign allocation_alignment = alignof(bucket) > alignof(hash_table)
                                              ? alignof(bucket)
                                              : alignof(hash_table);

   auto [mapping, allocation_bytes] =
      $prop(allocator.template align_salloc_multi<byte>(
         allocation_alignment, mapping_bytes
      ));
   byte* const p_mapping = mapping.data();

   hash_table* const p_table = new (p_mapping) hash_table{
      .m_p_entries = __builtin_bit_cast(bucket*, p_mapping + entries_offset),
      .m_size = size,
      .m_hash_bits = hash_bits,
      .m_allocation_bytes = allocation_bytes,
   };

   maybe<clock_steady::time_point> const now = clock_steady::now();
   clock_steady::time_point const initial_timeout =
      now.has_value() ? now.value() : clock_steady::time_point{};
   for (idx index; index < size; ++index) {
      new (&p_table->m_p_entries[index]) bucket(
         initial_timeout, narrow_cast<uint4>(index + 1).assert(),
         now.has_value()
      );
   }

   return p_table;
}

template <is_allocator Allocator, typename Free>
void
destroy_hash_table(
   allocator_ref<Allocator> allocator, hash_table* p_table, Free&& free_mapping
) {
   if (p_table == nullptr) {
      return;
   }

   for (idx index; index < p_table->m_size; ++index) {
      bucket& current = p_table->m_p_entries[index];
      current.m_mutex.lock();
      assert(current.m_p_queue_head == nullptr);
      assert(current.m_p_queue_tail == nullptr);
      current.m_mutex.unlock();
      current.~bucket();
   }

   idx const allocation_bytes = p_table->m_allocation_bytes;
   p_table->~hash_table();
   $fwd(free_mapping)(
      allocator, span<byte>(reinterpret_cast<byte*>(p_table), allocation_bytes)
   );
}

}  // namespace cat::detail::mutex_parking_lot

namespace cat {

// `alloc_mutex_runtime` allocates the parking lot runtime if it has not already
// been initialized. It is mostly thread-safe and idempotent, in that multiple
// callers can race to allocate it. However, `free_mutex_runtime` is NOT
// thread-safe, it must not be called concurrently with `alloc_mutex_runtime`,
// and callers must not race other `free_mutex_runtime` calls.

template <is_allocator Allocator>
auto
alloc_mutex_runtime(allocator_ref<Allocator> allocator, idx thread_count)
   -> maybe<void> {
   if (detail::mutex_parking_lot::p_hash_table.acquire() != nullptr) {
      return monostate;
   }

   detail::mutex_parking_lot::hash_table* const p_new_table = $prop(
      detail::mutex_parking_lot::allocate_hash_table(allocator, thread_count)
   );
   detail::mutex_parking_lot::hash_table* p_expected = nullptr;

   if (
      detail::mutex_parking_lot::p_hash_table.compare_exchange_strong(
         p_expected, p_new_table, memory_order::acq_rel, memory_order::acquire
      )
   ) {
      return monostate;
   }

   detail::mutex_parking_lot::destroy_hash_table(
      allocator, p_new_table,
      [](allocator_ref<Allocator> current_allocator, span<byte> mapping) {
         current_allocator.free_multi(mapping);
      }
   );

   return monostate;
}

inline auto
alloc_mutex_runtime(dyn_allocator allocator, idx thread_count) -> maybe<void> {
   return alloc_mutex_runtime<dyn_allocator>(allocator, thread_count);
}

template <is_allocator Allocator>
void
free_mutex_runtime(allocator_ref<Allocator> allocator) {
   detail::mutex_parking_lot::hash_table* const p_table =
      detail::mutex_parking_lot::p_hash_table.exchange(
         nullptr, memory_order::acq_rel
      );
   detail::mutex_parking_lot::destroy_hash_table(
      allocator, p_table,
      [](allocator_ref<Allocator> current_allocator, span<byte> mapping) {
         current_allocator.free_multi(mapping);
      }
   );
}

inline void
free_mutex_runtime(dyn_allocator allocator) {
   free_mutex_runtime<dyn_allocator>(allocator);
}

template <is_allocator Allocator>
void
cfree_mutex_runtime(allocator_ref<Allocator> allocator) {
   detail::mutex_parking_lot::hash_table* const p_table =
      detail::mutex_parking_lot::p_hash_table.exchange(
         nullptr, memory_order::acq_rel
      );
   detail::mutex_parking_lot::destroy_hash_table(
      allocator, p_table,
      [](allocator_ref<Allocator> current_allocator, span<byte> mapping) {
         current_allocator.cfree_multi(mapping);
      }
   );
}

inline void
cfree_mutex_runtime(dyn_allocator allocator) {
   cfree_mutex_runtime<dyn_allocator>(allocator);
}

}  // namespace cat

namespace cat::detail::mutex_parking_lot {

// TODO: Provide re-usable hash functions.
inline auto
hash(uword key, uint1 bits) -> idx {
   // Golden ratio hash.
   // https://softwareengineering.stackexchange.com/questions/402542/where-do-magic-hashing-constants-like-0x9e3779b9-and-0x9e3779b1-come-from/402543#402543
   uword const hash = key.wrap() * 0x9e3779b9'7f4a7c15u;
   return narrow_cast<idx>(hash >> (64 - bits)).assert();
}

inline auto
lock_bucket(uword key) -> bucket* {
   hash_table* const p_table = p_hash_table.acquire();
   assert(p_table != nullptr);

   bucket* const p_bucket =
      &p_table->m_p_entries[hash(key, p_table->m_hash_bits)];
   p_bucket->m_mutex.lock();
   assert(p_hash_table.relaxed() == p_table);

   return p_bucket;
}

template <is_predicate Validate, is_invocable BeforeSleep>
auto
park(uword key, Validate&& validate, BeforeSleep&& before_sleep)
   -> park_result {
   thread_data& data = local_thread_data;
   bucket* const p_bucket = lock_bucket(key);
   if (!$fwd(validate)()) {
      p_bucket->m_mutex.unlock();
      return park_result{
         .m_status = park_status::invalid,
         .m_token = unpark_token::normal,
      };
   }

   data.m_p_next_in_queue = nullptr;
   data.m_key.relaxed() = key;
   data.m_park_token = 0;
   data.m_parked_with_timeout = false;
   data.m_thread_parker.prepare_park();

   if (p_bucket->m_p_queue_head != nullptr) {
      p_bucket->m_p_queue_tail->m_p_next_in_queue = &data;
   } else {
      p_bucket->m_p_queue_head = &data;
   }

   p_bucket->m_p_queue_tail = &data;
   p_bucket->m_mutex.unlock();

   $fwd(before_sleep)();
   data.m_thread_parker.park();
   return {.m_status = park_status::unparked, .m_token = data.m_unpark};
}

template <
   is_predicate Validate, is_invocable BeforeSleep, is_invocable<bool> TimedOut,
   is_clock Clock, is_duration Duration>
auto
park_until(
   uword key, Validate&& validate, BeforeSleep&& before_sleep,
   TimedOut&& timed_out, time_point<Clock, Duration> timeout
) -> park_result {
   thread_data& data = local_thread_data;
   bucket* const p_bucket = lock_bucket(key);
   if (!$fwd(validate)()) {
      p_bucket->m_mutex.unlock();
      return park_result{
         .m_status = park_status::invalid,
         .m_token = unpark_token::normal,
      };
   }

   data.m_p_next_in_queue = nullptr;
   data.m_key.relaxed() = key;
   data.m_park_token = 0;
   data.m_parked_with_timeout = true;
   data.m_thread_parker.prepare_park();

   if (p_bucket->m_p_queue_head != nullptr) {
      p_bucket->m_p_queue_tail->m_p_next_in_queue = &data;
   } else {
      p_bucket->m_p_queue_head = &data;
   }

   p_bucket->m_p_queue_tail = &data;
   p_bucket->m_mutex.unlock();

   // TODO: This is no-op for the `cat::mutex` but other primitives WILL need
   // it.
   $fwd(before_sleep)();
   if (data.m_thread_parker.park_until(timeout)) {
      return park_result{
         .m_status = park_status::unparked,
         .m_token = data.m_unpark,
      };
   }

   bucket* const p_timeout_bucket = lock_bucket(key);
   thread_data** p_link = &p_timeout_bucket->m_p_queue_head;
   thread_data* p_current = p_timeout_bucket->m_p_queue_head;
   thread_data* p_previous = nullptr;
   while (p_current != nullptr && p_current != &data) {
      p_link = &p_current->m_p_next_in_queue;
      p_previous = p_current;
      p_current = p_current->m_p_next_in_queue;
   }

   if (p_current == nullptr) {
      p_timeout_bucket->m_mutex.unlock();
      return park_result{
         .m_status = park_status::unparked,
         .m_token = data.m_unpark,
      };
   }

   *p_link = data.m_p_next_in_queue;
   if (p_timeout_bucket->m_p_queue_tail == &data) {
      p_timeout_bucket->m_p_queue_tail = p_previous;
   }

   bool have_more_threads = false;
   p_current = p_timeout_bucket->m_p_queue_head;
   while (p_current != nullptr) {
      if (p_current->m_key.relaxed() == key) {
         have_more_threads = true;
         break;
      }
      p_current = p_current->m_p_next_in_queue;
   }

   $fwd(timed_out)(!have_more_threads);
   p_timeout_bucket->m_mutex.unlock();
   return park_result{
      .m_status = park_status::timed_out,
      .m_token = unpark_token::normal,
   };
}

template <typename Callback>
   requires requires(Callback callback, unpark_result const& result) {
               { $fwd(callback)(result) } -> is_convertible<unpark_token>;
            }
auto
unpark_one(uword key, Callback&& callback) -> unpark_result {
   bucket* const p_bucket = lock_bucket(key);
   thread_data** p_link = &p_bucket->m_p_queue_head;
   thread_data* p_current = p_bucket->m_p_queue_head;
   thread_data* p_previous = nullptr;
   unpark_result result{};

   while (p_current != nullptr) {
      if (p_current->m_key.relaxed() == key) {
         thread_data* const p_next = p_current->m_p_next_in_queue;
         *p_link = p_next;

         if (p_bucket->m_p_queue_tail == p_current) {
            p_bucket->m_p_queue_tail = p_previous;
         } else {
            thread_data* p_scan = p_next;
            while (p_scan != nullptr) {
               if (p_scan->m_key.relaxed() == key) {
                  result.m_have_more_threads = true;
                  break;
               }
               p_scan = p_scan->m_p_next_in_queue;
            }
         }

         result.m_unparked_threads = 1;
         result.m_be_fair = p_bucket->m_timeout.should_timeout();
         result.m_parked_with_timeout = p_current->m_parked_with_timeout;
         p_current->m_unpark = $fwd(callback)(result);
         nix::futex* const p_futex = p_current->m_thread_parker.unpark_lock();
         p_bucket->m_mutex.unlock();
         parker::unpark(p_futex);
         return result;
      }

      p_link = &p_current->m_p_next_in_queue;
      p_previous = p_current;
      p_current = p_current->m_p_next_in_queue;
   }

   // Nothing was unparked, so the callback runs only for its side effects and
   // its token has nowhere to go.
   $fwd(callback)(result);
   p_bucket->m_mutex.unlock();
   return result;
}

}  // namespace cat::detail::mutex_parking_lot
