// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/atomic>

// Included by <cat/atomic> immediately after `atomic_ref_bound` is defined.

namespace cat {

template <is_atomic Atomic, memory_order order>
class memory_order_reference
    : public detail::atomic_ref_bound<
         typename detail::memory_order_traits<Atomic>::target_type, order> {
 private:
   using target_type = detail::memory_order_traits<Atomic>::target_type;
   using base = detail::atomic_ref_bound<target_type, order>;

 public:
   using base::operator=;

   explicit constexpr memory_order_reference(
      Atomic& atomic [[clang::lifetimebound]]
   )
       : base(target(atomic)) {
   }

   memory_order_reference(memory_order_reference const&) = default;

   auto
   operator=(memory_order_reference const&) -> memory_order_reference& = delete;

   friend constexpr void
   operator&(memory_order_reference const volatile&) = delete;

   friend constexpr void
   operator&(memory_order_reference const volatile&&) = delete;

 private:
   static constexpr auto
   target(Atomic& atomic) -> target_type& {
      if constexpr (detail::memory_order_traits<Atomic>::is_reference) {
         return *atomic.m_p_value;
      } else {
         return atomic.m_value;
      }
   }
};

namespace detail {
template <overflow_policies overflow_policy>
constexpr auto
bind_atomic_overflow_policy(auto memory_order_reference) {
   if constexpr (overflow_policy == overflow_policies::undefined) {
      return memory_order_reference.undef();
   } else if constexpr (overflow_policy == overflow_policies::wrap) {
      return memory_order_reference.wrap();
   } else {
      return memory_order_reference.sat();
   }
}
}  // namespace detail

template <is_atomic Atomic, overflow_policies overflow_policy>
constexpr auto
detail::atomic_overflow_reference<Atomic, overflow_policy>::relaxed() const {
   return bind_atomic_overflow_policy<overflow_policy>(m_p_atomic->relaxed());
}

template <is_atomic Atomic, overflow_policies overflow_policy>
constexpr auto
detail::atomic_overflow_reference<Atomic, overflow_policy>::acquire() const {
   return bind_atomic_overflow_policy<overflow_policy>(m_p_atomic->acquire());
}

template <is_atomic Atomic, overflow_policies overflow_policy>
constexpr auto
detail::atomic_overflow_reference<Atomic, overflow_policy>::release() const {
   return bind_atomic_overflow_policy<overflow_policy>(m_p_atomic->release());
}

template <is_atomic Atomic, overflow_policies overflow_policy>
constexpr auto
detail::atomic_overflow_reference<Atomic, overflow_policy>::acq_rel() const {
   return bind_atomic_overflow_policy<overflow_policy>(m_p_atomic->acq_rel());
}

template <is_atomic Atomic, overflow_policies overflow_policy>
constexpr auto
detail::atomic_overflow_reference<Atomic, overflow_policy>::seq_cst() const {
   return bind_atomic_overflow_policy<overflow_policy>(m_p_atomic->seq_cst());
}

template <typename T>
   requires(!is_reference<T> && !is_const<T> && !is_volatile<T>)
constexpr auto
atomic<T>::relaxed() & -> memory_order_reference<
   atomic, memory_order::relaxed> {
   return memory_order_reference<atomic, memory_order::relaxed>(*this);
}

template <typename T>
   requires(!is_reference<T> && !is_const<T> && !is_volatile<T>)
constexpr auto
atomic<T>::relaxed() const& -> memory_order_reference<
   atomic const, memory_order::relaxed> {
   return memory_order_reference<atomic const, memory_order::relaxed>(*this);
}

template <typename T>
   requires(!is_reference<T> && !is_const<T> && !is_volatile<T>)
constexpr auto
atomic<T>::acquire() & -> memory_order_reference<
   atomic, memory_order::acquire> {
   return memory_order_reference<atomic, memory_order::acquire>(*this);
}

template <typename T>
   requires(!is_reference<T> && !is_const<T> && !is_volatile<T>)
constexpr auto
atomic<T>::acquire() const& -> memory_order_reference<
   atomic const, memory_order::acquire> {
   return memory_order_reference<atomic const, memory_order::acquire>(*this);
}

template <typename T>
   requires(!is_reference<T> && !is_const<T> && !is_volatile<T>)
constexpr auto
atomic<T>::release() & -> memory_order_reference<
   atomic, memory_order::release> {
   return memory_order_reference<atomic, memory_order::release>(*this);
}

template <typename T>
   requires(!is_reference<T> && !is_const<T> && !is_volatile<T>)
constexpr auto
atomic<T>::release() const& -> memory_order_reference<
   atomic const, memory_order::release> {
   return memory_order_reference<atomic const, memory_order::release>(*this);
}

template <typename T>
   requires(!is_reference<T> && !is_const<T> && !is_volatile<T>)
constexpr auto
atomic<T>::acq_rel() & -> memory_order_reference<
   atomic, memory_order::acq_rel> {
   return memory_order_reference<atomic, memory_order::acq_rel>(*this);
}

template <typename T>
   requires(!is_reference<T> && !is_const<T> && !is_volatile<T>)
constexpr auto
atomic<T>::acq_rel() const& -> memory_order_reference<
   atomic const, memory_order::acq_rel> {
   return memory_order_reference<atomic const, memory_order::acq_rel>(*this);
}

template <typename T>
   requires(!is_reference<T> && !is_const<T> && !is_volatile<T>)
constexpr auto
atomic<T>::seq_cst() & -> memory_order_reference<
   atomic, memory_order::seq_cst> {
   return memory_order_reference<atomic, memory_order::seq_cst>(*this);
}

template <typename T>
   requires(!is_reference<T> && !is_const<T> && !is_volatile<T>)
constexpr auto
atomic<T>::seq_cst() const& -> memory_order_reference<
   atomic const, memory_order::seq_cst> {
   return memory_order_reference<atomic const, memory_order::seq_cst>(*this);
}

template <typename T>
   requires(!is_volatile<T>)
constexpr auto
atomic<T&>::relaxed() & -> memory_order_reference<
   atomic, memory_order::relaxed> {
   return memory_order_reference<atomic, memory_order::relaxed>(*this);
}

template <typename T>
   requires(!is_volatile<T>)
constexpr auto
atomic<T&>::relaxed() const& -> memory_order_reference<
   atomic const, memory_order::relaxed> {
   return memory_order_reference<atomic const, memory_order::relaxed>(*this);
}

template <typename T>
   requires(!is_volatile<T>)
constexpr auto
atomic<T&>::acquire() & -> memory_order_reference<
   atomic, memory_order::acquire> {
   return memory_order_reference<atomic, memory_order::acquire>(*this);
}

template <typename T>
   requires(!is_volatile<T>)
constexpr auto
atomic<T&>::acquire() const& -> memory_order_reference<
   atomic const, memory_order::acquire> {
   return memory_order_reference<atomic const, memory_order::acquire>(*this);
}

template <typename T>
   requires(!is_volatile<T>)
constexpr auto
atomic<T&>::release() & -> memory_order_reference<
   atomic, memory_order::release> {
   return memory_order_reference<atomic, memory_order::release>(*this);
}

template <typename T>
   requires(!is_volatile<T>)
constexpr auto
atomic<T&>::release() const& -> memory_order_reference<
   atomic const, memory_order::release> {
   return memory_order_reference<atomic const, memory_order::release>(*this);
}

template <typename T>
   requires(!is_volatile<T>)
constexpr auto
atomic<T&>::acq_rel() & -> memory_order_reference<
   atomic, memory_order::acq_rel> {
   return memory_order_reference<atomic, memory_order::acq_rel>(*this);
}

template <typename T>
   requires(!is_volatile<T>)
constexpr auto
atomic<T&>::acq_rel() const& -> memory_order_reference<
   atomic const, memory_order::acq_rel> {
   return memory_order_reference<atomic const, memory_order::acq_rel>(*this);
}

template <typename T>
   requires(!is_volatile<T>)
constexpr auto
atomic<T&>::seq_cst() & -> memory_order_reference<
   atomic, memory_order::seq_cst> {
   return memory_order_reference<atomic, memory_order::seq_cst>(*this);
}

template <typename T>
   requires(!is_volatile<T>)
constexpr auto
atomic<T&>::seq_cst() const& -> memory_order_reference<
   atomic const, memory_order::seq_cst> {
   return memory_order_reference<atomic const, memory_order::seq_cst>(*this);
}

}  // namespace cat
