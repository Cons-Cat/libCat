#include <cat/allocator_linear>
#include <cat/array>
#include <cat/atomic>
#include <cat/linux>
#include <cat/runtime>
#include <cat/signal_guard>
#include <cat/thread>

#include "../unit_tests.hpp"

namespace {
[[gnu::noinline, gnu::no_sanitize_address, gnu::no_sanitize("undefined")]]
auto
fault_null() -> int4 {
   int volatile* p_null = nullptr;
   asm(""
       : "+r"(p_null));
   *p_null = 1;
   return 0;
}

[[gnu::noinline, gnu::no_sanitize_address, gnu::no_sanitize("undefined")]]
void
divide_by_zero() {
   int dividend = 1;
   int divisor = 0;
   asm(""
       : "+r"(dividend), "+r"(divisor));
   int const quotient = dividend / divisor;
   asm volatile(""
                :
                : "r"(quotient));
}

// Overflows the stack. The addition after the call prevents a tail call.
[[gnu::noinline, gnu::no_sanitize_address, gnu::no_sanitize("undefined")]]
auto
recurse(int4 depth) -> int4 {  // NOLINT
   cat::array<int4, 64u> padding{};
   padding[0u] = depth;
   asm volatile(""
                :
                : "r"(padding.data())
                : "memory");
   if (depth < 0) {
      return 0;
   }
   return recurse(depth + 1) + padding[0u];
}

auto
recover(cat::raised_signal_info const* p_info) -> int4 {
   return -int4(cat::to_underlying(p_info->signal));
}

void
send_to_self(nix::signal signal) {
   auto _ = nix::sys_tkill(nix::sys_gettid(), signal);
}

auto
wait_for_child(int8 child) -> uint4 {
   int4 status = 0;
   auto const waited =
      nix::syscall_volatile<int8>(61, child, &status, 0, nullptr).verify();
   cat::verify(waited == child);
   return uint4(status);
}

// Run `p_action` in a child process, so that process-wide signal
// dispositions it changes cannot leak into other tests. Returns its `wait()`
// status, which is 0 when `p_action` returns.
auto
run_in_child(void (*p_action)()) -> uint4 {
   auto const child = nix::syscall_volatile<int8>(57).verify();
   if (child == 0) {
      p_action();
      cat::exit(0);
   }
   return wait_for_child(child);
}

auto
exit_code(uint4 status) -> uint4 {
   cat::verify((status & 0x7fu) == 0u);
   return status >> 8u;
}

auto
terminating_signal(uint4 status) -> uint4 {
   return status & 0x7fu;
}

auto
current_handler(nix::signal signal) -> uword {
   nix::sigaction action{};
   nix::sys_rt_sigaction(signal, nullptr, &action).verify();
   return __builtin_bit_cast(uword, action.handler);
}

auto
mask_of(nix::signal signal) -> nix::signals_mask_set {
   nix::signals_mask_set mask{};
   mask.bytes[(cat::to_underlying(signal) - 1u) / 8u] =
      static_cast<unsigned char>(
         1u << ((cat::to_underlying(signal) - 1u) % 8u)
      );
   return mask;
}

// Handlers installed without libCat need their own restorer on x86-64.
extern "C" [[gnu::naked]]
void
test_signal_restorer() {
   asm volatile(R"(movl $15, %%eax
                   syscall
                )"
                :
                :
                : "memory");
}

void
set_raw_action(nix::signal signal, nix::sigaction action) {
   action.flags = action.flags | nix::signal_action_flags::restorer;
   action.restorer = test_signal_restorer;
   nix::sys_rt_sigaction(signal, &action, nullptr).verify();
}

inline int4 raw_calls = 0;
inline int4 raw_last_signal = 0;
inline int4 raw_last_code = 0;
inline bool raw_last_had_context = false;

void
count_with_info(
   int4 number, nix::signal_info* _Nonnull p_info, void* _Nullable p_context
) {
   ++raw_calls;
   raw_last_signal = number;
   raw_last_code = p_info->code;
   raw_last_had_context = p_context != nullptr;
}

void
count_plain(int4 number) {
   ++raw_calls;
   raw_last_signal = number;
}

auto
info_action() -> nix::sigaction {
   nix::sigaction action{};
   action.sigaction_handler = count_with_info;
   action.flags = nix::signal_action_flags::siginfo;
   return action;
}

auto
plain_action() -> nix::sigaction {
   nix::sigaction action{};
   action.handler = count_plain;
   return action;
}

auto
ignore_action() -> nix::sigaction {
   nix::sigaction action{};
   action.handler = __builtin_bit_cast(nix::sigaction::handler_fn, 1ul);
   return action;
}

// Sanitizers install their own fault handler, which libCat chains to.
void
use_default_fault_handler() {
   set_raw_action(nix::signal::segmentation_violation, nix::sigaction{});
}

struct counting_decider {
   int4* _Nonnull p_calls;

   auto
   operator()(cat::raised_signal_info* /*unused*/) const -> bool {
      ++*p_calls;
      return true;
   }
};

cat::byte* p_protected_page = nullptr;
}  // namespace

// `signal_guard()` is `noexcept` exactly when its callback and handler are.
static_assert(noexcept(cat::signal_guard(
   cat::signal_set::segmentation_fault,
   [] noexcept -> int4 {
      return 1;
   },
   [](cat::raised_signal_info const*) noexcept -> int4 {
      return 2;
   }
)));
static_assert(!noexcept(cat::signal_guard(
   cat::signal_set::segmentation_fault, fault_null,
   [](cat::raised_signal_info const*) noexcept -> int4 {
      return 2;
   }
)));
static_assert(
   !noexcept(cat::signal_guard(cat::signal_set::segmentation_fault, fault_null))
);

$test(signal_guard_recovers_from_faults) {
   // Repeat to show the handler and signal mask survive recovery.
   for (int4 i = 0; i < 3; ++i) {
      int4 const result = cat::signal_guard(
         cat::signal_set::segmentation_fault, fault_null,
         [](cat::raised_signal_info const* p_info) -> int4 {
            cat::verify(p_info->signal == cat::signal_kind::segmentation_fault);
            return -1;
         }
      );
      cat::verify(result == -1);
   }

   bool recovered = false;
   cat::signal_guard(
      cat::signal_set::floating_point_error, divide_by_zero,
      [&](cat::raised_signal_info const* p_info) {
         cat::verify(p_info->signal == cat::signal_kind::floating_point_error);
         recovered = true;
      }
   );
   cat::verify(recovered);
}

$test(signal_guard_reports_signal_details) {
   static constexpr cat::uword bytes = cat::page_size;
   p_protected_page =
      nix::sys_mmap(
         nullptr, bytes, nix::memory_protection_flags::none,
         nix::memory_flags::privately | nix::memory_flags::anonymous,
         nix::invalid_file_descriptor, 0
      )
         .verify();

   // A fault describes its address, and the decider sees its context.
   bool decided = false;
   int4 const faulted = cat::signal_guard(
      cat::signal_set::segmentation_fault,
      [] -> int4 {
         return *reinterpret_cast<int volatile*>(p_protected_page);
      },
      [](cat::raised_signal_info const* p_info) -> int4 {
         cat::verify(p_info->signal == cat::signal_kind::segmentation_fault);
         cat::verify(p_info->p_address == p_protected_page);
         cat::verify(p_info->error_code == 0);
         cat::verify(p_info->p_raw_info != nullptr);
         cat::verify(
            static_cast<nix::signal_info const*>(p_info->p_raw_info)
               ->signal_number
            == 11
         );
         // The context is gone once the callback is abandoned.
         cat::verify(p_info->p_raw_context == nullptr);
         return 1;
      },
      [&](cat::raised_signal_info* p_info) -> bool {
         cat::verify(p_info->p_raw_context != nullptr);
         cat::verify(p_info->p_address == p_protected_page);
         decided = true;
         return false;
      }
   );
   cat::verify(faulted == 1);
   cat::verify(decided);
   nix::sys_munmap(p_protected_page, bytes).verify();

   // A sent signal has no fault address.
   int4 const sent = cat::signal_guard(
      cat::signal_set::interrupt,
      [] -> int4 {
         send_to_self(nix::signal::interrupt);
         return 0;
      },
      [](cat::raised_signal_info const* p_info) -> int4 {
         cat::verify(p_info->p_address == nullptr);
         // `SI_TKILL`.
         cat::verify(
            static_cast<nix::signal_info const*>(p_info->p_raw_info)->code == -6
         );
         return 1;
      }
   );
   cat::verify(sent == 1);

   // A C++ signal has no platform information.
   int4 const terminated = cat::signal_guard(
      cat::signal_set::termination,
      [] -> int4 {
         cat::terminate();
      },
      [](cat::raised_signal_info const* p_info) -> int4 {
         cat::verify(p_info->signal == cat::signal_kind::termination);
         cat::verify(p_info->p_address == nullptr);
         cat::verify(p_info->p_raw_info == nullptr);
         cat::verify(p_info->p_raw_context == nullptr);
         return 1;
      }
   );
   cat::verify(terminated == 1);
}

$test(signal_guard_guards_several_signals) {
   constexpr cat::signal_set both = cat::signal_set::segmentation_fault
                                    | cat::signal_set::floating_point_error;
   cat::verify(cat::signal_guard(both, fault_null, recover) == -11);
   cat::verify(
      cat::signal_guard(
         both,
         [] -> int4 {
            divide_by_zero();
            return 0;
         },
         recover
      )
      == -8
   );
}

$test(signal_guard_passes_arguments_and_results) {
   cat::verify(
      cat::signal_guard(
         cat::signal_set::segmentation_fault,
         [](int4 left, int4 right) -> int4 {
            return left + right;
         },
         3, 4
      )
      == 7
   );

   // Lvalue arguments are forwarded as references.
   int4 count = 0;
   cat::signal_guard(
      cat::signal_set::segmentation_fault,
      [](int4& counter) {
         ++counter;
      },
      count
   );
   cat::verify(count == 1);
}

$test(signal_guard_decider_resumes) {
   static constexpr cat::uword bytes = cat::page_size;
   p_protected_page =
      nix::sys_mmap(
         nullptr, bytes, nix::memory_protection_flags::none,
         nix::memory_flags::privately | nix::memory_flags::anonymous,
         nix::invalid_file_descriptor, 0
      )
         .verify();

   int4 const result = cat::signal_guard(
      cat::signal_set::segmentation_fault,
      [] -> int4 {
         *reinterpret_cast<int volatile*>(p_protected_page) = 9;
         return *reinterpret_cast<int volatile*>(p_protected_page);
      },
      recover,
      // Make the page writable, then resume the faulting store.
      [](cat::raised_signal_info* p_info) -> bool {
         if (p_info->p_address != p_protected_page) {
            return false;
         }
         return nix::sys_mprotect(
                   p_protected_page, bytes,
                   nix::memory_protection_flags::read_write
         )
            .has_value();
      }
   );
   cat::verify(result == 9);
   nix::sys_munmap(p_protected_page, bytes).verify();

   // A decider may also claim raised C++ signals.
   int4 decisions = 0;
   int4 const resumed = cat::signal_guard(
      cat::signal_set::termination,
      [] -> int4 {
         cat::verify(cat::thread_raise_signal(cat::signal_kind::termination));
         return 4;
      },
      recover,
      [&](cat::raised_signal_info* p_info) -> bool {
         cat::verify(p_info->signal == cat::signal_kind::termination);
         ++decisions;
         return true;
      }
   );
   cat::verify(resumed == 4);
   cat::verify(decisions == 1);
}

$test(signal_guard_nests) {
   // The inner guard does not guard this signal, so the outer one recovers.
   int4 const outer = cat::signal_guard(
      cat::signal_set::segmentation_fault,
      [] -> int4 {
         return cat::signal_guard(
            cat::signal_set::floating_point_error, fault_null, recover
         );
      },
      [](cat::raised_signal_info const*) -> int4 {
         return 5;
      }
   );
   cat::verify(outer == 5);

   // Once the inner guard recovers, the outer guard is active again.
   int4 const inner = cat::signal_guard(
      cat::signal_set::segmentation_fault,
      [] -> int4 {
         int4 const first = cat::signal_guard(
            cat::signal_set::segmentation_fault, fault_null, recover
         );
         cat::verify(first == -11);
         return fault_null();
      },
      [](cat::raised_signal_info const*) -> int4 {
         return 6;
      }
   );
   cat::verify(inner == 6);

   // A handler runs without its own guard, so its fault reaches the outer one.
   int4 const handled = cat::signal_guard(
      cat::signal_set::segmentation_fault,
      [] -> int4 {
         return cat::signal_guard(
            cat::signal_set::segmentation_fault, fault_null,
            [](cat::raised_signal_info const*) -> int4 {
               return fault_null();
            }
         );
      },
      [](cat::raised_signal_info const*) -> int4 {
         return 7;
      }
   );
   cat::verify(handled == 7);
}

$test(signal_guard_pops_its_guard) {
   // Returning removes the guard.
   cat::signal_guard(
      cat::signal_set::termination,
      [] {
      },
      [](cat::raised_signal_info const*) {
         cat::verify(false);
      }
   );
   cat::verify(!cat::thread_raise_signal(cat::signal_kind::termination));

   // So does abandoning the callback, before the handler is called.
   cat::signal_guard(
      cat::signal_set::termination,
      [] {
         cat::terminate();
      },
      [](cat::raised_signal_info const*) {
         cat::verify(!cat::thread_raise_signal(cat::signal_kind::termination));
      }
   );
   cat::verify(!cat::thread_raise_signal(cat::signal_kind::termination));

#if __has_feature(cxx_exceptions)
   // So does an exception escaping the callback.
   bool caught = false;
   try {
      cat::signal_guard(
         cat::signal_set::termination,
         [] {
            throw int4(1);
         },
         [](cat::raised_signal_info const*) {
            cat::verify(false);
         }
      );
   } catch (int4 value) {
      cat::verify(value == 1);
      caught = true;
   }
   cat::verify(caught);
   cat::verify(!cat::thread_raise_signal(cat::signal_kind::termination));
#endif
}

$test(signal_guard_sent_signals) {
   // Delivered by the kernel.
   int4 const delivered = cat::signal_guard(
      cat::signal_set::interrupt,
      [] -> int4 {
         send_to_self(nix::signal::interrupt);
         return 0;
      },
      recover
   );
   cat::verify(delivered == -2);

   // Raised in-process.
   int4 const raised = cat::signal_guard(
      cat::signal_set::interrupt,
      [] -> int4 {
         auto _ = cat::thread_raise_signal(cat::signal_kind::interrupt);
         return 0;
      },
      recover
   );
   cat::verify(raised == -2);
}

$test(signal_guard_broken_pipe) {
   nix::file_descriptor ends[2] = {};
   nix::sys_pipe2(ends, nix::pipe2_flags::close_exec).verify();
   nix::sys_close(ends[0]).verify();
   int4 const result = cat::signal_guard(
      cat::signal_set::broken_pipe,
      [&] -> int4 {
         auto _ = nix::sys_write(ends[1], "x", 1u);
         return 0;
      },
      recover
   );
   cat::verify(result == -13);
   nix::sys_close(ends[1]).verify();
}

$test(signal_guard_termination) {
   int4 const result = cat::signal_guard(
      cat::signal_set::termination,
      [] -> int4 {
         cat::terminate();
      },
      recover
   );
   cat::verify(result == -33);
}

$test(signal_guard_recovers_from_stack_overflow) {
   // Handlers run on an alternate stack, when one is set.
   uint4 const status = run_in_child([] {
      static constexpr cat::uword bytes = 65'536u;
      cat::byte* const p_stack =
         nix::sys_mmap(
            nullptr, bytes, nix::memory_protection_flags::read_write,
            nix::memory_flags::privately | nix::memory_flags::anonymous,
            nix::invalid_file_descriptor, 0
         )
            .verify();
      nix::signal_stack const stack{
         .p_stack = p_stack,
         .flags = nix::signal_stack_flags::none,
         .size = bytes,
      };
      nix::sys_sigaltstack(&stack, nullptr).verify();
      int4 const result = cat::signal_guard(
         cat::signal_set::segmentation_fault,
         [] -> int4 {
            return recurse(0);
         },
         recover
      );
      cat::verify(result == -11);
   });
   cat::verify(status == 0u);
}

$test(signal_guard_threads) {
   // Guards on several threads recover independently.
   cat::atomic<int4> recovered{0};
   auto const worker = [&] {
      for (idx i = 0u; i < 50u; ++i) {
         if (
            cat::signal_guard(
               cat::signal_set::segmentation_fault
                  | cat::signal_set::floating_point_error,
               fault_null, recover
            )
            == -11
         ) {
            ++recovered;
         }
      }
   };
   cat::thread first;
   first.spawn(g_tests_alloc, 256_uki, worker).verify();
   cat::thread second;
   second.spawn(g_tests_alloc, 256_uki, worker).verify();
   worker();
   first.join().verify();
   second.join().verify();
   first.free(g_tests_alloc);
   second.free(g_tests_alloc);
   cat::verify(recovered.relaxed() == 150);

   // A guard only guards the thread that created it.
   cat::atomic<int4> claimed{-1};
   cat::signal_guard(
      cat::signal_set::termination,
      [&] {
         cat::thread other;
         other
            .spawn(
               g_tests_alloc, 256_uki,
               [&] {
                  claimed =
                     cat::thread_raise_signal(cat::signal_kind::termination)
                        ? 1
                        : 0;
               }
            )
            .verify();
         other.join().verify();
         other.free(g_tests_alloc);
      },
      [](cat::raised_signal_info const*) {
         cat::verify(false);
      }
   );
   cat::verify(claimed.relaxed() == 0);
}

$test(signal_guard_info_values) {
   cat::verify(cat::raised_signal_info_value().p_value == nullptr);
   cat::verify(cat::raised_signal_info_value(12).int_value == 12);
   cat::verify(cat::raised_signal_info_value(int4(-5)).int_value == -5);
   cat::verify(cat::raised_signal_info_value(int2(7)).int_value == 7);
   cat::verify(cat::raised_signal_info_value(uint2(9u)).int_value == 9);
   int4 object = 0;
   cat::verify(cat::raised_signal_info_value(&object).p_value == &object);
}

$test(signal_guard_global_decider) {
   cat::verify(!cat::thread_raise_signal(cat::signal_kind::termination));
   {
      int4 calls = 0;
      cat::signal_guard_global_decider const decider(
         cat::signal_set::termination,
         [&](cat::raised_signal_info* p_info) -> bool {
            ++calls;
            return p_info->value.int_value == 12;
         },
         false, 12
      );
      cat::verify(cat::thread_raise_signal(cat::signal_kind::termination));
      cat::verify(calls == 1);
   }
   cat::verify(!cat::thread_raise_signal(cat::signal_kind::termination));

   int4 object = 0;
   cat::signal_guard_global_decider const pointer_decider(
      cat::signal_set::termination,
      [&](cat::raised_signal_info* p_info) -> bool {
         return p_info->value.p_value == &object;
      },
      false, &object
   );
   cat::verify(cat::thread_raise_signal(cat::signal_kind::termination));
}

$test(signal_guard_global_deciders_run_in_order) {
   cat::array<int4, 4u> order{};
   idx calls = 0u;
   auto const record = [&](int4 id, bool claim) {
      return [&order, &calls, id, claim](cat::raised_signal_info*) -> bool {
         order[calls] = id;
         ++calls;
         return claim;
      };
   };

   cat::signal_guard_global_decider const first(
      cat::signal_set::termination, record(1, false), false
   );
   // `call_first` puts this before `first`.
   cat::signal_guard_global_decider const second(
      cat::signal_set::termination, record(2, false), true
   );
   cat::signal_guard_global_decider const third(
      cat::signal_set::termination, record(3, true), false
   );
   // `third` claims the signal first, so this is never called.
   cat::signal_guard_global_decider const fourth(
      cat::signal_set::termination, record(4, true), false
   );
   // This does not guard the raised signal.
   cat::signal_guard_global_decider const unrelated(
      cat::signal_set::out_of_memory, record(5, true), true
   );

   cat::verify(cat::thread_raise_signal(cat::signal_kind::termination));
   cat::verify(calls == 3u);
   cat::verify(order[0u] == 2);
   cat::verify(order[1u] == 1);
   cat::verify(order[2u] == 3);

   // A thread-local guard that claims a signal hides it from global deciders.
   calls = 0u;
   cat::signal_guard(
      cat::signal_set::termination,
      [] {
         cat::verify(cat::thread_raise_signal(cat::signal_kind::termination));
      },
      [](cat::raised_signal_info const*) {
         cat::verify(false);
      },
      [](cat::raised_signal_info*) noexcept -> bool {
         return true;
      }
   );
   cat::verify(calls == 0u);

   // A thread-local guard for other signals does not.
   cat::signal_guard(
      cat::signal_set::segmentation_fault,
      [] {
         cat::verify(cat::thread_raise_signal(cat::signal_kind::termination));
      },
      [](cat::raised_signal_info const*) {
         cat::verify(false);
      }
   );
   cat::verify(calls == 3u);
}

$test(signal_guard_global_decider_resumes_faults) {
   static constexpr cat::uword bytes = cat::page_size;
   p_protected_page =
      nix::sys_mmap(
         nullptr, bytes, nix::memory_protection_flags::none,
         nix::memory_flags::privately | nix::memory_flags::anonymous,
         nix::invalid_file_descriptor, 0
      )
         .verify();
   {
      cat::signal_guard_global_decider const decider(
         cat::signal_set::segmentation_fault,
         [](cat::raised_signal_info* p_info) -> bool {
            if (p_info->p_address != p_protected_page) {
               return false;
            }
            return nix::sys_mprotect(
                      p_protected_page, bytes,
                      nix::memory_protection_flags::read_write
            )
               .has_value();
         },
         false
      );
      // No thread-local guard exists here.
      *reinterpret_cast<int volatile*>(p_protected_page) = 3;
      cat::verify(*reinterpret_cast<int volatile*>(p_protected_page) == 3);
   }
   nix::sys_munmap(p_protected_page, bytes).verify();
}

$test(signal_guard_global_decider_moves) {
   int4 first_calls = 0;
   int4 second_calls = 0;
   {
      cat::signal_guard_global_decider<counting_decider> original(
         cat::signal_set::termination, counting_decider{&first_calls}, false
      );
      cat::signal_guard_global_decider<counting_decider> moved(
         cat::move(original)
      );
      cat::verify(cat::thread_raise_signal(cat::signal_kind::termination));
      cat::verify(first_calls == 1);

      {
         cat::signal_guard_global_decider<counting_decider> other(
            cat::signal_set::termination, counting_decider{&second_calls}, false
         );
         moved = cat::move(other);
         // Only `other`'s decider remains registered.
         cat::verify(cat::thread_raise_signal(cat::signal_kind::termination));
         cat::verify(first_calls == 1);
         cat::verify(second_calls == 1);
      }
      // Destroying the moved-from `other` left `moved` registered.
      cat::verify(cat::thread_raise_signal(cat::signal_kind::termination));
      cat::verify(second_calls == 2);

      auto& alias = moved;
      moved = cat::move(alias);
      cat::verify(cat::thread_raise_signal(cat::signal_kind::termination));
      cat::verify(second_calls == 3);
   }
   cat::verify(!cat::thread_raise_signal(cat::signal_kind::termination));
   cat::verify(first_calls == 1);
   cat::verify(second_calls == 3);
}

$test(signal_guard_install_counts_references) {
   uint4 const status = run_in_child([] {
      uword const initial_pipe = current_handler(nix::signal::broken_pipe);
      uword const initial_interrupt = current_handler(nix::signal::interrupt);
      {
         cat::signal_guard_install first(cat::signal_set::broken_pipe);
         uword const installed = current_handler(nix::signal::broken_pipe);
         cat::verify(installed != initial_pipe);
         {
            cat::signal_guard_install const second(
               cat::signal_set::broken_pipe
            );
         }
         cat::verify(current_handler(nix::signal::broken_pipe) == installed);
         {
            cat::signal_guard_install const moved(cat::move(first));
            cat::verify(current_handler(nix::signal::broken_pipe) == installed);
         }
         // `moved` held the last reference.
         cat::verify(current_handler(nix::signal::broken_pipe) == initial_pipe);
      }
      cat::verify(current_handler(nix::signal::broken_pipe) == initial_pipe);

      // Move assignment exchanges the guarded signals.
      {
         cat::signal_guard_install pipe_install(cat::signal_set::broken_pipe);
         {
            cat::signal_guard_install interrupt_install(
               cat::signal_set::interrupt
            );
            pipe_install = cat::move(interrupt_install);
         }
         cat::verify(current_handler(nix::signal::broken_pipe) == initial_pipe);
         cat::verify(
            current_handler(nix::signal::interrupt) != initial_interrupt
         );

         auto& alias = pipe_install;
         pipe_install = cat::move(alias);
         cat::verify(
            current_handler(nix::signal::interrupt) != initial_interrupt
         );
      }
      cat::verify(current_handler(nix::signal::interrupt) == initial_interrupt);

      // C++ signals have no platform handler to install.
      cat::signal_guard_install const cpp_signals(
         cat::signal_set::termination | cat::signal_set::out_of_memory
      );
   });
   cat::verify(status == 0u);
}

$test(signal_guard_install_detects_replaced_handlers) {
   uint4 const status = run_in_child([] {
      cat::signal_guard_install const install(cat::signal_set::broken_pipe);
      // Replacing libCat's handler makes the last uninstall terminate.
      set_raw_action(nix::signal::broken_pipe, plain_action());
   });
   cat::verify(exit_code(status) == 134u);
}

$test(signal_guard_chains_to_previous_handlers) {
   uint4 const status = run_in_child([] {
      set_raw_action(nix::signal::broken_pipe, info_action());
      {
         cat::signal_guard_install const install(cat::signal_set::broken_pipe);

         // Sent and unclaimed.
         send_to_self(nix::signal::broken_pipe);
         cat::verify(raw_calls == 1);
         cat::verify(raw_last_signal == 13);

         // Raised in-process, with synthesized information.
         cat::verify(cat::thread_raise_signal(cat::signal_kind::broken_pipe));
         cat::verify(raw_calls == 2);
         cat::verify(raw_last_code == -6);
         // Handlers commonly read the context, so it must be genuine.
         cat::verify(raw_last_had_context);

         // Raised in-process, with the caller's information.
         nix::signal_info information{};
         information.signal_number = 13;
         // `SI_QUEUE`.
         information.code = -1;
         cat::verify(
            cat::thread_raise_signal(
               cat::signal_kind::broken_pipe, &information
            )
         );
         cat::verify(raw_calls == 3);
         cat::verify(raw_last_code == -1);
         cat::verify(raw_last_had_context);

         // A guard for other signals does not claim it.
         cat::signal_guard(
            cat::signal_set::segmentation_fault,
            [] {
               send_to_self(nix::signal::broken_pipe);
            },
            [](cat::raised_signal_info const*) {
               cat::verify(false);
            }
         );
         cat::verify(raw_calls == 4);

         // A guard for it does.
         int4 const result = cat::signal_guard(
            cat::signal_set::broken_pipe,
            [] -> int4 {
               send_to_self(nix::signal::broken_pipe);
               return 0;
            },
            recover
         );
         cat::verify(result == -13);
         cat::verify(raw_calls == 4);
      }

      // The last uninstall restores the previous handler.
      cat::verify(
         current_handler(nix::signal::broken_pipe)
         == __builtin_bit_cast(uword, &count_with_info)
      );
      send_to_self(nix::signal::broken_pipe);
      cat::verify(raw_calls == 5);

      // Handlers without `SA_SIGINFO` are called with only the signal number.
      set_raw_action(nix::signal::broken_pipe, plain_action());
      {
         cat::signal_guard_install const install(cat::signal_set::broken_pipe);
         send_to_self(nix::signal::broken_pipe);
         cat::verify(raw_calls == 6);
         cat::verify(cat::thread_raise_signal(cat::signal_kind::broken_pipe));
         cat::verify(raw_calls == 7);
      }
   });
   cat::verify(status == 0u);
}

$test(signal_guard_ignored_signals) {
   uint4 const status = run_in_child([] {
      set_raw_action(nix::signal::broken_pipe, ignore_action());
      cat::verify(!cat::thread_raise_signal(cat::signal_kind::broken_pipe));
      {
         cat::signal_guard_install const install(cat::signal_set::broken_pipe);
         // libCat's handler is called, and defers to `SIG_IGN`.
         cat::verify(cat::thread_raise_signal(cat::signal_kind::broken_pipe));
         send_to_self(nix::signal::broken_pipe);
      }
      cat::verify(current_handler(nix::signal::broken_pipe) == 1u);
      cat::verify(!cat::thread_raise_signal(cat::signal_kind::broken_pipe));
   });
   cat::verify(status == 0u);
}

$test(signal_guard_raises_default_dispositions) {
   // A defaulted signal is unblocked and sent to the calling thread.
   uint4 status = run_in_child([] {
      nix::signals_mask_set const mask = mask_of(nix::signal::interrupt);
      nix::sys_rt_sigprocmask(nix::signal_action::block, &mask, nullptr)
         .verify();
      auto _ = cat::thread_raise_signal(cat::signal_kind::interrupt);
   });
   cat::verify(terminating_signal(status) == 2u);

   // An unclaimed signal reaches the default disposition through libCat.
   status = run_in_child([] {
      cat::signal_guard_install const install(cat::signal_set::interrupt);
      auto _ = cat::thread_raise_signal(cat::signal_kind::interrupt);
   });
   cat::verify(terminating_signal(status) == 2u);
}

$test(signal_guard_handler_reraises) {
   // P2069's example re-raises unrecognized signals from the handler.
   uint4 const status = run_in_child([] {
      use_default_fault_handler();
      auto _ = cat::signal_guard(
         cat::signal_set::segmentation_fault, fault_null,
         [](cat::raised_signal_info const* p_info) -> int4 {
            auto _ = cat::thread_raise_signal(
               p_info->signal, p_info->p_raw_info, p_info->p_raw_context
            );
            return 0;
         }
      );
   });
   cat::verify(terminating_signal(status) == 11u);
}

$test(signal_guard_unclaimed_fault_is_fatal) {
   uint4 const status = run_in_child([] {
      use_default_fault_handler();
      cat::signal_guard_install const install(
         cat::signal_set::segmentation_fault
      );
      auto _ = fault_null();
   });
   cat::verify(terminating_signal(status) == 11u);
}

$test(signal_guard_signal_raised_names) {
   cat::verify(
      cat::signal_raised{cat::signal_kind::segmentation_fault}.what()
      == "segmentation fault"
   );
   cat::verify(
      cat::signal_raised{cat::signal_kind::termination}.what() == "termination"
   );
}

$test(signal_guard_format) {
   cat::span page = g_tests_alloc.alloc_multi<cat::byte>(1_uki).verify();
   $defer {
      g_tests_alloc.free(page);
   };
   auto allocator = make_allocator_linear(page);

   cat::verify(
      cat::fmt(
         allocator, "{}",
         cat::signal_raised{cat::signal_kind::segmentation_fault}
      )
         .verify()
      == "segmentation fault"
   );
   cat::verify(
      cat::fmt(allocator, "{}", cat::signal_kind::broken_pipe).verify()
      == "broken pipe"
   );
   cat::verify(
      cat::fmt(allocator, "{:?}", cat::signal_kind::termination).verify()
      == R"("termination")"
   );
}

#if __has_feature(cxx_exceptions)
$test(signal_guard_default_handler_throws) {
   bool caught = false;
   try {
      auto _ =
         cat::signal_guard(cat::signal_set::segmentation_fault, fault_null);
   } catch (cat::signal_raised const& raised) {
      cat::verify(raised.signal == cat::signal_kind::segmentation_fault);
      caught = true;
   }
   cat::verify(caught);
}

$test(signal_guard_throwing_decider_terminates) {
   // Exceptions cannot propagate out of signal context.
   uint4 const status = run_in_child([] {
      // ASan runs handlers on a small alternate stack, which unwinding a throw
      // there exhausts. The process then dies of `SIGSEGV` instead.
      static constexpr cat::uword bytes = 1'048'576u;
      cat::byte* const p_stack =
         nix::sys_mmap(
            nullptr, bytes, nix::memory_protection_flags::read_write,
            nix::memory_flags::privately | nix::memory_flags::anonymous,
            nix::invalid_file_descriptor, 0
         )
            .verify();
      nix::signal_stack const stack{
         .p_stack = p_stack,
         .flags = nix::signal_stack_flags::none,
         .size = bytes,
      };
      nix::sys_sigaltstack(&stack, nullptr).verify();
      auto _ = cat::signal_guard(
         cat::signal_set::segmentation_fault, fault_null, recover,
         [](cat::raised_signal_info*) -> bool {
            throw int4(1);
         }
      );
   });
   cat::verify(exit_code(status) == 134u);
}
#endif
