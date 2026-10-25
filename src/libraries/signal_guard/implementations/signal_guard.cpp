#include <cat/array>
#include <cat/atomic>
#include <cat/linux>
#include <cat/mutex_native>
#include <cat/signal_guard>

namespace cat {
namespace {

inline constexpr idx signal_kind_count = 34u;

// Guards installs and the global decider list.

inline constinit mutex_native global_lock;
inline constinit array<uint4, signal_kind_count> install_counts{};
inline constinit detail::signal_global_decider_node* p_global_deciders =
   nullptr;
inline constinit thread_local detail::signal_guard_frame* p_guards = nullptr;

using decider_link = detail::signal_global_decider_node* _Nullable;

auto
load_link(decider_link& link) noexcept(!__has_feature(cxx_exceptions))
   -> decider_link {
   return atomic<decider_link&>(link).load(memory_order::acquire);
}

void
store_link(decider_link& link, decider_link p_node) noexcept(
   !__has_feature(cxx_exceptions)
) {
   atomic<decider_link&>(link).store(p_node, memory_order::release);
}

auto
is_guarded(signal_set guarded, signal_kind signal) noexcept -> bool {
   return (guarded & static_cast<signal_set>(1ull << to_underlying(signal)))
          != signal_set::none;
}

void
install_platform_signal(signal_kind signal) noexcept(
   !__has_feature(cxx_exceptions)
);

void
uninstall_platform_signal(signal_kind signal) noexcept(
   !__has_feature(cxx_exceptions)
);

[[nodiscard]]
auto
is_platform_signal(signal_kind signal) noexcept -> bool;

void
abandon_guarded_section(
   detail::signal_guard_frame& frame, void* _Nullable p_raw_context
) noexcept(!__has_feature(cxx_exceptions));

// Offer `raised` to the innermost thread-local guard for its signal, then to
// the global deciders. Returns `false` if nothing claimed it.
auto
dispatch_signal(raised_signal_info const& raised) noexcept -> bool {
   for (detail::signal_guard_frame* p_frame = p_guards; p_frame != nullptr;
        p_frame = p_frame->p_previous) {
      if (!is_guarded(p_frame->guarded, raised.signal)) {
         continue;
      }
      raised_signal_info& info = p_frame->info;
      info.signal = raised.signal;
      info.error_code = raised.error_code;
      info.p_address = raised.p_address;
      info.p_raw_info = raised.p_raw_info;
      info.p_raw_context = raised.p_raw_context;
      if (p_frame->p_decide(p_frame->p_decider, &info)) {
         return true;
      }
      p_guards = p_frame->p_previous;
      abandon_guarded_section(*p_frame, raised.p_raw_context);
      return true;
   }

   // TODO: Deciders may be removed concurrently with this walk, as P2069
   // permits. Hazard pointers would fix that.
   for (decider_link p_node = load_link(p_global_deciders); p_node != nullptr;
        p_node = load_link(p_node->p_next)) {
      if (!is_guarded(p_node->guarded, raised.signal)) {
         continue;
      }
      raised_signal_info info = raised;
      info.value = p_node->value;
      if (p_node->p_decide(p_node->p_decider, &info)) {
         return true;
      }
   }
   return false;
}

// Linux x86-64.

// Kernel `ucontext_t` prefix.
struct user_context {
   uword flags;
   void* _Nullable p_link;
   nix::signal_stack stack;
   array<uword, 23u> general_registers;
};

inline constexpr idx context_rdi = 8u;
inline constexpr idx context_rsi = 9u;
inline constexpr idx context_rip = 16u;

// The kernel reports `SI_TKILL` for signals sent by `tkill()`.
inline constexpr int4 signal_code_tkill = -6;

inline constinit array<nix::sigaction, 32u> previous_actions{};

// `p_raw_info` points here once its signal frame is gone.
inline constinit thread_local nix::signal_info abandoned_info{};

extern "C" [[gnu::naked]]
void
cat_signal_restorer() noexcept {
   asm volatile(R"(movl $15, %%eax
                   syscall
                )"
                :
                :
                : "memory");
}

auto
is_platform_signal(signal_kind signal) noexcept -> bool {
   return to_underlying(signal) < 32u;
}

auto
is_default_handler(nix::sigaction const& action) noexcept -> bool {
   return __builtin_bit_cast(uword, action.handler) == 0u;
}

auto
is_ignore_handler(nix::sigaction const& action) noexcept -> bool {
   return __builtin_bit_cast(uword, action.handler) == 1u;
}

void
call_handler(
   nix::sigaction const& action, int4 number, nix::signal_info* _Nonnull p_info,
   void* _Nullable p_context
) noexcept {
   if (
      (action.flags & nix::signal_action_flags::siginfo)
      != nix::signal_action_flags::none
   ) {
      action.sigaction_handler(number, p_info, p_context);
   } else {
      action.handler(number);
   }
}

void
send_to_this_thread(nix::signal signal) noexcept(
   !__has_feature(cxx_exceptions)
) {
   auto _ = nix::sys_tkill(nix::sys_gettid(), signal);
}

auto
signal_mask(nix::signal signal) noexcept(!__has_feature(cxx_exceptions))
   -> nix::signals_mask_set {
   nix::signals_mask_set mask{};
   mask.bytes[(to_underlying(signal) - 1u) / 8u] =
      static_cast<unsigned char>(1u << ((to_underlying(signal) - 1u) % 8u));
   return mask;
}

void
on_signal(
   int4 number, nix::signal_info* _Nonnull p_info, void* _Nullable p_context
) noexcept(!__has_feature(cxx_exceptions)) {
   auto const signal = static_cast<signal_kind>(number.raw);
   raised_signal_info raised{};
   raised.signal = signal;
   raised.error_code = p_info->errno_code;
   if (
      signal == signal_kind::segmentation_fault
      || signal == signal_kind::undefined_memory_access
      || signal == signal_kind::illegal_instruction
      || signal == signal_kind::floating_point_error
   ) {
      raised.p_address = p_info->fault_address();
   }
   raised.p_raw_info = p_info;
   raised.p_raw_context = p_context;
   if (dispatch_signal(raised)) {
      return;
   }

   nix::sigaction const& previous = previous_actions[to_underlying(signal)];
   if (is_ignore_handler(previous)) {
      return;
   }
   if (is_default_handler(previous)) {
      // The signal is blocked until this handler returns. A fault then
      // re-executes into the default disposition too.
      nix::sigaction const fallback{};
      auto _ = nix::sys_rt_sigaction(
         static_cast<nix::signal>(number.raw), &fallback, nullptr
      );
      send_to_this_thread(static_cast<nix::signal>(number.raw));
      return;
   }
   call_handler(previous, number, p_info, p_context);
}

void
abandon_guarded_section(
   detail::signal_guard_frame& frame, void* _Nullable p_raw_context
) noexcept(!__has_feature(cxx_exceptions)) {
   frame.info.p_raw_context = nullptr;
   if (frame.info.p_raw_info != nullptr) {
      abandoned_info = *static_cast<nix::signal_info*>(frame.info.p_raw_info);
      frame.info.p_raw_info = &abandoned_info;
   }
   if (p_raw_context == nullptr) {
      longjmp(frame.info.buffer, 1);
   }
   // Resume into `longjmp()` once the kernel has restored the signal mask and
   // left the alternate stack.
   array<uword, 23u>& registers =
      static_cast<user_context*>(p_raw_context)->general_registers;
   registers[context_rip] = __builtin_bit_cast(uword, &longjmp);
   registers[context_rdi] = __builtin_bit_cast(uword, &frame.info.buffer);
   registers[context_rsi] = 1u;
}

void
install_platform_signal(signal_kind signal) noexcept(
   !__has_feature(cxx_exceptions)
) {
   nix::sigaction action{};
   action.sigaction_handler = on_signal;
   action.flags = nix::signal_action_flags::siginfo
                  | nix::signal_action_flags::restorer
                  | nix::signal_action_flags::on_stack;
   action.restorer = cat_signal_restorer;
   nix::sys_rt_sigaction(
      static_cast<nix::signal>(signal), &action,
      &previous_actions[to_underlying(signal)]
   )
      .verify();
}

void
uninstall_platform_signal(signal_kind signal) noexcept(
   !__has_feature(cxx_exceptions)
) {
   nix::sigaction current{};
   nix::sys_rt_sigaction(static_cast<nix::signal>(signal), nullptr, &current)
      .verify();
   if (current.sigaction_handler != on_signal) {
      // Another handler was installed over ours, and may chain to it.
      terminate();
   }
   nix::sys_rt_sigaction(
      static_cast<nix::signal>(signal),
      &previous_actions[to_underlying(signal)], nullptr
   )
      .verify();
}

}  // namespace

auto
thread_raise_signal(
   signal_kind signal, void* _Nullable p_raw_info, void* _Nullable p_raw_context
) noexcept(!__has_feature(cxx_exceptions)) -> bool {
   if (!is_platform_signal(signal)) {
      raised_signal_info raised{};
      raised.signal = signal;
      raised.p_raw_info = p_raw_info;
      raised.p_raw_context = p_raw_context;
      return dispatch_signal(raised);
   }

   auto const number = int4(to_underlying(signal));
   auto const platform_signal = static_cast<nix::signal>(signal);
   nix::sigaction current{};
   nix::sys_rt_sigaction(platform_signal, nullptr, &current).verify();
   if (is_ignore_handler(current)) {
      return false;
   }

   // A caller that supplies a context vouches for it, so call the handler
   // directly, as P2069 does.
   if (p_raw_context != nullptr && !is_default_handler(current)) {
      nix::signal_info synthesized{};
      synthesized.signal_number = number;
      synthesized.code = signal_code_tkill;
      call_handler(
         current, number,
         p_raw_info != nullptr ? static_cast<nix::signal_info*>(p_raw_info)
                               : &synthesized,
         p_raw_context
      );
      return true;
   }

   // Otherwise, have the kernel deliver the signal, because handlers may read
   // the context. Third-party handlers commonly do. An unblocked signal sent
   // to the calling thread is delivered as the sending syscall returns.
   nix::signals_mask_set const mask = signal_mask(platform_signal);
   nix::signals_mask_set previous_mask{};
   nix::sys_rt_sigprocmask(nix::signal_action::unblock, &mask, &previous_mask)
      .verify();
   if (p_raw_info != nullptr) {
      auto _ = nix::sys_rt_tgsigqueueinfo(
         nix::sys_getpid(), nix::sys_gettid(), platform_signal,
         *static_cast<nix::signal_info*>(p_raw_info)
      );
   } else {
      send_to_this_thread(platform_signal);
   }
   // If a guard abandoned its callable meanwhile, this is skipped, and the
   // signal stays unblocked.
   nix::sys_rt_sigprocmask(
      nix::signal_action::set_mask, &previous_mask, nullptr
   )
      .verify();
   return true;
}

auto
signal_raised::what() const noexcept -> str_view {
   switch (signal) {
      case signal_kind::none:
         return "no signal";
      case signal_kind::interrupt:
         return "interrupt";
      case signal_kind::illegal_instruction:
         return "illegal instruction";
      case signal_kind::abort_process:
         return "abort process";
      case signal_kind::undefined_memory_access:
         return "undefined memory access";
      case signal_kind::floating_point_error:
         return "floating point error";
      case signal_kind::segmentation_fault:
         return "segmentation fault";
      case signal_kind::broken_pipe:
         return "broken pipe";
      case signal_kind::out_of_memory:
         return "out of memory";
      case signal_kind::termination:
         return "termination";
   }
   __builtin_unreachable();
}

signal_guard_install::signal_guard_install(signal_set guarded) noexcept(
   !__has_feature(cxx_exceptions)
)
    : m_guarded(guarded) {
   global_lock.lock();
   for (idx i = 0u; i < signal_kind_count; ++i) {
      auto const signal = static_cast<signal_kind>(i.raw);
      if (
         is_guarded(guarded, signal) && install_counts[i]++ == 0u
         && is_platform_signal(signal)
      ) {
         install_platform_signal(signal);
      }
   }
   global_lock.unlock();
}

signal_guard_install::~signal_guard_install() noexcept(
   !__has_feature(cxx_exceptions)
) {
   if (m_guarded == signal_set::none) {
      return;
   }
   global_lock.lock();
   for (idx i = 0u; i < signal_kind_count; ++i) {
      auto const signal = static_cast<signal_kind>(i.raw);
      if (
         is_guarded(m_guarded, signal) && --install_counts[i] == 0u
         && is_platform_signal(signal)
      ) {
         uninstall_platform_signal(signal);
      }
   }
   global_lock.unlock();
}

signal_guard_install::signal_guard_install(
   signal_guard_install&& other
) noexcept
    : m_guarded(other.m_guarded) {
   other.m_guarded = signal_set::none;
}

auto
signal_guard_install::operator=(signal_guard_install&& other) noexcept
   -> signal_guard_install& {
   signal_set const guarded = m_guarded;
   m_guarded = other.m_guarded;
   other.m_guarded = guarded;
   return *this;
}

void
detail::signal_guard_frame::push() noexcept {
   p_previous = p_guards;
   p_guards = this;
}

detail::signal_guard_frame::~signal_guard_frame() {
   // An abandoned frame is already popped, and this is then a no-op.
   p_guards = p_previous;
}

void
detail::add_signal_global_decider(
   signal_global_decider_node& node, bool call_first
) noexcept {
   global_lock.lock();
   signal_global_decider_node** p_link = &p_global_deciders;
   if (!call_first) {
      while (*p_link != nullptr) {
         p_link = &(*p_link)->p_next;
      }
   }
   node.p_next = *p_link;
   store_link(*p_link, &node);
   global_lock.unlock();
}

namespace {
auto
find_link(detail::signal_global_decider_node& node) noexcept
   -> detail::signal_global_decider_node** {
   detail::signal_global_decider_node** p_link = &p_global_deciders;
   while (*p_link != &node) {
      p_link = &(*p_link)->p_next;
   }
   return p_link;
}
}  // namespace

void
detail::remove_signal_global_decider(
   signal_global_decider_node& node
) noexcept {
   global_lock.lock();
   store_link(*find_link(node), node.p_next);
   global_lock.unlock();
}

void
detail::replace_signal_global_decider(
   signal_global_decider_node& old_node, signal_global_decider_node& new_node
) noexcept {
   global_lock.lock();
   store_link(*find_link(old_node), &new_node);
   global_lock.unlock();
}

}  // namespace cat
