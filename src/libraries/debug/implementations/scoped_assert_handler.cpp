#include <cat/debug>

namespace {

constinit cat::assert_handler process_assert_handler =
   cat::default_assert_handler;

thread_local constinit void (*_Nullable thread_assert_handler)(
   cat::source_location const&
) = nullptr;

thread_local constinit cat::detail::
   assert_handler_frame* _Nullable p_assert_handler_frames = nullptr;

}  // namespace

auto
cat::current_assert_handler() -> assert_handler {
   if (p_assert_handler_frames != nullptr) {
      return p_assert_handler_frames->handler;
   }
   if (thread_assert_handler != nullptr) {
      return thread_assert_handler;
   }
   return __atomic_load_n(&process_assert_handler, __ATOMIC_ACQUIRE);
}

void
cat::set_global_assert_handler(assert_handler p_handler) {
   __atomic_store_n(&process_assert_handler, p_handler, __ATOMIC_RELEASE);
}

void
cat::set_thread_local_assert_handler(assert_handler p_handler) {
   thread_assert_handler = p_handler;
}

void
cat::reset_thread_local_assert_handler() {
   thread_assert_handler = nullptr;
}

void
cat::detail::reset_local_assert_handlers() {
   p_assert_handler_frames = nullptr;
   thread_assert_handler = nullptr;
}

cat::scoped_assert_handler::scoped_assert_handler(assert_handler p_handler)
    : m_frame{p_handler, p_assert_handler_frames} {
   p_assert_handler_frames = &m_frame;
}

cat::scoped_assert_handler::~scoped_assert_handler() {
   p_assert_handler_frames = m_frame.p_previous;
}
