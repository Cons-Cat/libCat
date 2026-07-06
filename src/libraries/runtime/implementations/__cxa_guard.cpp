#include <cat/atomic>
#include <cat/runtime>

// Implement Itanium C++ ABI thread-safe `static` variable initialisation.
//
// `__cxa_guard_acquire()`'s slow path takes the lock, returns 1 to run
// the initialiser. `__cxa_guard_release()` flips byte 0 and drops the
// lock. `__cxa_guard_abort()` only drops the lock.

namespace {

[[gnu::always_inline]]
inline auto
done_ref(cat::uword* _Nonnull p_guard) -> cat::uint1& {
   return reinterpret_cast<cat::uint1* _Nonnull>(p_guard)[0];
}

[[gnu::always_inline]]
inline auto
lock_ref(cat::uword* _Nonnull p_guard) -> cat::uint1& {
   return reinterpret_cast<cat::uint1* _Nonnull>(p_guard)[1];
}

}  // namespace

extern "C" [[nodiscard]]
auto
// NOLINTNEXTLINE
__cxa_guard_acquire(cat::uword* _Nonnull p_guard) -> int {
   cat::atomic<cat::uint1&> done{done_ref(p_guard)};
   cat::atomic<cat::uint1&> lock{lock_ref(p_guard)};

   if (done.load(cat::memory_order::acquire) != 0u) {
      return 0;
   }

   // Spin-acquire the lock byte; whoever wins runs the initialiser.
   for (cat::uint1 expected = 0u; !lock.compare_exchange_weak(
           expected, 1u, cat::memory_order::acquire, cat::memory_order::relaxed
        );
        expected = 0u) {
      __builtin_ia32_pause();
   }

   if (done.load(cat::memory_order::acquire) != 0u) {
      // Another thread finished while we were spinning.
      lock.store(0u, cat::memory_order::release);
      return 0;
   }
   return 1;
}

extern "C" void
// NOLINTNEXTLINE
__cxa_guard_release(cat::uword* _Nonnull p_guard) {
   cat::atomic<cat::uint1&>{done_ref(p_guard)}.store(
      1u, cat::memory_order::release
   );
   cat::atomic<cat::uint1&>{lock_ref(p_guard)}.store(
      0u, cat::memory_order::release
   );
}

extern "C" void
// NOLINTNEXTLINE
__cxa_guard_abort(cat::uword* _Nonnull p_guard) {
   cat::atomic<cat::uint1&>{lock_ref(p_guard)}.store(
      0u, cat::memory_order::release
   );
}
