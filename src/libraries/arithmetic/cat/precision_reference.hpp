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

// The arithmetic interface generates this reference's operators from the hooks
// below. Like every libCat arithmetic type, when both operands could provide an
// operator, the left-hand operand's is selected.
template <is_floating_point Float, precision_policies precision>
class precision_reference
    : public arithmetic_interface<precision_reference<Float, precision>> {
 private:
   template <is_floating_point OtherFloat, precision_policies other_precision>
   friend class precision_reference;

   template <typename, typename>
   friend struct formatter;

 public:
   constexpr explicit precision_reference(Float& w) noexcept
       : m_wrapped(addressof(w)) {
   }

   // P3207R0. This type is reference-like.
   friend constexpr void
   operator&(precision_reference const volatile&) = delete;

   friend constexpr void
   operator&(precision_reference const volatile&&) = delete;

   constexpr precision_reference(precision_reference const&) = default;

   // Assigning another reference of this type writes its value through, as
   // assigning a built-in reference does. `rebind()` changes the address.
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   operator=(precision_reference const& other) noexcept -> precision_reference&
      requires(!is_const<Float>)
   {
      *m_wrapped = *other.m_wrapped;
      return *this;
   }

   using float_type = __typeof_unqual(Float);
   using raw_type = __typeof_unqual(declval<Float>().raw);
   using quantity_type = float_type::quantity_type;
   static constexpr auto quantity_reference = float_type::quantity_reference;

   static constexpr precision_policies precision_policy = precision;

   // Rebind this reference wrapper to a different address.
   constexpr void
   rebind(Float& w [[clang::lifetime_capture_by_this]]) noexcept {
      m_wrapped = addressof(w);
   }

 private:
   Float* _Nonnull m_wrapped;

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   view() const noexcept
      -> basic_float<raw_type, precision, quantity_reference> {
      return basic_float<raw_type, precision, quantity_reference>(
         m_wrapped->raw
      );
   }

 public:
   constexpr auto
   precise() & noexcept
      -> precision_reference<Float, precision_policies::precise> {
      return precision_reference<Float, precision_policies::precise>(
         *m_wrapped
      );
   }

   constexpr auto
   precise() const& noexcept
      -> precision_reference<Float const, precision_policies::precise> {
      return precision_reference<Float const, precision_policies::precise>(
         *m_wrapped
      );
   }

   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   precise() const&& noexcept -> basic_float<
      raw_type, precision_policies::precise, quantity_reference> {
      return basic_float<
         raw_type, precision_policies::precise, quantity_reference>(
         m_wrapped->raw
      );
   }

   constexpr auto
   fast() & noexcept -> precision_reference<Float, precision_policies::fast> {
      return precision_reference<Float, precision_policies::fast>(*m_wrapped);
   }

   constexpr auto
   fast() const& noexcept
      -> precision_reference<Float const, precision_policies::fast> {
      return precision_reference<Float const, precision_policies::fast>(
         *m_wrapped
      );
   }

   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   fast() const&& noexcept
      -> basic_float<raw_type, precision_policies::fast, quantity_reference> {
      return basic_float<
         raw_type, precision_policies::fast, quantity_reference>(
         m_wrapped->raw
      );
   }

   // A reference converts implicitly into its own value with this policy, and
   // explicitly into anything else.
   [[gnu::always_inline, gnu::nodebug]]
   constexpr
   operator auto() const noexcept {
      return view();
   }

   template <typename U>
      requires(
         !is_same<U, basic_float<raw_type, precision, quantity_reference>>
      )
   [[gnu::always_inline, gnu::nodebug]]
   constexpr explicit
   operator U() const noexcept {
      return static_cast<U>(view());
   }

   template <is_arithmetic U>
      requires(is_safe_arithmetic_comparison<raw_type, U>)
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   operator<=>(U rhs) const noexcept {
      return view() <=> rhs;
   }

   template <is_floating_point OtherFloat, precision_policies other_precision>
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   operator<=>(
      precision_reference<OtherFloat, other_precision> rhs
   ) const noexcept -> std::partial_ordering {
      return view() <=> rhs.view();
   }

   template <is_arithmetic U>
      requires(is_safe_arithmetic_comparison<raw_type, U>)
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   friend constexpr auto
   operator==(precision_reference lhs, U rhs) noexcept -> bool {
      return lhs.view() == rhs;
   }

   template <is_floating_point OtherFloat, precision_policies other_precision>
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   friend constexpr auto
   operator==(
      precision_reference lhs,
      precision_reference<OtherFloat, other_precision> rhs
   ) noexcept -> bool {
      return lhs.view() == rhs.view();
   }

   // Narrowing assignment is rejected, so compound assignments that promote
   // are too.
   template <is_arithmetic U>
      requires(
         is_safe_arithmetic_comparison<raw_type, U>
         && is_safe_arithmetic_conversion<raw_arithmetic_type<U>, raw_type>
         && !is_const<Float>
      )
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   operator=(U operand) noexcept -> precision_reference& {
      *m_wrapped = operand;
      return *this;
   }

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   negate() const noexcept {
      return -view();
   }

   template <is_floating_point U>
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   add(U other) const noexcept {
      return view().add(other);
   }

   template <is_floating_point OtherFloat, precision_policies other_precision>
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   add(precision_reference<OtherFloat, other_precision> other) const noexcept {
      return view().add(other.view());
   }

   template <is_floating_point U>
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   subtract_by(U operand) const noexcept {
      return view().subtract_by(operand);
   }

   template <is_floating_point OtherFloat, precision_policies other_precision>
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   subtract_by(
      precision_reference<OtherFloat, other_precision> operand
   ) const noexcept {
      return view().subtract_by(operand.view());
   }

   template <is_floating_point U>
      requires(detail::is_quantity_addable<
               U, basic_float<raw_type, precision, quantity_reference>>)
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   subtract_from(U operand) const noexcept {
      return view().subtract_from(operand);
   }

   template <is_floating_point U>
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   multiply(U operand) const noexcept {
      return view().multiply(operand);
   }

   template <is_floating_point OtherFloat, precision_policies other_precision>
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   multiply(
      precision_reference<OtherFloat, other_precision> operand
   ) const noexcept {
      return view().multiply(operand.view());
   }

   template <is_floating_point U>
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   divide_by(U operand) const noexcept {
      return view().divide_by(operand);
   }

   template <is_floating_point OtherFloat, precision_policies other_precision>
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   divide_by(
      precision_reference<OtherFloat, other_precision> operand
   ) const noexcept {
      return view().divide_by(operand.view());
   }

   template <is_raw_arithmetic U>
      requires(is_floating_point<U>)
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   divide_into(U operand) const noexcept {
      return view().divide_into(operand);
   }

   template <is_floating_point U, is_floating_point V>
      requires(
         is_same<raw_type, raw_arithmetic_type<U>>
         && is_same<raw_type, raw_arithmetic_type<V>>
      )
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   fma(U multiplier, V addend) const noexcept {
      return view().fma(multiplier, addend);
   }
};

// Implementing this here is a circular dependency. The implementation can be
// found in <cat/arithmetic/implementations/format_precision_reference.tpp>.
template <is_floating_point Float, precision_policies policy, typename CharT>
struct formatter<precision_reference<Float, policy>, CharT>;

// A reference is a floating-point number of the type it refers to, with its
// own policy.
namespace detail {
template <is_floating_point Float, precision_policies policy>
inline constexpr bool
   is_arithmetic_reference<precision_reference<Float, policy>> = true;

template <is_floating_point Float, precision_policies policy>
inline constexpr bool
   is_floating_point_detail<precision_reference<Float, policy>> = true;

template <is_floating_point Float, precision_policies policy>
inline constexpr bool is_signed_detail<precision_reference<Float, policy>> =
   true;
}  // namespace detail

template <is_floating_point Float, precision_policies policy>
inline constexpr precision_policies
   make_precision_policy<precision_reference<Float, policy>> = policy;

}  // namespace cat
