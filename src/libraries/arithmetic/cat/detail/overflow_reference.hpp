// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic_interface>

// Included by <cat/arithmetic> immediately after `struct arithmetic` closes.

namespace cat::detail {

// This proxy reference wraps an arithmetic type with specific overflow
// semantics. It is only intended to be created by the overflow accessors of
// `cat::arithmetic`, `cat::index`, and `cat::arithmetic_ptr`.
//
// In the past, we had expressed overflow accessors through `union` punning and
// `reinterpret_cast`. These approaches violated lifetime semantics and were not
// `constexpr`-friendly.
template <typename WrappedQual, overflow_policies Semantics>
class overflow_reference
    : public arithmetic_interface<
         detail::overflow_reference<WrappedQual, Semantics>> {
 public:
   constexpr explicit overflow_reference(WrappedQual& w)
       : m_wrapped(__builtin_addressof(w)) {
   }

 private:
   using wrapper_type = remove_cvref<WrappedQual>;
   using raw_type = typename wrapper_type::raw_type;

   static constexpr overflow_policies policy = Semantics;

   WrappedQual* m_wrapped;

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   view() const {
      if constexpr (is_arithmetic_ptr<wrapper_type>) {
         using pointee = remove_pointer<typename wrapper_type::ptr>;
         return arithmetic_ptr<pointee, raw_type, Semantics>(m_wrapped->raw);
      } else if constexpr (detail::is_idx<wrapper_type>) {
         return index<Semantics>(m_wrapped->raw);
      } else {
         return arithmetic<raw_type, Semantics>(m_wrapped->raw);
      }
   }

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   view_undef_cmp() const {
      if constexpr (is_arithmetic_ptr<wrapper_type>) {
         using pointee = remove_pointer<typename wrapper_type::ptr>;
         return arithmetic_ptr<pointee, raw_type, overflow_policies::undefined>(
            m_wrapped->raw);
      } else if constexpr (detail::is_idx<wrapper_type>) {
         return index<overflow_policies::undefined>(m_wrapped->raw);
      } else {
         return arithmetic<raw_type, overflow_policies::undefined>(
            m_wrapped->raw);
      }
   }

 public:
   constexpr auto
   undef() & -> detail::overflow_reference<WrappedQual,
                                           overflow_policies::undefined> {
      return detail::overflow_reference<WrappedQual,
                                        overflow_policies::undefined>(
         *m_wrapped);
   }

   constexpr auto
   undef() const& -> detail::overflow_reference<WrappedQual const,
                                                overflow_policies::undefined> {
      return detail::overflow_reference<WrappedQual const,
                                        overflow_policies::undefined>(
         *m_wrapped);
   }

   constexpr auto
   wrap() & -> detail::overflow_reference<WrappedQual,
                                          overflow_policies::wrap> {
      return detail::overflow_reference<WrappedQual, overflow_policies::wrap>(
         *m_wrapped);
   }

   constexpr auto
   wrap() const& -> detail::overflow_reference<WrappedQual const,
                                               overflow_policies::wrap> {
      return detail::overflow_reference<WrappedQual const,
                                        overflow_policies::wrap>(*m_wrapped);
   }

   constexpr auto
   sat() & -> detail::overflow_reference<WrappedQual,
                                         overflow_policies::saturate> {
      return detail::overflow_reference<WrappedQual,
                                        overflow_policies::saturate>(
         *m_wrapped);
   }

   constexpr auto
   sat() const& -> detail::overflow_reference<WrappedQual const,
                                              overflow_policies::saturate> {
      return detail::overflow_reference<WrappedQual const,
                                        overflow_policies::saturate>(
         *m_wrapped);
   }

   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   undef() && {
      if constexpr (is_arithmetic_ptr<wrapper_type>) {
         using pointee = remove_pointer<typename wrapper_type::ptr>;
         return arithmetic_ptr<pointee, raw_type, overflow_policies::undefined>(
            m_wrapped->raw);
      } else if constexpr (detail::is_idx<wrapper_type>) {
         return index<overflow_policies::undefined>(m_wrapped->raw);
      } else {
         return arithmetic<raw_type, overflow_policies::undefined>(
            m_wrapped->raw);
      }
   }

   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   wrap() && {
      if constexpr (is_arithmetic_ptr<wrapper_type>) {
         using pointee = remove_pointer<typename wrapper_type::ptr>;
         return arithmetic_ptr<pointee, raw_type, overflow_policies::wrap>(
            m_wrapped->raw);
      } else if constexpr (detail::is_idx<wrapper_type>) {
         return index<overflow_policies::wrap>(m_wrapped->raw);
      } else {
         return arithmetic<raw_type, overflow_policies::wrap>(m_wrapped->raw);
      }
   }

   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   sat() && {
      if constexpr (is_arithmetic_ptr<wrapper_type>) {
         using pointee = remove_pointer<typename wrapper_type::ptr>;
         return arithmetic_ptr<pointee, raw_type, overflow_policies::saturate>(
            m_wrapped->raw);
      } else if constexpr (detail::is_idx<wrapper_type>) {
         return index<overflow_policies::saturate>(m_wrapped->raw);
      } else {
         return arithmetic<raw_type, overflow_policies::saturate>(
            m_wrapped->raw);
      }
   }

   template <typename U>
      requires(!is_arithmetic_ptr<wrapper_type>
               && !detail::is_idx<wrapper_type>)
   [[gnu::always_inline, gnu::nodebug]]
   constexpr explicit
   operator U() const {
      return static_cast<U>(*m_wrapped);
   }

   template <is_integral U>
      requires(is_signed<raw_arithmetic_type<U>>
               && detail::is_idx<wrapper_type>)
   [[gnu::always_inline, gnu::nodebug]]
   constexpr explicit(sizeof(raw_type) > sizeof(U))
   operator U() const {
      return static_cast<U>(m_wrapped->raw);
   }

   template <is_same<__SIZE_TYPE__> U>
      requires(detail::is_idx<wrapper_type>)
   [[gnu::always_inline, gnu::nodebug]]
   constexpr
   operator U() const {
      return m_wrapped->raw;
   }

   template <typename U>
      requires(detail::is_idx<wrapper_type>)
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   operator<=>(U rhs) const {
      return view_undef_cmp() <=> rhs;
   }

   template <typename U>
      requires(detail::is_idx<wrapper_type>)
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   friend constexpr auto
   operator==(overflow_reference lhs, U rhs) -> bool {
      using common = common_type<raw_type, raw_arithmetic_type<U>>;
      return common(lhs.m_wrapped->raw) == common(make_raw_arithmetic(rhs));
   }

   template <is_integral U>
      requires(is_safe_arithmetic_comparison<raw_type, U>
               && is_arithmetic_ptr<wrapper_type>)
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   friend constexpr auto
   operator<=>(overflow_reference lhs, U rhs) {
      return lhs.m_wrapped->raw <=> make_raw_arithmetic(rhs);
   }

   template <is_integral U>
      requires(is_safe_arithmetic_comparison<raw_type, U>
               && is_arithmetic_ptr<wrapper_type>)
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   friend constexpr auto
   operator==(overflow_reference lhs, U rhs) -> bool {
      return lhs.m_wrapped->raw == make_raw_arithmetic(rhs);
   }

   template <is_arithmetic U>
      requires(is_safe_arithmetic_comparison<raw_type, U>
               && !is_arithmetic_ptr<wrapper_type>
               && !detail::is_idx<wrapper_type>)
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   operator<=>(U rhs) const {
      return view_undef_cmp() <=> rhs;
   }

   template <is_arithmetic U>
      requires(is_safe_arithmetic_comparison<raw_type, U>
               && !is_arithmetic_ptr<wrapper_type>
               && !detail::is_idx<wrapper_type>)
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   friend constexpr auto
   operator==(overflow_reference lhs, U rhs) -> bool {
      return *lhs.m_wrapped == make_raw_arithmetic(rhs);
   }

   template <is_arithmetic U>
      requires(!is_safe_arithmetic_comparison<raw_type, U>
               && !detail::is_idx<overflow_reference>
               && sizeof(raw_type) >= sizeof(raw_arithmetic_type<U>)
               && ((is_unsigned_integral<raw_type> && is_signed_integral<U>)
                   || (is_signed_integral<raw_type> && is_unsigned_integral<U>))
               && !is_arithmetic_ptr<wrapper_type>
               && !detail::is_idx<wrapper_type>)
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   friend constexpr auto
   operator<=>(overflow_reference lhs, U rhs) __attribute__((enable_if(
      detail::raw_mixed_integral_compare_sound(lhs.m_wrapped->raw,
                                               make_raw_arithmetic(rhs)),
      "mixed signed-unsigned comparison is not sound for these values!"))) {
      auto const rhs_raw = make_raw_arithmetic(rhs);
      if !consteval {
         assert(detail::raw_mixed_integral_compare_sound(lhs.m_wrapped->raw,
                                                         rhs_raw));
      }
      if constexpr (is_signed_integral<raw_type>
                    && is_unsigned_integral<raw_arithmetic_type<U>>) {
         if (lhs.m_wrapped->raw < 0) {
            return std::strong_ordering::less;
         }
      }
      if constexpr (is_unsigned_integral<raw_type>
                    && is_signed_integral<raw_arithmetic_type<U>>) {
         if (rhs_raw < 0) {
            return std::strong_ordering::greater;
         }
      }
      using common = common_type<raw_type, raw_arithmetic_type<U>>;
      return static_cast<common>(lhs.m_wrapped->raw)
             <=> static_cast<common>(rhs_raw);
   }

   template <is_arithmetic U>
      requires(!is_safe_arithmetic_comparison<raw_type, U>
               && !detail::is_idx<overflow_reference>
               && sizeof(raw_type) >= sizeof(raw_arithmetic_type<U>)
               && ((is_unsigned_integral<raw_type> && is_signed_integral<U>)
                   || (is_signed_integral<raw_type> && is_unsigned_integral<U>))
               && !is_arithmetic_ptr<wrapper_type>
               && !detail::is_idx<wrapper_type>)
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   friend constexpr bool
   operator==(overflow_reference lhs, U rhs) __attribute__((enable_if(
      detail::raw_mixed_integral_compare_sound(lhs.m_wrapped->raw,
                                               make_raw_arithmetic(rhs)),
      "mixed signed-unsigned comparison is not sound for these values!"))) {
      auto const rhs_raw = make_raw_arithmetic(rhs);
      if !consteval {
         verify(detail::raw_mixed_integral_compare_sound(lhs.m_wrapped->raw,
                                                         rhs_raw));
      }
      if constexpr (is_signed_integral<raw_type>
                    && is_unsigned_integral<raw_arithmetic_type<U>>) {
         if (lhs.m_wrapped->raw < 0) {
            return false;
         }
      }
      if constexpr (is_unsigned_integral<raw_type>
                    && is_signed_integral<raw_arithmetic_type<U>>) {
         if (rhs_raw < 0) {
            return false;
         }
      }
      using common = common_type<raw_type, raw_arithmetic_type<U>>;
      return static_cast<common>(lhs.m_wrapped->raw)
             == static_cast<common>(rhs_raw);
   }

   template <is_integral U>
      requires(sizeof(U) < sizeof(raw_type)
               && !is_const<WrappedQual> && detail::is_idx<wrapper_type>)
   [[gnu::always_inline, gnu::nodebug]]
   constexpr overflow_reference&
   operator=(U other) __attribute__((
      enable_if(detail::raw_source_fits_implicit_storage<raw_type>(other),
                "value is out of range for assignment to cat::index"))) {
      *m_wrapped = other;
      return *this;
   }

   template <is_arithmetic U>
      requires(is_safe_arithmetic_comparison<raw_type, U>
               && !is_const<WrappedQual> && !is_arithmetic_ptr<wrapper_type>
               && !detail::is_idx<wrapper_type>)
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   operator=(U operand) -> overflow_reference& {
      *m_wrapped = operand;
      return *this;
   }

   template <is_integral U>
      requires(is_safe_arithmetic_conversion<U, raw_type>
               && !is_const<WrappedQual> && is_arithmetic_ptr<wrapper_type>)
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   operator=(U operand) -> overflow_reference& {
      *m_wrapped = operand;
      return *this;
   }

   template <is_arithmetic U>
      requires(is_safe_arithmetic_comparison<raw_type, U>
               && !is_const<WrappedQual> && !is_arithmetic_ptr<wrapper_type>
               && !detail::is_idx<wrapper_type>)
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   operator+=(U operand) -> overflow_reference& {
      *this = add(operand);
      return *this;
   }

   template <is_integral U>
      requires(is_safe_arithmetic_conversion<U, raw_type>
               && !is_const<WrappedQual> && is_arithmetic_ptr<wrapper_type>)
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   operator+=(U operand) -> overflow_reference& {
      *this = add(operand);
      return *this;
   }

   template <is_arithmetic U>
      requires(!is_const<WrappedQual> && detail::is_idx<wrapper_type>)
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   operator+=(U operand) -> overflow_reference& {
      *m_wrapped = make_raw_arithmetic(add(operand));
      return *this;
   }

   template <is_arithmetic U>
      requires(!is_arithmetic_ptr<wrapper_type>
               && !detail::is_idx<wrapper_type>)
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   add(U other) const
      -> detail::promoted_type<arithmetic<raw_type, Semantics>, U> {
      return view().add(other);
   }

   template <is_integral U>
      requires(is_arithmetic_ptr<wrapper_type>)
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   add(U operand) const {
      return view().add(operand);
   }

   template <is_unsigned_integral T_int>
      requires(detail::is_idx<wrapper_type>)
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   add(T_int other) const -> index<Semantics> {
      return view().add(other);
   }

   template <is_signed_integral T_int>
      requires(detail::is_idx<wrapper_type>)
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   add(T_int other) const
      -> arithmetic<make_signed_type<__SIZE_TYPE__>, Semantics> {
      return view().add(other);
   }

   template <typename U>
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   add(U* p_operand) const -> U* {
      return view().add(p_operand);
   }

   template <is_arithmetic U>
      requires(!is_arithmetic_ptr<wrapper_type>
               && !detail::is_idx<wrapper_type>)
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   subtract_by(U operand) const
      -> detail::promoted_type<arithmetic<raw_type, Semantics>, U> {
      return view().subtract_by(operand);
   }

   template <is_integral U>
      requires(is_arithmetic_ptr<wrapper_type>)
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   subtract_by(U operand) const {
      return view().subtract_by(operand);
   }

   template <is_integral T_int>
      requires(!is_same<remove_cvref<T_int>, index<Semantics>>
               && detail::is_idx<wrapper_type>)
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   subtract_by(T_int other) const
      -> arithmetic<make_signed_type<__SIZE_TYPE__>, Semantics> {
      return view().subtract_by(other);
   }

   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   subtract_by(index<Semantics> other) const
      -> arithmetic<make_signed_type<__SIZE_TYPE__>, Semantics>
      requires(detail::is_idx<wrapper_type>)
   {
      return view().subtract_by(other);
   }

   template <typename U>
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   subtract_by(U* p_operand) -> U* {
      return view().subtract_by(p_operand);
   }

   template <is_arithmetic U>
      requires(!is_arithmetic_ptr<wrapper_type>
               && !detail::is_idx<wrapper_type>)
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   subtract_from(U operand) const
      -> detail::promoted_type<arithmetic<raw_type, Semantics>, U> {
      return view().subtract_from(operand);
   }

   template <is_arithmetic U>
      requires(is_arithmetic_ptr<wrapper_type>)
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   subtract_from(U operand) const {
      return view().subtract_from(operand);
   }

   template <is_arithmetic U>
      requires(detail::is_idx<wrapper_type>)
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   subtract_from(U other) const {
      return view().subtract_from(other);
   }

   template <typename U>
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   subtract_from(U* p_operand) -> U* {
      return view().subtract_from(p_operand);
   }

   template <is_arithmetic U>
      requires(!is_arithmetic_ptr<wrapper_type>
               && !detail::is_idx<wrapper_type>)
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   multiply(U operand) const
      -> detail::promoted_type<arithmetic<raw_type, Semantics>, U> {
      return view().multiply(operand);
   }

   template <typename U>
      requires(is_arithmetic_ptr<wrapper_type>)
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   multiply(U operand) const {
      return view().multiply(operand);
   }

   template <is_integral T_int>
      requires(detail::is_idx<wrapper_type>)
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   multiply(T_int other) const -> index<Semantics> {
      return view().multiply(other);
   }

   template <typename U>
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   multiply(U* p_operand) const -> U* {
      return view().multiply(p_operand);
   }

   template <typename U>
      requires(!is_arithmetic_ptr<wrapper_type>
               && !detail::is_idx<wrapper_type>)
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   divide_by(U operand) const
      -> detail::promoted_type<arithmetic<raw_type, Semantics>, U> {
      return view().divide_by(operand);
   }

   template <typename U>
      requires(is_arithmetic_ptr<wrapper_type>)
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   divide_by(U operand) const {
      return view().divide_by(operand);
   }

   template <is_integral T_int>
      requires(detail::is_idx<wrapper_type>)
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   divide_by(T_int other) const {
      return view().divide_by(other);
   }

   template <typename U>
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   divide_by(U* p_operand) const -> U* {
      return view().divide_by(p_operand);
   }

   template <is_unsafe_arithmetic U>
      requires(is_safe_arithmetic_comparison<raw_type, U>
               && !is_arithmetic_ptr<wrapper_type>
               && !detail::is_idx<wrapper_type>)
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   divide_into(U operand) const
      -> detail::promoted_type<U, arithmetic<raw_type, Semantics>> {
      return view().divide_into(operand);
   }

   template <typename U>
      requires(is_arithmetic_ptr<wrapper_type>)
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   divide_into(U operand) const {
      return view().divide_into(operand);
   }

   template <is_integral T_int>
      requires(detail::is_idx<wrapper_type>)
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   divide_into(T_int other) const {
      return view().divide_into(other);
   }

   template <typename U>
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   divide_into(U* p_operand) const -> U* {
      return view().divide_into(p_operand);
   }

   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   bit_complement() const {
      return view().bit_complement();
   }

   template <is_arithmetic U>
      requires(!is_arithmetic_ptr<wrapper_type>
               && !detail::is_idx<wrapper_type>)
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   modulo_by(U operand) const -> detail::promoted_type<raw_type, U> {
      return view().modulo_by(operand);
   }

   template <typename U>
      requires(is_arithmetic_ptr<wrapper_type>)
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   modulo_by(U operand) const {
      return view().modulo_by(operand);
   }

   template <is_integral T_int>
      requires(detail::is_idx<wrapper_type>)
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   modulo_by(T_int other) const -> index<Semantics> {
      return view().modulo_by(other);
   }

   template <typename U>
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   modulo_by(U* p_operand) const -> U* {
      return view().modulo_by(p_operand);
   }

   template <typename U>
      requires(is_arithmetic_ptr<wrapper_type>)
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   modulo_into(U operand) const {
      return view().modulo_into(operand);
   }

   template <is_integral T_int>
      requires(detail::is_idx<wrapper_type>)
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   modulo_into(T_int operand) const -> index<Semantics> {
      return view().modulo_into(operand);
   }

   template <is_arithmetic U>
      requires(is_safe_arithmetic_comparison<raw_type, U>
               && !is_arithmetic_ptr<wrapper_type>
               && !detail::is_idx<wrapper_type>)
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   bit_and(U operand) const
      -> detail::promoted_type<arithmetic<raw_type, Semantics>, U> {
      return view().bit_and(operand);
   }

   template <typename U>
      requires(is_arithmetic_ptr<wrapper_type>)
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   bit_and(U other) const {
      return view().bit_and(other);
   }

   template <is_integral T_int>
      requires(detail::is_idx<wrapper_type>)
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   bit_and(T_int other) const -> index<Semantics> {
      return view().bit_and(other);
   }

   template <typename U>
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   bit_and(U* p_operand) const -> U* {
      return view().bit_and(p_operand);
   }

   template <is_arithmetic U>
      requires(is_safe_arithmetic_comparison<U, raw_type>
               && !is_arithmetic_ptr<wrapper_type>
               && !detail::is_idx<wrapper_type>)
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   bit_or(U operand) const -> detail::promoted_type<raw_type, U> {
      return view().bit_or(operand);
   }

   template <typename Self, typename U>
      requires(is_arithmetic_ptr<wrapper_type>)
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   bit_or(Self self, U operand) const {
      return view().bit_or(self, operand);
   }

   template <typename U>
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   bit_or(U* p_operand) const -> U* {
      return view().bit_or(p_operand);
   }

   template <is_integral T_int>
      requires(detail::is_idx<wrapper_type>)
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   bit_or(T_int other) const -> index<Semantics> {
      return view().bit_or(other);
   }

   template <is_arithmetic U>
      requires(!is_arithmetic_ptr<wrapper_type>
               && !detail::is_idx<wrapper_type>)
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   shift_left_by(U operand) const -> arithmetic<raw_type, Semantics> {
      return view().shift_left_by(operand);
   }

   template <typename U>
      requires(is_arithmetic_ptr<wrapper_type>)
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   shift_left_by(U operand) const {
      return view().shift_left_by(operand);
   }

   template <is_integral T_int>
      requires(detail::is_idx<wrapper_type>)
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   shift_left_by(T_int other) const -> index<Semantics> {
      return view().shift_left_by(other);
   }

   template <is_arithmetic U>
      requires(!is_arithmetic_ptr<wrapper_type>
               && !detail::is_idx<wrapper_type>)
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   shift_left_into(U other) const -> U {
      return view().shift_left_into(other);
   }

   template <is_arithmetic U>
      requires(is_arithmetic_ptr<wrapper_type>)
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   shift_left_into(U other) const -> U {
      return view().shift_left_into(other);
   }

   template <is_arithmetic U>
      requires(detail::is_idx<wrapper_type>)
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   shift_left_into(U other) const -> U {
      return view().shift_left_into(other);
   }

   template <is_arithmetic U>
      requires(!is_arithmetic_ptr<wrapper_type>
               && !detail::is_idx<wrapper_type>)
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   shift_right_by(U operand) const -> arithmetic<raw_type, Semantics> {
      return view().shift_right_by(operand);
   }

   template <typename U>
      requires(is_arithmetic_ptr<wrapper_type>)
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   shift_right_by(U operand) const {
      return view().shift_right_by(operand);
   }

   template <is_integral T_int>
      requires(detail::is_idx<wrapper_type>)
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   shift_right_by(T_int other) const -> index<Semantics> {
      return view().shift_right_by(other);
   }

   template <is_arithmetic U>
      requires(!is_arithmetic_ptr<wrapper_type>
               && !detail::is_idx<wrapper_type>)
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   shift_right_into(U other) const -> U {
      return view().shift_right_into(other);
   }

   template <is_arithmetic U>
      requires(is_arithmetic_ptr<wrapper_type>)
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   shift_right_into(U other) const -> U {
      return view().shift_right_into(other);
   }

   template <is_integral T_int>
      requires(detail::is_idx<wrapper_type>)
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   shift_right_into(T_int other) const -> decltype(auto) {
      return view().shift_right_into(other);
   }
};

}  // namespace cat::detail
