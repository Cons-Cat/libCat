#include <cat/allocator_page>
#include <cat/array>
#include <cat/format>
#include <cat/linux>
#include <cat/mutex_native>
#include <cat/signal_guard>
#include <cat/span>
#include <cat/stacktrace>

namespace {

constexpr cat::array handled_signals = {
   nix::signal::segmentation_violation,
   nix::signal::abort,
   nix::signal::interrupt,
   nix::signal::illegal_instruction,
   nix::signal::graceful_termination,
   nix::signal::floating_point_error,
   nix::signal::bus,
   nix::signal::quit,
};

constinit cat::array<nix::sigaction, 32u> previous_actions{};
constinit cat::mutex_native fatal_signal_lock;
constinit thread_local bool is_handling_this_thread = false;

struct user_context {
   cat::uword flags;
   void* _Nullable p_link;
   nix::signal_stack stack;
   cat::array<cat::uword, 23u> general_registers;
};

[[gnu::naked]]
void
return_from_signal() noexcept {
   // `nix::syscall` can't be used unfortunately because this must not introduce
   // a stack frame.
   asm(R"(movl $15, %eax
          syscall)");
}

[[nodiscard]]
auto
signal_name(nix::signal signal) -> cat::str_view {
   switch (signal) {
      case nix::signal::segmentation_violation:
         return "segmentation_violation";
      case nix::signal::illegal_instruction:
         return "illegal_instruction";
      case nix::signal::floating_point_error:
         return "floating_point_error";
      case nix::signal::abort:
         return "abort";
      case nix::signal::bus:
         return "bus";
      case nix::signal::graceful_termination:
         return "graceful_termination";
      case nix::signal::quit:
         return "quit";
      case nix::signal::interrupt:
         return "interrupt";
      default:
         return "unknown";
   }
}

[[nodiscard]]
auto
is_fault(nix::signal signal) -> bool {
   return signal == nix::signal::segmentation_violation
          || signal == nix::signal::bus
          || signal == nix::signal::illegal_instruction
          || signal == nix::signal::floating_point_error;
}

// Abandon the innermost guarded section that catches `signal`, if there is
// one. It resumes into `longjmp()` once the kernel has restored the signal mask
// and left the alternate stack.
[[nodiscard]]
auto
abandon_guarded_section(nix::signal signal, user_context& context) -> bool {
   auto const bit =
      static_cast<cat::signal_set>(1ull << cat::to_underlying(signal));

   for (cat::detail::signal_guard_frame* p_frame = cat::detail::p_signal_guards;
        p_frame != nullptr; p_frame = p_frame->p_previous) {
      if ((p_frame->guarded & bit) == cat::signal_set::none) {
         continue;
      }

      p_frame->info.signal = signal;

      cat::detail::p_signal_guards = p_frame->p_previous;

      // The 16th register here is `%rip`.
      context.general_registers[16u] =
         __builtin_bit_cast(cat::uword, &cat::longjmp);

      // The 8th register here is `%rdi`.
      context.general_registers[8u] =
         __builtin_bit_cast(cat::uword, &p_frame->info.callsite);

      // The 9th register here is `%rsi`.
      context.general_registers[9u] = 1u;
      return true;
   }

   return false;
}

void
raise_to_previous_handler(nix::signal signal) {
   // Reinstall the handlers that were active before `signal_guard` replaced
   // them.
   for (nix::signal handled_signal : handled_signals) {
      auto _ = nix::sys_rt_sigaction(
         handled_signal, &previous_actions[cat::to_underlying(handled_signal)],
         nullptr
      );
   }

   auto _ = nix::sys_tkill(nix::sys_gettid(), signal);
}

}  // namespace

namespace cat::detail {

void
signal_guard_top() noexcept {
   auto const on_signal = [](cat::int4 number,
                             nix::signal_info* _Nonnull p_info,
                             void* _Nullable p_context) {
      auto const signal = static_cast<nix::signal>(number.raw);

      if (
         p_context != nullptr
         && abandon_guarded_section(
            signal, *static_cast<user_context*>(p_context)
         )
      ) {
         return;
      }

      // An unguarded termination request is passed on without a stack trace.
      if (
         signal == nix::signal::interrupt
         || signal == nix::signal::graceful_termination
      ) {
         raise_to_previous_handler(signal);
         return;
      }

      // A fault while printing must not recurse forever.
      if (is_handling_this_thread) {
         raise_to_previous_handler(signal);
         return;
      }

      is_handling_this_thread = true;

      // If another thread is already printing a stack trace, block here until
      // that thread's handler ends the process. This should never unlock!
      fatal_signal_lock.lock();

      // This is a bit hacky. To avoid bringing in the huge `cat::fmt` lookup
      // tables, use its internal components and low-level `cat::to_chars`
      // directly.
      cat::allocator_page allocator;
      cat::detail::format_memory_buffer<cat::allocator_page> buffer{allocator};
      cat::format_context const context{buffer};

      // TODO: Colorize this and make it prettier.
      auto _ = context.append("*** Signal ");
      auto _ = context.append(cat::to_chars(number));
      auto _ = context.append(" (");
      auto _ = context.append(signal_name(signal));
      auto _ = context.append(") ");

      if (is_fault(signal)) {
         auto _ = context.append('(');
         auto _ = cat::formatter<void*>{}.format(
            allocator, p_info->fault_address(), context
         );
         auto _ = context.append(") ");
      }

      auto _ = context.append("received by PID ");
      auto _ = context.append(cat::to_chars(nix::sys_getpid().value));
      auto _ = context.append(" (TID ");
      auto _ = context.append(cat::to_chars(nix::sys_gettid().value));
      auto _ = context.append("), stack trace: ***\n");

      // We need to statically allocate the frames for this stacktrace, and 64
      // is presumed to be adequate.
      // TODO: Should consider this limit carefully.
      constexpr cat::idx max_frames = 64u;
      cat::maybe const maybe_trace =
         cat::stacktrace::current(allocator, 0u, max_frames);

      if (maybe_trace.has_value()) {
         cat::stacktrace const& frames = maybe_trace.value();
         cat::idx first = 0u;

         for (cat::idx i = 0u; i < frames.size(); ++i) {
            if (
               frames[i].native_handle()
               == __builtin_bit_cast(void*, &return_from_signal)
            ) {
               first = i + 1u;
               break;
            }
         }

         cat::array<cat::stacktrace_entry, max_frames + 1u> spliced{};
         cat::idx count = 0u;
         if (p_context != nullptr && first != 0u) {
            spliced[count] = cat::stacktrace_entry(
               __builtin_bit_cast(
                  void*,
                  static_cast<user_context*>(p_context)->general_registers[16u]
               )
            );
            ++count;
         }

         for (cat::idx i = first; i < frames.size(); ++i) {
            spliced[count] = frames[i];
            ++count;
         }

         auto _ =
            cat::formatter<cat::span<cat::stacktrace_entry const>>{}.format(
               allocator,
               cat::span<cat::stacktrace_entry const>(spliced.data(), count),
               context
            );
         auto _ = context.append('\n');
      } else {
         auto _ = context.append("Stack trace is unavailable!\n");
      }

      // A blocking `write` only comes up short if it is interrupted, so this
      // normally loops once.
      cat::str_view remaining(buffer.data(), buffer.size());
      while (!remaining.is_empty()) {
         nix::scaredy_nix<cat::idx> const written =
            nix::sys_write(nix::stderr, remaining);

         if (!written.has_value()) {
            break;
         }

         remaining = remaining.remove_prefix(written.value());
      }

      raise_to_previous_handler(signal);
   };

   // TODO: `on_stack` does nothing until `sys_sigaltstack()` installs an
   // alternate stack for each thread, so a stack overflow is not reported yet.
   nix::sigaction action{};
   action.sigaction_handler = on_signal;
   action.flags = nix::signal_action_flags::siginfo
                  | nix::signal_action_flags::restorer
                  | nix::signal_action_flags::on_stack;
   action.restorer = return_from_signal;

   for (nix::signal signal : handled_signals) {
      nix::sigaction& previous = previous_actions[to_underlying(signal)];
      nix::sys_rt_sigaction(signal, &action, &previous).or_exit();

      // A signal that was ignored, such as `interrupt` in a background job,
      // stays ignored.
      if (previous.is_ignored()) {
         nix::sys_rt_sigaction(signal, &previous, nullptr).or_exit();
      }
   }
}

}  // namespace cat::detail
