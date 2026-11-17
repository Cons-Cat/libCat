#include <cat/array>
#include <cat/chrono>
#include <cat/linux>
#include <cat/thread>

#include "../tsan_tests.hpp"
#include "../unit_tests.hpp"

$test(futex_syscalls) {
   using nix::futex_command;
   using nix::futex_options;
   using nix::futex_wait_flags;
   using nix::futex_waitv_call_flags;

   // Make futex operations using `nix::make_futex_op()`.
   auto const wait_private =
      nix::make_futex_op(futex_command::wait, futex_options::private_process);
   cat::verify(wait_private.command() == futex_command::wait);
   cat::verify(wait_private.options() == futex_options::private_process);

   auto const wake_private =
      nix::make_futex_op(futex_command::wake, futex_options::private_process);
   cat::verify(wake_private.command() == futex_command::wake);
   cat::verify(wake_private.options() == futex_options::private_process);

   // Manually construct a futex operation.
   nix::futex_op assembled_op{};
   assembled_op.set_options(futex_options::private_process);
   assembled_op.set_command(futex_command::wake);
   cat::verify(assembled_op.command() == futex_command::wake);
   cat::verify(assembled_op.options() == futex_options::private_process);
   cat::verify(assembled_op.encoded == wake_private.encoded);

   nix::futex futex_word{};
   cat::verify(futex_word.m_value.order_relaxed().load() == 0u);

   // Robust futexes.
   cat::verify(nix::robust_futex::unlocked() == 0u);
   cat::uint4 const tid{123u};
   cat::uint4 const held = tid;
   cat::verify(nix::robust_futex::decode_owner_thread_id(held) == tid);
   cat::verify(!nix::robust_futex::has_waiters(held));
   cat::uint4 const held_with_waiters = held | nix::futex_waiters_flag;
   cat::verify(nix::robust_futex::has_waiters(held_with_waiters));
   cat::uint4 const owner_died = held | nix::futex_owner_died_flag;
   cat::verify(nix::robust_futex::owner_exited_abnormally(owner_died));

   auto const woken = nix::sys_futex(
                         futex_word,
                         nix::make_futex_op(
                            futex_command::wake, futex_options::private_process
                         ),
                         1u, nullptr, nullptr, 0u
   )
                         .verify();
   cat::verify(woken == 0u);

   nix::futex futex_other{};
   // Source word is 0. `expected_source_value` 1 fails the kernel compare for
   // `futex_command::compare_requeue` (`linux_error::again`).
   auto const cmp_requeue_result =
      futex_word.compare_requeue(1u, 1u, futex_other, 1u);
   cat::verify(cmp_requeue_result.is_empty());
   cat::verify(cmp_requeue_result.error() == nix::linux_error::again);

   nix::robust_list_head head{};
   head.head.p_next = &head.head;
   head.futex_offset = 0u;
   head.p_list_op_pending = nullptr;
   nix::sys_set_robust_list(head, sizeof(head)).verify();

   nix::robust_list_head* p_retrieved = nullptr;
   cat::idx length{};
   nix::sys_get_robust_list(nix::process_id{0}, p_retrieved, length).verify();
   cat::verify(p_retrieved == &head);
   cat::verify(length == sizeof(head));

   // `futex_wait_flags::word_32` | `futex_wait_flags::private_process`. `val`
   // does not match `futex_word.m_value` (0), so the kernel returns
   // `linux_error::again` without enqueueing a waiter.
   nix::futex_waitv const waiter{
      .expected_value = 1u,
      .user_space_address = &futex_word,
      .wait_flags =
         futex_wait_flags::word_32 | futex_wait_flags::private_process,
      .kernel_reserved = 0u,
   };

   auto const waitv_result = nix::sys_futex_waitv(
      cat::span{&waiter, 1u}, futex_waitv_call_flags::none, nullptr, cat::int4{}
   );

   cat::verify(waitv_result.is_empty());
   cat::verify(waitv_result.error() == nix::linux_error::again);
}

// `wake()` wakes one waiter, `wake(count)` wakes `count`, and `wake_all()`
// wakes every remaining waiter.
$test(futex_wake_counts) {
   nix::futex futex_word{};
   cat::array<cat::thread, 5> threads;
   $defer {
      for (cat::thread& thread : threads) {
         thread.free(g_tests_alloc);
      }
   };

   for (cat::thread& thread : threads) {
      thread
         .spawn(
            g_tests_alloc, 16_uki,
            [&] {
               // Retry after a signal interrupts the wait. The word never
               // changes, so only a wake returns successfully.
               while (futex_word.wait(0u).is_empty()) {
               }
            }
         )
         .verify();
   }

   // Requeueing the waiters onto their own word, waking none, counts them
   // without disturbing them.
   auto const count_sleepers = [&] -> cat::idx {
      return futex_word.compare_requeue(0u, 0x7fffffffu, futex_word, 0u)
         .verify();
   };
   while (count_sleepers() != 5u) {
      cat::this_thread::yield();
   }

   cat::verify(futex_word.wake().verify() == 1u);
   cat::verify(futex_word.wake(2).verify() == 2u);
   cat::verify(count_sleepers() == 2u);
   cat::verify(futex_word.wake_all().verify() == 2u);
   cat::verify(count_sleepers() == 0u);

   for (cat::thread& thread : threads) {
      thread.join().verify();
   }
}

namespace {

// Wait on a word that never changes until `deadline`, retrying after signals,
// and verify that it times out no earlier than `deadline`.
template <cat::is_clock Clock>
void
verify_times_out(
   nix::futex& futex_word, typename Clock::time_point const deadline
) {
   for (;;) {
      nix::scaredy_nix<void> const result =
         futex_word.wait_until(0u, Clock::native_handle(), deadline);
      cat::verify(result.is_empty());
      if (result.error() != nix::linux_error::intr) {
         cat::verify(result.error() == nix::linux_error::timedout);
         break;
      }
   }
   cat::verify(Clock::now().verify() >= deadline);
}

}  // namespace

template <typename Timer, typename TimePoint>
concept can_wait_until =
   requires(nix::futex& futex_word, Timer const& timer, TimePoint deadline) {
      futex_word.wait_until(0u, timer, deadline);
   };

// A deadline must be on the timer that is passed with it, and the kernel only
// measures absolute deadlines on `CLOCK_MONOTONIC` and `CLOCK_REALTIME`.
static_assert(can_wait_until<
              nix::clock_timerfd<nix::clock_id::monotonic>,
              cat::clock_steady::time_point>);
static_assert(can_wait_until<
              nix::clock_timerfd<nix::clock_id::realtime>,
              cat::clock_system::time_point>);
static_assert(can_wait_until<
              nix::clock_timerfd<nix::clock_id::monotonic>,
              nix::clock_timerfd<nix::clock_id::monotonic>::time_point>);
static_assert(!can_wait_until<
              nix::clock_timerfd<nix::clock_id::monotonic>,
              cat::clock_system::time_point>);
static_assert(
   !can_wait_until<
      nix::clock_timerfd<nix::clock_id::realtime>, cat::clock_tai::time_point>
);
static_assert(!can_wait_until<
              nix::clock_timerfd<nix::clock_id::boottime>,
              nix::clock_timerfd<nix::clock_id::boottime>::time_point>);

// `wait_until()` passes `CLOCK_MONOTONIC` and `CLOCK_REALTIME` deadlines to the
// kernel.
$test(futex_wait_until) {
   nix::futex futex_word{};

   // Deadlines that have passed time out without sleeping.
   verify_times_out<cat::clock_steady>(
      futex_word, cat::clock_steady::now().verify()
   );
   verify_times_out<cat::clock_system>(
      futex_word, cat::clock_system::now().verify()
   );

   verify_times_out<cat::clock_steady>(
      futex_word,
      cat::clock_steady::now().verify() + 10 * cat::units::millisecond
   );
   verify_times_out<cat::clock_system>(
      futex_word,
      cat::clock_system::now().verify() + 10 * cat::units::millisecond
   );

   // A word that no longer holds `expected` does not wait.
   nix::scaredy_nix<void> const changed = futex_word.wait_until(
      1u, cat::clock_steady::native_handle(),
      cat::clock_steady::now().verify() + 1 * cat::units::second
   );
   cat::verify(changed.is_empty());
   cat::verify(changed.error() == nix::linux_error::again);
}

// A bitset wake only wakes waiters whose bitset intersects its own.
$test(futex_bitset) {
   nix::futex futex_word{};
   nix::futex_bitset const first = static_cast<nix::futex_bitset>(0b01u);
   nix::futex_bitset const second = static_cast<nix::futex_bitset>(0b10u);
   cat::array<cat::thread, 2> threads;
   $defer {
      for (cat::thread& thread : threads) {
         thread.free(g_tests_alloc);
      }
   };

   for (cat::thread& thread : threads) {
      nix::futex_bitset const bitset = &thread == &threads[0] ? first : second;
      thread
         .spawn(
            g_tests_alloc, 16_uki,
            [&futex_word, bitset] {
               // Retry after a signal interrupts the wait. The word never
               // changes, so only a wake returns successfully.
               while (futex_word.wait_bitset(0u, bitset).is_empty()) {
               }
            }
         )
         .verify();
   }

   auto const count_sleepers = [&] -> cat::idx {
      return futex_word.compare_requeue(0u, 0x7fffffffu, futex_word, 0u)
         .verify();
   };
   while (count_sleepers() != 2u) {
      cat::this_thread::yield();
   }

   nix::futex_bitset const neither = static_cast<nix::futex_bitset>(0b100u);
   cat::verify(futex_word.wake_bitset(2, neither).verify() == 0u);
   cat::verify(futex_word.wake_bitset(2, second).verify() == 1u);
   cat::verify(count_sleepers() == 1u);
   cat::verify(
      futex_word.wake_bitset(2, nix::futex_bitset::match_any).verify() == 1u
   );

   for (cat::thread& thread : threads) {
      thread.join().verify();
   }
}

namespace {

// Issue `command` with `options` on a process-private `futex_word`.
auto
futex_operation(
   nix::futex_word& futex_word, nix::futex_command command,
   nix::futex_options options, cat::uint4 value, cat::uint4 value2,
   nix::futex_word* _Nullable p_second_futex, cat::uint4 value3
) -> nix::scaredy_nix<cat::idx> {
   return nix::sys_futex(
      futex_word,
      nix::make_futex_op(
         command, nix::futex_options::private_process | options
      ),
      value, value2, p_second_futex, value3
   );
}

// The error of a failed futex operation, or `0` on success.
auto
futex_error(nix::scaredy_nix<cat::idx> const& result) -> int4 {
   return result.has_value() ? 0 : cat::to_underlying(result.error());
}

}  // namespace

// Each `futex_command` does what Linux documents.
$test(futex_commands) {
   using nix::futex_command;
   using nix::futex_options;
   using nix::linux_error;

   nix::futex futex_word{};
   nix::futex other_word{};

   // `fd` was removed in Linux 2.6.26.
   cat::verify(
      futex_error(futex_operation(
         futex_word, futex_command::fd, futex_options::none, 0u, 0u, nullptr, 0u
      ))
      == cat::to_underlying(linux_error::nosys)
   );

   // `requeue` wakes and requeues nobody when nobody waits.
   cat::verify(
      futex_operation(
         futex_word, futex_command::requeue, futex_options::none, 0u, 1u,
         &other_word, 0u
      )
         .verify()
      == 0u
   );

   // `wake_op` sets the second word to 7 when its old value 5 equals 5. Its
   // operation is encoded as `FUTEX_OP(FUTEX_OP_SET, 7, FUTEX_OP_CMP_EQ, 5)`.
   other_word.m_value.store(5u);
   cat::uint4 const set_7_if_5 = (7u << 12u) | 5u;
   cat::verify(
      futex_operation(
         futex_word, futex_command::wake_op, futex_options::none, 1u, 1u,
         &other_word, set_7_if_5
      )
         .verify()
      == 0u
   );
   cat::verify(other_word.m_value.load() == 7u);

   // The priority-inheritance locks store their owner's thread ID.
   cat::uint4 const thread_id = nix::sys_gettid().value;
   auto const pi = [&](futex_command command) -> int4 {
      return futex_error(futex_operation(
         futex_word, command, futex_options::none, 0u, 0u, nullptr, 0u
      ));
   };
   futex_word.m_value.store(0u);
   cat::verify(pi(futex_command::lock_pi) == 0);
   cat::verify(futex_word.m_value.load() == thread_id);
   cat::verify(
      pi(futex_command::trylock_pi) == cat::to_underlying(linux_error::deadlk)
   );
   cat::verify(pi(futex_command::unlock_pi) == 0);
   cat::verify(futex_word.m_value.load() == 0u);
   cat::verify(pi(futex_command::lock_pi2) == 0);
   cat::verify(futex_word.m_value.load() == thread_id);
   cat::verify(pi(futex_command::unlock_pi) == 0);
   cat::verify(pi(futex_command::trylock_pi) == 0);
   cat::verify(futex_word.m_value.load() == thread_id);
   cat::verify(pi(futex_command::unlock_pi) == 0);

   // `wait_requeue_pi` does not wait on a word that no longer holds its
   // expected value. `compare_requeue_pi` only requeues while the word holds
   // its expected value, and wakes nobody when nobody waits.
   nix::futex pi_word{};
   futex_word.m_value.store(0u);
   cat::verify(
      futex_error(
         nix::sys_futex(
            futex_word,
            nix::make_futex_op(
               futex_command::wait_requeue_pi, futex_options::private_process
            ),
            1u, nullptr, &pi_word, 0u
         )
      )
      == cat::to_underlying(linux_error::again)
   );
   cat::verify(
      futex_operation(
         futex_word, futex_command::compare_requeue_pi, futex_options::none, 1u,
         0u, &pi_word, 0u
      )
         .verify()
      == 0u
   );
   cat::verify(
      futex_error(futex_operation(
         futex_word, futex_command::compare_requeue_pi, futex_options::none, 1u,
         0u, &pi_word, 1u
      ))
      == cat::to_underlying(linux_error::again)
   );
}

// `futex_options::robust_unlock` unlocks the futex word and clears the robust
// list's pending operation pointer that the second address points to.
$test(futex_robust_unlock) {
   using nix::futex_command;
   using nix::futex_options;

   cat::uint4 const locked = nix::sys_gettid().value | nix::futex_waiters_flag;
   nix::futex futex_word{};

   // A 64-bit pending operation pointer.
   futex_word.m_value.store(locked);
   void* p_pending = &futex_word;
   nix::scaredy_nix<cat::idx> const wake = futex_operation(
      futex_word, futex_command::wake, futex_options::robust_unlock, 1u, 0u,
      reinterpret_cast<nix::futex_word*>(&p_pending), 0u
   );
   // Older kernels do not know this option.
   if (wake.is_empty() && wake.error() == nix::linux_error::nosys) {
      return;
   }
   cat::verify(wake.has_value());
   cat::verify(futex_word.m_value.load() == 0u);
   cat::verify(p_pending == nullptr);

   // A 32-bit pending operation pointer.
   futex_word.m_value.store(locked);
   cat::uint4 pending_32 = 0x1234u;
   futex_operation(
      futex_word, futex_command::wake,
      futex_options::robust_unlock | futex_options::robust_list_32, 1u, 0u,
      reinterpret_cast<nix::futex_word*>(&pending_32), 0u
   )
      .verify();
   cat::verify(futex_word.m_value.load() == 0u);
   cat::verify(pending_32 == 0u);

   // A bitset wake.
   futex_word.m_value.store(locked);
   p_pending = &futex_word;
   futex_operation(
      futex_word, futex_command::wake_bitset, futex_options::robust_unlock, 1u,
      0u, reinterpret_cast<nix::futex_word*>(&p_pending),
      cat::to_underlying(nix::futex_bitset::match_any)
   )
      .verify();
   cat::verify(futex_word.m_value.load() == 0u);
   cat::verify(p_pending == nullptr);

   // A priority-inheritance unlock.
   futex_word.m_value.store(0u);
   futex_operation(
      futex_word, futex_command::lock_pi, futex_options::none, 0u, 0u, nullptr,
      0u
   )
      .verify();
   p_pending = &futex_word;
   futex_operation(
      futex_word, futex_command::unlock_pi, futex_options::robust_unlock, 0u,
      0u, reinterpret_cast<nix::futex_word*>(&p_pending), 0u
   )
      .verify();
   cat::verify(futex_word.m_value.load() == 0u);
   cat::verify(p_pending == nullptr);

   // The pending operation pointer is required.
   futex_word.m_value.store(locked);
   nix::scaredy_nix<cat::idx> const missing = futex_operation(
      futex_word, futex_command::wake, futex_options::robust_unlock, 1u, 0u,
      nullptr, 0u
   );
   cat::verify(missing.is_empty());
   cat::verify(missing.error() == nix::linux_error::fault);
}

// Linux implements every `futex_wait_flags` member except the 8, 16, and 64 bit
// word sizes.
$test(futex_wait_flags) {
   using nix::futex_wait_flags;
   using nix::linux_error;

   // A `futex_wait_flags::numa` futex is followed by its node.
   alignas(8) cat::array<nix::futex_word, 2> words{};
   words[1].m_value.store(__builtin_bit_cast(cat::uint4, nix::futex_no_node));

   // Each wait expects a value that the word does not hold, so a supported
   // wait fails with `linux_error::again`.
   auto const waitv = [&](futex_wait_flags flags) -> linux_error {
      nix::futex_waitv const waiter{
         .expected_value = 1u,
         .user_space_address = &words[0],
         .wait_flags = flags | futex_wait_flags::private_process,
         .kernel_reserved = 0u,
      };
      return nix::sys_futex_waitv(
                cat::span{&waiter, 1u}, nix::futex_waitv_call_flags::none,
                nullptr, cat::int4{}
      )
         .error();
   };
   auto const wait = [&](futex_wait_flags flags) -> linux_error {
      return nix::sys_futex_wait(
                words[0], 1u, 0xffffffffu,
                cat::to_underlying(flags | futex_wait_flags::private_process),
                nullptr, cat::to_underlying(nix::clock_id::monotonic)
      )
         .error();
   };

   for (futex_wait_flags const size :
        {futex_wait_flags::word_8, futex_wait_flags::word_16,
         futex_wait_flags::word_64}) {
      cat::verify(waitv(size) == linux_error::inval);
   }
   cat::verify(waitv(futex_wait_flags::word_32) == linux_error::again);

   // Older kernels do not know these flags.
   for (futex_wait_flags const flags :
        {futex_wait_flags::word_32 | futex_wait_flags::numa,
         futex_wait_flags::word_32 | futex_wait_flags::memory_policy,
         futex_wait_flags::word_32
            | futex_wait_flags::numa
            | futex_wait_flags::memory_policy}) {
      linux_error const error = waitv(flags);
      cat::verify(error == linux_error::again || error == linux_error::inval);
   }

   if (nix::has_sys_futex_wait()) {
      cat::verify(wait(futex_wait_flags::word_8) == linux_error::inval);
      cat::verify(wait(futex_wait_flags::word_16) == linux_error::inval);
      cat::verify(wait(futex_wait_flags::word_64) == linux_error::inval);
      cat::verify(wait(futex_wait_flags::word_32) == linux_error::again);
      linux_error const numa =
         wait(futex_wait_flags::word_32 | futex_wait_flags::numa);
      cat::verify(numa == linux_error::again || numa == linux_error::inval);
   }
}

// These verify that ThreadSanitizer reports races, so they only run with it.
#if __has_feature(thread_sanitizer)

namespace {

// Publish `data` by storing to a futex word and waking its waiter. The futex
// syscalls are invisible to ThreadSanitizer, so only the word's memory orders
// can synchronize.
void
publish_through_futex(
   cat::memory_order store_order, cat::memory_order load_order
) {
   alignas(64) int4 data = 0;
   nix::futex futex_word{};

   cat::thread waiter;
   $defer {
      waiter.free(g_tests_alloc);
   };

   waiter
      .spawn(
         g_tests_alloc, tsan_stack_size,
         [&] {
            while (futex_word.m_value.load(load_order) == 0u) {
               // A wake, a signal, or a changed word all return here to
               // re-check the word.
               auto _ = futex_word.wait(0u);
            }
            cat::verify(data == 1);
         }
      )
      .verify();

   data = 1;
   futex_word.m_value.store(1u, store_order);
   auto _ = futex_word.wake();
   waiter.join().verify();
}

}  // namespace

$test(tsan_futex_wake_does_not_synchronize) {
   expect_tsan_report(tsan_report::data_race, [&] {
      publish_through_futex(
         cat::memory_order::relaxed, cat::memory_order::relaxed
      );
   });
}

$test(tsan_futex_word_release_acquire) {
   expect_tsan_report(tsan_report::none, [&] {
      publish_through_futex(
         cat::memory_order::release, cat::memory_order::acquire
      );
   });
}

#endif
