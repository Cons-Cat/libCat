// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/meta>

namespace cat::detail {

// Invoke a callback if an exception unwinds this before `.commit()` is called.
// This is used primarily by my containers. It is constructed at the point from
// which a thrown exception may follow afterwards, until `.commit()`.
//
// Without exceptions, this is a no-op that never stores its callback, so it
// costs nothing. I borrowed this idea from libc++.
template <typename Callback>
class [[nodiscard]] exception_defer {
 private:
#if __has_feature(cxx_exceptions)
   Callback m_rollback;
   bool m_is_committed = false;
#endif

 public:
#if __has_feature(cxx_exceptions)
   constexpr explicit exception_defer(
      Callback&& rollback
   ) noexcept(is_nothrow_move_constructible<Callback>)
       : m_rollback(forward<Callback>(rollback)) {
   }
#else
   constexpr explicit exception_defer(Callback&& /*rollback*/) noexcept {
   }
#endif

   exception_defer(exception_defer const&) = delete;
   exception_defer(exception_defer&&) = delete;
   auto
   operator=(exception_defer const&) -> exception_defer& = delete;
   auto
   operator=(exception_defer&&) -> exception_defer& = delete;

   constexpr ~exception_defer() noexcept(noexcept(declval<Callback&>()())) {
#if __has_feature(cxx_exceptions)
      if (!m_is_committed) {
         m_rollback();
      }
#endif
   }

   // Flag that an exception has potentially thrown between the construction of
   // this `exception_defer` and this `commit()` call.
   constexpr void
   commit() noexcept {
#if __has_feature(cxx_exceptions)
      m_is_committed = true;
#endif
   }
};

}  // namespace cat::detail
