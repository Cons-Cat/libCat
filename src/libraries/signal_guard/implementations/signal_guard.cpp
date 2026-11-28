#include <cat/signal_guard>

constinit thread_local cat::detail::signal_guard_frame* _Nullable cat::detail::
   p_signal_guards = nullptr;

void
cat::detail::signal_guard_frame::push() noexcept {
   p_previous = p_signal_guards;
   p_signal_guards = this;

   // The signal handler reads this frame, but guarded code that never calls a
   // function gives the compiler no reason to store it first.
   signal_fence(memory_order::seq_cst);
}

cat::detail::signal_guard_frame::~signal_guard_frame() {
   // The signal handler may have popped this frame already.
   signal_fence(memory_order::seq_cst);

   // An abandoned frame is already popped, and this is then a no-op.
   p_signal_guards = p_previous;
}
