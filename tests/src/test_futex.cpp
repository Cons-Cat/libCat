#include <cat/linux>

#include "../unit_tests.hpp"

test(futex_syscalls) {
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
   cat::verify(futex_word.m_value.load(cat::memory_order::relaxed) == 0u);

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

   auto const woken =
      nix::sys_futex(&futex_word,
                     nix::make_futex_op(futex_command::wake,
                                        futex_options::private_process),
                     1u, nullptr, nullptr, 0u)
         .verify();
   cat::verify(woken == 0u);

   nix::futex futex_other{};
   // Source word is `0`; `expected_source_value` `1` fails the kernel compare
   // for `futex_command::compare_requeue` (`linux_error::again`).
   auto const cmp_requeue_result =
      futex_word.compare_requeue(1u, 1u, futex_other, 1u);
   cat::verify(!cmp_requeue_result.has_value());
   cat::verify(cmp_requeue_result.error() == nix::linux_error::again);

   nix::robust_list_head head{};
   head.head.p_next = &head.head;
   head.futex_offset = 0u;
   head.p_list_op_pending = nullptr;
   nix::sys_set_robust_list(&head, sizeof(head)).verify();

   nix::robust_list_head* p_retrieved = nullptr;
   cat::idx length{};
   nix::sys_get_robust_list(nix::process_id{0}, &p_retrieved, &length).verify();
   cat::verify(p_retrieved == &head);
   cat::verify(length == sizeof(head));

   // `futex_wait_flags::word_32` | `futex_wait_flags::private_process`. `val`
   // does not match `futex_word.m_value` (`0`), so the kernel returns
   // `linux_error::again` without enqueueing a waiter.
   nix::futex_waitv const waiter{
      .expected_value = 1u,
      .user_space_address = &futex_word,
      .wait_flags =
         futex_wait_flags::word_32 | futex_wait_flags::private_process,
      .kernel_reserved = 0u,
   };

   auto const waitv_result =
      nix::sys_futex_waitv(cat::span{&waiter, 1u}, futex_waitv_call_flags::none,
                           nullptr, cat::int4{});

   cat::verify(!waitv_result.has_value());
   cat::verify(waitv_result.error() == nix::linux_error::again);
}
