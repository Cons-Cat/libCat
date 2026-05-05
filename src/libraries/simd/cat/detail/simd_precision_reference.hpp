// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>

namespace cat::detail {

template <precision_policies precision, typename T, typename Abi>
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
simd_float_add_forced(cat::simd<T, Abi> const& left,
                      cat::simd<T, Abi> const& right) -> cat::simd<T, Abi> {
   return cat::simd<T, Abi>(float_add<precision>(left.raw, right.raw));
}

template <precision_policies precision, typename T, typename Abi>
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
simd_float_sub_forced(cat::simd<T, Abi> const& left,
                      cat::simd<T, Abi> const& right) -> cat::simd<T, Abi> {
   return cat::simd<T, Abi>(float_subtract<precision>(left.raw, right.raw));
}

template <precision_policies precision, typename T, typename Abi>
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
simd_float_mul_forced(cat::simd<T, Abi> const& left,
                      cat::simd<T, Abi> const& right) -> cat::simd<T, Abi> {
   return cat::simd<T, Abi>(float_multiply<precision>(left.raw, right.raw));
}

template <precision_policies precision, typename T, typename Abi>
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
simd_float_div_forced(cat::simd<T, Abi> const& left,
                      cat::simd<T, Abi> const& right) -> cat::simd<T, Abi> {
   return cat::simd<T, Abi>(float_divide<precision>(left.raw, right.raw));
}

template <typename WrappedQual, precision_policies precision>
class simd_precision_reference
    : public arithmetic_interface<simd_precision_reference<WrappedQual,
                                                           precision>> {
   using wrapper_type = remove_cvref<WrappedQual>;
   using T = wrapper_type::value_type;
   using abi_type = wrapper_type::abi_type;
   using result_scalar = simd_precision_scalar<T, precision>::type;
   using result_type =
      cat::simd<result_scalar,
                typename abi_type::template make_abi_type<result_scalar>>;

   template <typename LeftT, typename LeftAbi>
   using left_result_scalar =
      simd_precision_scalar<LeftT, simd_float_precision_policy<LeftT>>::type;

   template <typename LeftT, typename LeftAbi>
   using left_result_type =
      cat::simd<left_result_scalar<LeftT, LeftAbi>,
                typename LeftAbi::template make_abi_type<
                   left_result_scalar<LeftT, LeftAbi>>>;

   using result_mask =
      cat::simd_mask<result_scalar,
                     typename abi_type::template make_abi_type<result_scalar>>;

   template <typename LeftT, typename LeftAbi>
   using left_result_mask =
      cat::simd_mask<left_result_scalar<LeftT, LeftAbi>,
                     typename LeftAbi::template make_abi_type<
                        left_result_scalar<LeftT, LeftAbi>>>;

   WrappedQual* m_wrapped;

   template <typename Result, typename Value>
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   static constexpr auto
   make_simd_value(Value const& value) -> Result {
      return Result(__builtin_bit_cast(typename Result::raw_type, value.raw));
   }

   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   assign_result(result_type value) -> void
      requires(!is_const<WrappedQual>)
   {
      m_wrapped->raw =
         __builtin_bit_cast(typename wrapper_type::raw_type, value.raw);
   }

 public:
   constexpr explicit simd_precision_reference(WrappedQual& w)
       : m_wrapped(__builtin_addressof(w)) {
   }

   template <typename OtherT, typename OtherAbi>
      requires(OtherAbi::lanes == abi_type::lanes
               && sizeof(raw_arithmetic_type<OtherT>)
                     == sizeof(raw_arithmetic_type<T>))
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   equal_lanes(cat::simd<OtherT, OtherAbi> const& rhs) const -> result_mask {
      result_type const left =
         make_simd_value<result_type>(*m_wrapped);
      result_type const right = make_simd_value<result_type>(rhs);
      return result_mask(simd_float_equal<precision>(left.raw, right.raw));
   }

   template <typename OtherT, typename OtherAbi>
      requires(OtherAbi::lanes == abi_type::lanes
               && sizeof(raw_arithmetic_type<OtherT>)
                     == sizeof(raw_arithmetic_type<T>))
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   unequal_lanes(cat::simd<OtherT, OtherAbi> const& rhs) const -> result_mask {
      result_type const left =
         make_simd_value<result_type>(*m_wrapped);
      result_type const right = make_simd_value<result_type>(rhs);
      return result_mask(simd_float_unequal<precision>(left.raw, right.raw));
   }

   template <typename OtherT, typename OtherAbi>
      requires(OtherAbi::lanes == abi_type::lanes
               && sizeof(raw_arithmetic_type<OtherT>)
                     == sizeof(raw_arithmetic_type<T>))
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   less(cat::simd<OtherT, OtherAbi> const& rhs) const -> result_mask {
      result_type const left =
         make_simd_value<result_type>(*m_wrapped);
      result_type const right = make_simd_value<result_type>(rhs);
      return result_mask(simd_float_less<precision>(left.raw, right.raw));
   }

   template <typename OtherT, typename OtherAbi>
      requires(OtherAbi::lanes == abi_type::lanes
               && sizeof(raw_arithmetic_type<OtherT>)
                     == sizeof(raw_arithmetic_type<T>))
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   less_equal(cat::simd<OtherT, OtherAbi> const& rhs) const -> result_mask {
      result_type const left =
         make_simd_value<result_type>(*m_wrapped);
      result_type const right = make_simd_value<result_type>(rhs);
      return result_mask(
         simd_float_less_equal<precision>(left.raw, right.raw));
   }

   template <typename OtherT, typename OtherAbi>
      requires(OtherAbi::lanes == abi_type::lanes
               && sizeof(raw_arithmetic_type<OtherT>)
                     == sizeof(raw_arithmetic_type<T>))
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   greater(cat::simd<OtherT, OtherAbi> const& rhs) const -> result_mask {
      result_type const left =
         make_simd_value<result_type>(*m_wrapped);
      result_type const right = make_simd_value<result_type>(rhs);
      return result_mask(simd_float_greater<precision>(left.raw, right.raw));
   }

   template <typename OtherT, typename OtherAbi>
      requires(OtherAbi::lanes == abi_type::lanes
               && sizeof(raw_arithmetic_type<OtherT>)
                     == sizeof(raw_arithmetic_type<T>))
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   greater_equal(cat::simd<OtherT, OtherAbi> const& rhs) const -> result_mask {
      result_type const left =
         make_simd_value<result_type>(*m_wrapped);
      result_type const right = make_simd_value<result_type>(rhs);
      return result_mask(
         simd_float_greater_equal<precision>(left.raw, right.raw));
   }

   constexpr auto
   precise() & -> simd_precision_reference<WrappedQual,
                                           precision_policies::precise> {
      return simd_precision_reference<WrappedQual,
                                      precision_policies::precise>(*m_wrapped);
   }

   constexpr auto
   precise() const& -> simd_precision_reference<WrappedQual const,
                                                precision_policies::precise> {
      return simd_precision_reference<WrappedQual const,
                                      precision_policies::precise>(*m_wrapped);
   }

   constexpr auto
   fast() & -> simd_precision_reference<WrappedQual,
                                        precision_policies::fast> {
      return simd_precision_reference<WrappedQual, precision_policies::fast>(
         *m_wrapped);
   }

   constexpr auto
   fast() const& -> simd_precision_reference<WrappedQual const,
                                             precision_policies::fast> {
      return simd_precision_reference<WrappedQual const,
                                      precision_policies::fast>(*m_wrapped);
   }

   template <typename OtherT, typename OtherAbi>
      requires(OtherAbi::lanes == abi_type::lanes
               && sizeof(raw_arithmetic_type<OtherT>)
                     == sizeof(raw_arithmetic_type<T>))
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   add(cat::simd<OtherT, OtherAbi> const& rhs) const -> result_type {
      return simd_float_add_forced<precision>(
         make_simd_value<result_type>(*m_wrapped),
         make_simd_value<result_type>(rhs));
   }

   template <is_arithmetic U>
      requires(simd_broadcast_really_convertible_to<remove_cvref<U>, T>())
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   add(U&& rhs) const -> result_type {
      return add(wrapper_type($fwd(rhs)));
   }

   template <typename OtherT, typename OtherAbi>
      requires(OtherAbi::lanes == abi_type::lanes
               && sizeof(raw_arithmetic_type<OtherT>)
                     == sizeof(raw_arithmetic_type<T>))
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   subtract_by(cat::simd<OtherT, OtherAbi> const& rhs) const -> result_type {
      return simd_float_sub_forced<precision>(
         make_simd_value<result_type>(*m_wrapped),
         make_simd_value<result_type>(rhs));
   }

   template <is_arithmetic U>
      requires(simd_broadcast_really_convertible_to<remove_cvref<U>, T>())
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   subtract_by(U&& rhs) const -> result_type {
      return subtract_by(wrapper_type($fwd(rhs)));
   }

   template <typename OtherT, typename OtherAbi>
      requires(OtherAbi::lanes == abi_type::lanes
               && sizeof(raw_arithmetic_type<OtherT>)
                     == sizeof(raw_arithmetic_type<T>))
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   subtract_from(cat::simd<OtherT, OtherAbi> const& lhs) const
      -> left_result_type<OtherT, OtherAbi> {
      using left_type = left_result_type<OtherT, OtherAbi>;
      constexpr precision_policies left_precision =
         simd_float_precision_policy<OtherT>;
      return simd_float_sub_forced<left_precision>(
         make_simd_value<left_type>(lhs),
         make_simd_value<left_type>(*m_wrapped));
   }

   template <is_arithmetic U>
      requires(simd_broadcast_really_convertible_to<remove_cvref<U>, T>())
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   subtract_from(U&& lhs) const -> result_type {
      return simd_float_sub_forced<precision>(
         result_type($fwd(lhs)), make_simd_value<result_type>(*m_wrapped));
   }

   template <typename OtherT, typename OtherAbi>
      requires(OtherAbi::lanes == abi_type::lanes
               && sizeof(raw_arithmetic_type<OtherT>)
                     == sizeof(raw_arithmetic_type<T>))
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   multiply(cat::simd<OtherT, OtherAbi> const& rhs) const -> result_type {
      return simd_float_mul_forced<precision>(
         make_simd_value<result_type>(*m_wrapped),
         make_simd_value<result_type>(rhs));
   }

   template <is_arithmetic U>
      requires(simd_broadcast_really_convertible_to<remove_cvref<U>, T>())
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   multiply(U&& rhs) const -> result_type {
      return multiply(wrapper_type($fwd(rhs)));
   }

   template <typename OtherT, typename OtherAbi>
      requires(OtherAbi::lanes == abi_type::lanes
               && sizeof(raw_arithmetic_type<OtherT>)
                     == sizeof(raw_arithmetic_type<T>))
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   divide_by(cat::simd<OtherT, OtherAbi> const& rhs) const -> result_type {
      return simd_float_div_forced<precision>(
         make_simd_value<result_type>(*m_wrapped),
         make_simd_value<result_type>(rhs));
   }

   template <is_arithmetic U>
      requires(simd_broadcast_really_convertible_to<remove_cvref<U>, T>())
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   divide_by(U&& rhs) const -> result_type {
      return divide_by(wrapper_type($fwd(rhs)));
   }

   template <typename OtherT, typename OtherAbi>
      requires(OtherAbi::lanes == abi_type::lanes
               && sizeof(raw_arithmetic_type<OtherT>)
                     == sizeof(raw_arithmetic_type<T>))
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   divide_into(cat::simd<OtherT, OtherAbi> const& lhs) const
      -> left_result_type<OtherT, OtherAbi> {
      using left_type = left_result_type<OtherT, OtherAbi>;
      constexpr precision_policies left_precision =
         simd_float_precision_policy<OtherT>;
      return simd_float_div_forced<left_precision>(
         make_simd_value<left_type>(lhs),
         make_simd_value<left_type>(*m_wrapped));
   }

   template <is_arithmetic U>
      requires(simd_broadcast_really_convertible_to<remove_cvref<U>, T>())
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   divide_into(U&& lhs) const -> result_type {
      return simd_float_div_forced<precision>(
         result_type($fwd(lhs)), make_simd_value<result_type>(*m_wrapped));
   }

   template <typename OtherT, typename OtherAbi>
      requires(OtherAbi::lanes == abi_type::lanes
               && sizeof(raw_arithmetic_type<OtherT>)
                     == sizeof(raw_arithmetic_type<T>))
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   friend constexpr auto
   operator<(simd_precision_reference lhs,
             cat::simd<OtherT, OtherAbi> const& rhs) -> result_mask {
      return lhs.less(rhs);
   }

   template <typename OtherT, typename OtherAbi>
      requires(OtherAbi::lanes == abi_type::lanes
               && sizeof(raw_arithmetic_type<OtherT>)
                     == sizeof(raw_arithmetic_type<T>))
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   friend constexpr auto
   operator<=(simd_precision_reference lhs,
              cat::simd<OtherT, OtherAbi> const& rhs) -> result_mask {
      return lhs.less_equal(rhs);
   }

   template <typename OtherT, typename OtherAbi>
      requires(OtherAbi::lanes == abi_type::lanes
               && sizeof(raw_arithmetic_type<OtherT>)
                     == sizeof(raw_arithmetic_type<T>))
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   friend constexpr auto
   operator>(simd_precision_reference lhs,
             cat::simd<OtherT, OtherAbi> const& rhs) -> result_mask {
      return lhs.greater(rhs);
   }

   template <typename OtherT, typename OtherAbi>
      requires(OtherAbi::lanes == abi_type::lanes
               && sizeof(raw_arithmetic_type<OtherT>)
                     == sizeof(raw_arithmetic_type<T>))
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   friend constexpr auto
   operator>=(simd_precision_reference lhs,
              cat::simd<OtherT, OtherAbi> const& rhs) -> result_mask {
      return lhs.greater_equal(rhs);
   }

   template <typename OtherT, typename OtherAbi>
      requires(OtherAbi::lanes == abi_type::lanes
               && sizeof(raw_arithmetic_type<OtherT>)
                     == sizeof(raw_arithmetic_type<T>))
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   friend constexpr auto
   operator<(cat::simd<OtherT, OtherAbi> const& lhs,
             simd_precision_reference rhs) -> left_result_mask<OtherT,
                                                               OtherAbi> {
      using left_type = left_result_type<OtherT, OtherAbi>;
      using mask_type = left_result_mask<OtherT, OtherAbi>;
      constexpr precision_policies left_precision =
         simd_float_precision_policy<OtherT>;
      left_type const left = make_simd_value<left_type>(lhs);
      left_type const right = make_simd_value<left_type>(*rhs.m_wrapped);
      return mask_type(simd_float_less<left_precision>(left.raw, right.raw));
   }

   template <typename OtherT, typename OtherAbi>
      requires(OtherAbi::lanes == abi_type::lanes
               && sizeof(raw_arithmetic_type<OtherT>)
                     == sizeof(raw_arithmetic_type<T>))
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   friend constexpr auto
   operator<=(cat::simd<OtherT, OtherAbi> const& lhs,
              simd_precision_reference rhs) -> left_result_mask<OtherT,
                                                                OtherAbi> {
      using left_type = left_result_type<OtherT, OtherAbi>;
      using mask_type = left_result_mask<OtherT, OtherAbi>;
      constexpr precision_policies left_precision =
         simd_float_precision_policy<OtherT>;
      left_type const left = make_simd_value<left_type>(lhs);
      left_type const right = make_simd_value<left_type>(*rhs.m_wrapped);
      return mask_type(
         simd_float_less_equal<left_precision>(left.raw, right.raw));
   }

   template <typename OtherT, typename OtherAbi>
      requires(OtherAbi::lanes == abi_type::lanes
               && sizeof(raw_arithmetic_type<OtherT>)
                     == sizeof(raw_arithmetic_type<T>))
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   friend constexpr auto
   operator>(cat::simd<OtherT, OtherAbi> const& lhs,
             simd_precision_reference rhs) -> left_result_mask<OtherT,
                                                               OtherAbi> {
      using left_type = left_result_type<OtherT, OtherAbi>;
      using mask_type = left_result_mask<OtherT, OtherAbi>;
      constexpr precision_policies left_precision =
         simd_float_precision_policy<OtherT>;
      left_type const left = make_simd_value<left_type>(lhs);
      left_type const right = make_simd_value<left_type>(*rhs.m_wrapped);
      return mask_type(
         simd_float_greater<left_precision>(left.raw, right.raw));
   }

   template <typename OtherT, typename OtherAbi>
      requires(OtherAbi::lanes == abi_type::lanes
               && sizeof(raw_arithmetic_type<OtherT>)
                     == sizeof(raw_arithmetic_type<T>))
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   friend constexpr auto
   operator>=(cat::simd<OtherT, OtherAbi> const& lhs,
              simd_precision_reference rhs) -> left_result_mask<OtherT,
                                                                OtherAbi> {
      using left_type = left_result_type<OtherT, OtherAbi>;
      using mask_type = left_result_mask<OtherT, OtherAbi>;
      constexpr precision_policies left_precision =
         simd_float_precision_policy<OtherT>;
      left_type const left = make_simd_value<left_type>(lhs);
      left_type const right = make_simd_value<left_type>(*rhs.m_wrapped);
      return mask_type(
         simd_float_greater_equal<left_precision>(left.raw, right.raw));
   }

   template <typename U>
      requires(!is_const<WrappedQual> && requires(simd_precision_reference self,
                                                  U&& operand) {
                  self + static_cast<U&&>(operand);
               })
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   operator+=(U&& operand) -> simd_precision_reference& {
      assign_result(*this + $fwd(operand));
      return *this;
   }

   template <typename U>
      requires(!is_const<WrappedQual> && requires(simd_precision_reference self,
                                                  U&& operand) {
                  self - static_cast<U&&>(operand);
               })
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   operator-=(U&& operand) -> simd_precision_reference& {
      assign_result(*this - $fwd(operand));
      return *this;
   }

   template <typename U>
      requires(!is_const<WrappedQual> && requires(simd_precision_reference self,
                                                  U&& operand) {
                  self * static_cast<U&&>(operand);
               })
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   operator*=(U&& operand) -> simd_precision_reference& {
      assign_result(*this * $fwd(operand));
      return *this;
   }

   template <typename U>
      requires(!is_const<WrappedQual> && requires(simd_precision_reference self,
                                                  U&& operand) {
                  self / static_cast<U&&>(operand);
               })
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   operator/=(U&& operand) -> simd_precision_reference& {
      assign_result(*this / $fwd(operand));
      return *this;
   }
};

}  // namespace cat::detail
