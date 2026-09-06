// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic_interface>

namespace cat {

namespace detail {

template <typename T, precision_policies fallback>
inline constexpr precision_policies float_precision_for = fallback;

template <
   typename T, precision_policies precision, auto quantity,
   precision_policies fallback>
inline constexpr precision_policies
   float_precision_for<basic_float<T, precision, quantity>, fallback> =
      precision;

}  // namespace detail

template <typename T, typename CharT>
struct formatter;

template <typename WrappedQual, precision_policies precision>
class precision_reference {
 private:
   template <typename, precision_policies>
   friend class precision_reference;

   template <typename, typename>
   friend struct formatter;

 public:
   constexpr explicit precision_reference(WrappedQual& w)
       : m_wrapped(__builtin_addressof(w)) {
   }

   using raw_type = remove_cvref<WrappedQual>::raw_type;
   using quantity_type = remove_cvref<WrappedQual>::quantity_type;
   static constexpr auto quantity_reference =
      remove_cvref<WrappedQual>::quantity_reference;

   static constexpr precision_policies precision_policy = precision;

   // Rebind this reference wrapper to a different address.
   constexpr void
   rebind(WrappedQual& w [[clang::lifetime_capture_by_this]]) {
      m_wrapped = __builtin_addressof(w);
   }

 private:
   WrappedQual* _Nonnull m_wrapped;

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   view() const -> basic_float<raw_type, precision, quantity_reference> {
      return basic_float<raw_type, precision, quantity_reference>(
         m_wrapped->raw
      );
   }

 public:
   constexpr auto
   precise() & -> precision_reference<
      WrappedQual, precision_policies::precise> {
      return precision_reference<WrappedQual, precision_policies::precise>(
         *m_wrapped
      );
   }

   constexpr auto
   precise() const& -> precision_reference<
      WrappedQual const, precision_policies::precise> {
      return precision_reference<
         WrappedQual const, precision_policies::precise>(*m_wrapped);
   }

   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   precise() && -> basic_float<
      raw_type, precision_policies::precise, quantity_reference> {
      return basic_float<
         raw_type, precision_policies::precise, quantity_reference>(
         m_wrapped->raw
      );
   }

   constexpr auto
   fast() & -> precision_reference<WrappedQual, precision_policies::fast> {
      return precision_reference<WrappedQual, precision_policies::fast>(
         *m_wrapped
      );
   }

   constexpr auto
   fast() const& -> precision_reference<
      WrappedQual const, precision_policies::fast> {
      return precision_reference<WrappedQual const, precision_policies::fast>(
         *m_wrapped
      );
   }

   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   fast() && -> basic_float<
      raw_type, precision_policies::fast, quantity_reference> {
      return basic_float<
         raw_type, precision_policies::fast, quantity_reference>(
         m_wrapped->raw
      );
   }

   template <typename U>
   [[gnu::always_inline, gnu::nodebug]]
   constexpr explicit
   operator U() const {
      return static_cast<U>(view());
   }

   template <is_arithmetic U>
      requires(is_safe_arithmetic_comparison<raw_type, U>)
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   operator<=>(U rhs) const {
      return view() <=> rhs;
   }

   template <typename OtherWrappedQual, precision_policies other_precision>
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   operator<=>(precision_reference<OtherWrappedQual, other_precision> rhs) const
      -> std::partial_ordering {
      return view() <=> rhs.view();
   }

   template <is_arithmetic U>
      requires(is_safe_arithmetic_comparison<raw_type, U>)
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   friend constexpr auto
   operator==(precision_reference lhs, U rhs) -> bool {
      return lhs.view() == rhs;
   }

   template <typename OtherWrappedQual, precision_policies other_precision>
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   friend constexpr auto
   operator==(
      precision_reference lhs,
      precision_reference<OtherWrappedQual, other_precision> rhs
   ) -> bool {
      return lhs.view() == rhs.view();
   }

   template <is_arithmetic U>
      requires(
         is_safe_arithmetic_comparison<raw_type, U> && !is_const<WrappedQual>
      )
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   operator=(U operand) -> precision_reference& {
      *m_wrapped = operand;
      return *this;
   }

   template <is_arithmetic U>
      requires(is_floating_point<U>)
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   add(U other) const {
      return view().add(other);
   }

   template <typename OtherWrappedQual, precision_policies other_precision>
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   add(precision_reference<OtherWrappedQual, other_precision> other) const {
      return view().add(other.view());
   }

   template <is_arithmetic U>
      requires(is_floating_point<U>)
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   subtract_by(U operand) const {
      return view().subtract_by(operand);
   }

   template <typename OtherWrappedQual, precision_policies other_precision>
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   subtract_by(
      precision_reference<OtherWrappedQual, other_precision> operand
   ) const {
      return view().subtract_by(operand.view());
   }

   template <is_arithmetic U>
      requires(
         is_floating_point<U>
         && detail::is_quantity_addable<
            U, basic_float<raw_type, precision, quantity_reference>>
      )
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   subtract_from(U operand) const {
      return view().subtract_from(operand);
   }

   template <is_arithmetic U>
      requires(is_floating_point<U>)
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   multiply(U operand) const {
      return view().multiply(operand);
   }

   template <typename OtherWrappedQual, precision_policies other_precision>
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   multiply(
      precision_reference<OtherWrappedQual, other_precision> operand
   ) const {
      return view().multiply(operand.view());
   }

   template <is_arithmetic U>
      requires(is_floating_point<U>)
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   divide_by(U operand) const {
      return view().divide_by(operand);
   }

   template <typename OtherWrappedQual, precision_policies other_precision>
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   divide_by(
      precision_reference<OtherWrappedQual, other_precision> operand
   ) const {
      return view().divide_by(operand.view());
   }

   template <is_raw_arithmetic U>
      requires(is_floating_point<U>)
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   divide_into(U operand) const {
      return view().divide_into(operand);
   }

   template <is_arithmetic U, is_arithmetic V>
      requires(
         (is_integral<U> || is_floating_point<U>)
         && (is_integral<V> || is_floating_point<V>)
         && (is_integral<U> || is_same<raw_type, raw_arithmetic_type<U>>)
         && (is_integral<V> || is_same<raw_type, raw_arithmetic_type<V>>)
      )
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   fma(U multiplier, V addend) const {
      return view().fma(multiplier, addend);
   }

   template <typename U>
      requires(requires(precision_reference lhs, U&& rhs) {
                  lhs.add(static_cast<U&&>(rhs));
               })
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   friend constexpr auto
   operator+(precision_reference lhs, U&& rhs)
      -> decltype(lhs.add(static_cast<U&&>(rhs))) {
      return lhs.add(static_cast<U&&>(rhs));
   }

   template <is_arithmetic U>
      requires(is_floating_point<U>)
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   friend constexpr auto
   operator+(U lhs, precision_reference rhs) -> decltype(rhs.add(lhs)) {
      return rhs.add(lhs);
   }

   template <typename U>
      requires(requires(precision_reference lhs, U&& rhs) {
                  lhs.subtract_by(static_cast<U&&>(rhs));
               })
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   friend constexpr auto
   operator-(precision_reference lhs, U&& rhs)
      -> decltype(lhs.subtract_by(static_cast<U&&>(rhs))) {
      return lhs.subtract_by(static_cast<U&&>(rhs));
   }

   template <is_arithmetic U>
      requires(is_floating_point<U>)
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   friend constexpr auto
   operator-(U lhs, precision_reference rhs)
      -> decltype(rhs.subtract_from(lhs)) {
      return rhs.subtract_from(lhs);
   }

   template <typename U>
      requires(requires(precision_reference lhs, U&& rhs) {
                  lhs.multiply(static_cast<U&&>(rhs));
               })
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   friend constexpr auto
   operator*(precision_reference lhs, U&& rhs)
      -> decltype(lhs.multiply(static_cast<U&&>(rhs))) {
      return lhs.multiply(static_cast<U&&>(rhs));
   }

   template <is_arithmetic U>
      requires(is_floating_point<U>)
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   friend constexpr auto
   operator*(U lhs, precision_reference rhs) -> decltype(rhs.multiply(lhs)) {
      return rhs.multiply(lhs);
   }

   template <typename U>
      requires(requires(precision_reference lhs, U&& rhs) {
                  lhs.divide_by(static_cast<U&&>(rhs));
               })
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   friend constexpr auto
   operator/(precision_reference lhs, U&& rhs)
      -> decltype(lhs.divide_by(static_cast<U&&>(rhs))) {
      return lhs.divide_by(static_cast<U&&>(rhs));
   }

   template <is_arithmetic U>
      requires(is_floating_point<U>)
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   friend constexpr auto
   operator/(U lhs, precision_reference rhs) -> decltype(rhs.divide_into(lhs)) {
      return rhs.divide_into(lhs);
   }

   template <is_arithmetic U>
      requires(
         !is_const<WrappedQual>
         && is_same<
            remove_cvref<
               decltype(declval<precision_reference&>().add(declval<U>()))>,
            basic_float<raw_type, precision, quantity_reference>>
      )
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   operator+=(U operand) -> precision_reference& {
      *this = add(operand);
      return *this;
   }

   template <is_arithmetic U>
      requires(
         !is_const<WrappedQual>
         && is_same<
            remove_cvref<decltype(declval<precision_reference&>()
                                     .subtract_by(declval<U>()))>,
            basic_float<raw_type, precision, quantity_reference>>
      )
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   operator-=(U operand) -> precision_reference& {
      *this = subtract_by(operand);
      return *this;
   }

   template <is_arithmetic U>
      requires(
         !is_const<WrappedQual>
         && is_same<
            remove_cvref<decltype(declval<precision_reference&>()
                                     .multiply(declval<U>()))>,
            basic_float<raw_type, precision, quantity_reference>>
      )
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   operator*=(U operand) -> precision_reference& {
      *this = multiply(operand);
      return *this;
   }

   template <is_arithmetic U>
      requires(
         !is_const<WrappedQual>
         && is_same<
            remove_cvref<decltype(declval<precision_reference&>()
                                     .divide_by(declval<U>()))>,
            basic_float<raw_type, precision, quantity_reference>>
      )
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   operator/=(U operand) -> precision_reference& {
      *this = divide_by(operand);
      return *this;
   }
};

// Implementing this here is a circular dependency. The implementation can be
// found in <cat/arithmetic/implementations/format_precision_reference.tpp>.
template <typename WrappedQual, precision_policies policy, typename CharT>
struct formatter<precision_reference<WrappedQual, policy>, CharT>;

}  // namespace cat
