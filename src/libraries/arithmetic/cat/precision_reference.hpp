// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic_interface>

namespace cat {

namespace detail {

template <typename T, precision_policies fallback>
inline constexpr precision_policies float_precision_for = fallback;

template <typename T, precision_policies precision, precision_policies fallback>
inline constexpr precision_policies
   float_precision_for<basic_float<T, precision>, fallback> = precision;

template <typename T, typename U, precision_policies precision>
using precision_reference_reverse_result =
   basic_float<common_type<raw_arithmetic_type<T>, raw_arithmetic_type<U>>,
               float_precision_for<remove_cvref<T>, precision>>;

}  // namespace detail

template <typename WrappedQual, precision_policies precision>
class precision_reference {
   template <typename, precision_policies>
   friend class precision_reference;

 public:
   constexpr explicit precision_reference(WrappedQual& w)
       : m_wrapped(__builtin_addressof(w)) {
   }

   using raw_type = remove_cvref<WrappedQual>::raw_type;

   static constexpr precision_policies precision_policy = precision;

 private:
   WrappedQual* m_wrapped;

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   view() const -> basic_float<raw_type, precision> {
      return basic_float<raw_type, precision>(m_wrapped->raw);
   }

 public:
   constexpr auto
   precise() & -> precision_reference<WrappedQual,
                                      precision_policies::precise> {
      return precision_reference<WrappedQual, precision_policies::precise>(
         *m_wrapped);
   }

   constexpr auto
   precise() const& -> precision_reference<WrappedQual const,
                                           precision_policies::precise> {
      return precision_reference<WrappedQual const,
                                 precision_policies::precise>(*m_wrapped);
   }

   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   precise() && -> basic_float<raw_type, precision_policies::precise> {
      return basic_float<raw_type, precision_policies::precise>(m_wrapped->raw);
   }

   constexpr auto
   fast() & -> precision_reference<WrappedQual, precision_policies::fast> {
      return precision_reference<WrappedQual, precision_policies::fast>(
         *m_wrapped);
   }

   constexpr auto
   fast() const& -> precision_reference<WrappedQual const,
                                        precision_policies::fast> {
      return precision_reference<WrappedQual const, precision_policies::fast>(
         *m_wrapped);
   }

   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   fast() && -> basic_float<raw_type, precision_policies::fast> {
      return basic_float<raw_type, precision_policies::fast>(m_wrapped->raw);
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
      using other_raw_type =
         precision_reference<OtherWrappedQual, other_precision>::raw_type;
      using common = common_type<raw_type, other_raw_type>;
      return detail::float_compare_three_way<precision>(
         common(m_wrapped->raw), common(rhs.m_wrapped->raw));
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
   operator==(precision_reference lhs,
              precision_reference<OtherWrappedQual, other_precision> rhs)
      -> bool {
      using other_raw_type =
         precision_reference<OtherWrappedQual, other_precision>::raw_type;
      using common = common_type<raw_type, other_raw_type>;
      return detail::float_equal<precision>(common(lhs.m_wrapped->raw),
                                            common(rhs.m_wrapped->raw));
   }

   template <is_arithmetic U>
      requires(is_safe_arithmetic_comparison<raw_type, U>
               && !is_const<WrappedQual>)
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
   add(precision_reference<OtherWrappedQual, other_precision> other) const
      -> basic_float<
         common_type<raw_type, typename precision_reference<
                                  OtherWrappedQual, other_precision>::raw_type>,
         precision> {
      using other_raw_type =
         precision_reference<OtherWrappedQual, other_precision>::raw_type;
      using result_type =
         basic_float<common_type<raw_type, other_raw_type>, precision>;
      return result_type(
         detail::float_add<precision>(m_wrapped->raw, other.m_wrapped->raw));
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
   subtract_by(precision_reference<OtherWrappedQual, other_precision> operand)
      const -> basic_float<
         common_type<raw_type, typename precision_reference<
                                  OtherWrappedQual, other_precision>::raw_type>,
         precision> {
      using other_raw_type =
         precision_reference<OtherWrappedQual, other_precision>::raw_type;
      using result_type =
         basic_float<common_type<raw_type, other_raw_type>, precision>;
      return result_type(detail::float_subtract<precision>(
         m_wrapped->raw, operand.m_wrapped->raw));
   }

   template <is_arithmetic U>
      requires(is_floating_point<U>)
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   subtract_from(U operand) const {
      using result_type =
         detail::precision_reference_reverse_result<U, raw_type, precision>;
      constexpr precision_policies left_precision =
         detail::float_precision_for<remove_cvref<U>, precision>;
      return result_type(detail::float_subtract<left_precision>(
         make_raw_arithmetic(operand), m_wrapped->raw));
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
   multiply(precision_reference<OtherWrappedQual, other_precision> operand)
      const -> basic_float<
         common_type<raw_type, typename precision_reference<
                                  OtherWrappedQual, other_precision>::raw_type>,
         precision> {
      using other_raw_type =
         precision_reference<OtherWrappedQual, other_precision>::raw_type;
      using result_type =
         basic_float<common_type<raw_type, other_raw_type>, precision>;
      return result_type(detail::float_multiply<precision>(
         m_wrapped->raw, operand.m_wrapped->raw));
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
   divide_by(precision_reference<OtherWrappedQual, other_precision> operand)
      const -> basic_float<
         common_type<raw_type, typename precision_reference<
                                  OtherWrappedQual, other_precision>::raw_type>,
         precision> {
      using other_raw_type =
         precision_reference<OtherWrappedQual, other_precision>::raw_type;
      using result_type =
         basic_float<common_type<raw_type, other_raw_type>, precision>;
      return result_type(detail::float_divide<precision>(
         m_wrapped->raw, operand.m_wrapped->raw));
   }

   template <is_unsafe_arithmetic U>
      requires(is_floating_point<U>)
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   divide_into(U operand) const {
      using result_type =
         detail::precision_reference_reverse_result<U, raw_type, precision>;
      constexpr precision_policies left_precision =
         detail::float_precision_for<remove_cvref<U>, precision>;
      return result_type(detail::float_divide<left_precision>(
         make_raw_arithmetic(operand), m_wrapped->raw));
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
      -> detail::precision_reference_reverse_result<U, raw_type, precision> {
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
   operator/(U lhs, precision_reference rhs)
      -> detail::precision_reference_reverse_result<U, raw_type, precision> {
      return rhs.divide_into(lhs);
   }

   template <is_arithmetic U>
      requires(!is_const<WrappedQual>
               && is_same<remove_cvref<decltype(declval<precision_reference&>()
                                                   .add(declval<U>()))>,
                          basic_float<raw_type, precision>>)
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   operator+=(U operand) -> precision_reference& {
      *this = add(operand);
      return *this;
   }

   template <is_arithmetic U>
      requires(!is_const<WrappedQual>
               && is_same<remove_cvref<decltype(declval<precision_reference&>()
                                                   .subtract_by(declval<U>()))>,
                          basic_float<raw_type, precision>>)
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   operator-=(U operand) -> precision_reference& {
      *this = subtract_by(operand);
      return *this;
   }

   template <is_arithmetic U>
      requires(!is_const<WrappedQual>
               && is_same<remove_cvref<decltype(declval<precision_reference&>()
                                                   .multiply(declval<U>()))>,
                          basic_float<raw_type, precision>>)
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   operator*=(U operand) -> precision_reference& {
      *this = multiply(operand);
      return *this;
   }

   template <is_arithmetic U>
      requires(!is_const<WrappedQual>
               && is_same<remove_cvref<decltype(declval<precision_reference&>()
                                                   .divide_by(declval<U>()))>,
                          basic_float<raw_type, precision>>)
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   operator/=(U operand) -> precision_reference& {
      *this = divide_by(operand);
      return *this;
   }
};

}  // namespace cat
