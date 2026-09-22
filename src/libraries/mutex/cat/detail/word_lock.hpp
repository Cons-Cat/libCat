// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

// `word_lock` implements the low-level idiom of the same name from `WTF::Lock`
// and `parking_lot::Mutex`.

#include <cat/atomic>
#include <cat/linux>
#include <cat/thread>

namespace cat::detail {

class word_lock {
 public:
   constexpr word_lock() = default;

   word_lock(word_lock const&) = delete;
   auto
   operator=(word_lock const&) -> word_lock& = delete;

   void
   lock() {
      uword expected = 0u;
      if (
         m_state.compare_exchange_weak(
            expected, locked_bit, memory_order::acquire, memory_order::relaxed
         )
      ) {
         return;
      }
      lock_slow();
   }

   void
   unlock() {
      uword const previous = m_state.release().fetch_sub(locked_bit);
      if (
         (previous & queue_locked_bit) != 0u || (previous & queue_mask) == 0u
      ) {
         return;
      }
      unlock_slow();
   }

 private:
   static constexpr uword locked_bit = 1u;
   static constexpr uword queue_locked_bit = 2u;
   static constexpr uword queue_mask = uword::max() - 3;

   struct alignas(8) thread_data {
      nix::futex m_parker{};
      thread_data* m_p_queue_tail = nullptr;
      thread_data* m_p_prev = nullptr;
      thread_data* m_p_next = nullptr;
   };

   static constexpr auto
   is_locked(uword state) -> bool {
      return (state & locked_bit) != 0u;
   }

   static constexpr auto
   is_queue_locked(uword state) -> bool {
      return (state & queue_locked_bit) != 0u;
   }

   static auto
   queue_head(uword state) -> thread_data* {
      uword const address = state & queue_mask;
      return __builtin_bit_cast(thread_data*, address);
   }

   static auto
   with_queue_head(uword state, thread_data* p_head) -> uword {
      return (state & ~queue_mask) | __builtin_bit_cast(uword, p_head);
   }

   void
   prepare_park(thread_data& node) {
      node.m_parker.m_value.relaxed() = 1u;
   }

   void
   park(thread_data& node) {
      while (node.m_parker.m_value.acquire() != 0u) {
         scaredy result = node.m_parker.wait(1u);
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

   void
   unpark(thread_data& node) {
      node.m_parker.m_value.release() = 0u;
      // The waiter may have already observed the store and left, so waking
      // nobody is fine.
      auto _ = node.m_parker.wake();
   }

   [[gnu::noinline]]
   void
   lock_slow() {
      uint1 spin_counter = 0u;
      uword state = m_state.relaxed();
      for (;;) {
         if (!is_locked(state)) {
            if (
               m_state.compare_exchange_weak(
                  state, state | locked_bit, memory_order::acquire,
                  memory_order::relaxed
               )
            ) {
               return;
            }
            continue;
         }

         if (queue_head(state) == nullptr && spin_counter < 10) {
            spin_counter = spin_counter + 1;
            if (spin_counter <= 3) {
               machine_pause(uint1(1) << spin_counter);
            } else {
               this_thread::yield();
            }
            state = m_state.relaxed();
            continue;
         }

         thread_data node;
         prepare_park(node);
         thread_data* const p_head = queue_head(state);
         if (p_head == nullptr) {
            node.m_p_queue_tail = &node;
            node.m_p_prev = nullptr;
            node.m_p_next = nullptr;
         } else {
            node.m_p_queue_tail = nullptr;
            node.m_p_prev = nullptr;
            node.m_p_next = p_head;
         }

         if (
            m_state.compare_exchange_weak(
               state, with_queue_head(state, &node), memory_order::acq_rel,
               memory_order::relaxed
            )
         ) {
            park(node);
            spin_counter = 0;
            state = m_state.relaxed();
         }
      }
   }

   [[gnu::noinline]]
   void
   unlock_slow() {
      uword state = m_state.relaxed();
      for (;;) {
         if (is_queue_locked(state) || queue_head(state) == nullptr) {
            return;
         }
         if (
            m_state.compare_exchange_weak(
               state, state | queue_locked_bit, memory_order::acquire,
               memory_order::relaxed
            )
         ) {
            break;
         }
      }

      bool rescan = true;
      while (rescan) {
         rescan = false;
         thread_data* const p_head = queue_head(state);
         thread_data* p_current = p_head;
         thread_data* p_tail = p_current->m_p_queue_tail;
         while (p_tail == nullptr) {
            thread_data* const p_next = p_current->m_p_next;
            p_next->m_p_prev = p_current;
            p_current = p_next;
            p_tail = p_current->m_p_queue_tail;
         }
         p_head->m_p_queue_tail = p_tail;

         if (is_locked(state)) {
            if (
               m_state.release().compare_exchange_weak(
                  state, state & ~queue_locked_bit
               )
            ) {
               return;
            }
            thread_fence(memory_order::acquire);
            // The queue is still owned here, so scan it again.
            rescan = true;
            continue;
         }

         thread_data* const p_new_tail = p_tail->m_p_prev;
         if (p_new_tail == nullptr) {
            for (;;) {
               if (
                  m_state.release().compare_exchange_weak(
                     state, state & locked_bit
                  )
               ) {
                  break;
               }
               if (queue_head(state) == nullptr) {
                  continue;
               }
               thread_fence(memory_order::acquire);
               rescan = true;
               break;
            }
         } else {
            p_head->m_p_queue_tail = p_new_tail;
            // The queue stays non-empty here, so the previous state cannot
            // change what happens next.
            auto _ = m_state.release().fetch_and(~queue_locked_bit);
         }

         if (rescan) {
            continue;
         }
         unpark(*p_tail);
         return;
      }
   }

   atomic<uword> m_state{0u};
};

}  // namespace cat::detail
