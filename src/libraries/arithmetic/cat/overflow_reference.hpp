// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic_interface>

// Included by <cat/arithmetic> immediately after `arithmetic` is defined.

namespace cat {

// This proxy reference wraps an arithmetic type with specific overflow
// semantics. It is intended to be obtained from the overflow accessors of
// `cat::arithmetic`, `cat::index`, and `cat::arithmetic_ptr` (`.undef()`,
// `.wrap()`, `.sat()`), but is also a public type so that users can store,
// pass, or return it.
//
// In the past, we had expressed overflow accessors through `union` punning and
// `reinterpret_cast`. These approaches violated lifetime semantics and were not
// `constexpr`-friendly.
template <typename WrappedQual, overflow_policies overflow_policy>
class overflow_reference
    : public arithmetic_interface<
         overflow_reference<WrappedQual, overflow_policy>> {
 public:
   constexpr explicit overflow_reference(WrappedQual& w)
       : m_wrapped(__builtin_addressof(w)) {
   }

   using raw_type = typename remove_cvref<WrappedQual>::raw_type;

 private:
   using wrapper_type = remove_cvref<WrappedQual>;

   static constexpr overflow_policies policy = overflow_policy;

   WrappedQual* m_wrapped;

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   view() const {
      if constexpr (detail::is_arithmetic_ptr<wrapper_type>) {
         using pointee = remove_pointer<typename wrapper_type::ptr>;
         return arithmetic_ptr<pointee, raw_type, overflow_policy>(
            m_wrapped->raw);
      } else if constexpr (detail::is_idx<wrapper_type>) {
         return index<overflow_policy>(m_wrapped->raw);
      } else {
         return arithmetic<raw_type, overflow_policy>(m_wrapped->raw);
      }
   }

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   view_undef_cmp() const {
      if constexpr (detail::is_arithmetic_ptr<wrapper_type>) {
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
   undef() & -> overflow_reference<WrappedQual, overflow_policies::undefined> {
      return overflow_reference<WrappedQual, overflow_policies::undefined>(
         *m_wrapped);
   }

   constexpr auto
   undef() const& -> overflow_reference<WrappedQual const,
                                        overflow_policies::undefined> {
      return overflow_reference<WrappedQual const,
                                overflow_policies::undefined>(*m_wrapped);
   }

   constexpr auto
   wrap() & -> overflow_reference<WrappedQual, overflow_policies::wrap> {
      return overflow_reference<WrappedQual, overflow_policies::wrap>(
         *m_wrapped);
   }

   constexpr auto
   wrap()
      const& -> overflow_reference<WrappedQual const, overflow_policies::wrap> {
      return overflow_reference<WrappedQual const, overflow_policies::wrap>(
         *m_wrapped);
   }

   constexpr auto
   sat() & -> overflow_reference<WrappedQual, overflow_policies::saturate> {
      return overflow_reference<WrappedQual, overflow_policies::saturate>(
         *m_wrapped);
   }

   constexpr auto
   sat() const& -> overflow_reference<WrappedQual const,
                                      overflow_policies::saturate> {
      return overflow_reference<WrappedQual const, overflow_policies::saturate>(
         *m_wrapped);
   }

   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   undef() && {
      if constexpr (detail::is_arithmetic_ptr<wrapper_type>) {
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
      if constexpr (detail::is_arithmetic_ptr<wrapper_type>) {
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
      if constexpr (detail::is_arithmetic_ptr<wrapper_type>) {
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
      requires(!detail::is_arithmetic_ptr<wrapper_type>
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
      return detail::raw_mixed_integral_equals(lhs.m_wrapped->raw,
                                               make_raw_arithmetic(rhs));
   }

   // Same-signed-ness <=> / == for `arithmetic_ptr`-wrapped references.
   template <is_integral U>
      requires(is_safe_arithmetic_comparison<raw_type, U>
               && detail::is_arithmetic_ptr<wrapper_type>)
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   friend constexpr auto
   operator<=>(overflow_reference lhs, U rhs) {
      return lhs.m_wrapped->raw <=> make_raw_arithmetic(rhs);
   }

   template <is_integral U>
      requires(is_safe_arithmetic_comparison<raw_type, U>
               && detail::is_arithmetic_ptr<wrapper_type>)
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   friend constexpr auto
   operator==(overflow_reference lhs, U rhs) -> bool {
      return lhs.m_wrapped->raw == make_raw_arithmetic(rhs);
   }

   // Mixed-signed-ness <=> / == for `arithmetic_ptr`-wrapped references.
   template <is_integral U>
      requires(!is_safe_arithmetic_comparison<raw_type, U>
               && detail::is_arithmetic_ptr<wrapper_type>
               && ((is_unsigned_integral<raw_type> && is_signed_integral<U>)
                   || (is_signed_integral<raw_type>
                       && is_unsigned_integral<U>)))
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   friend constexpr auto
   operator<=>(overflow_reference lhs, U rhs) {
      return detail::raw_mixed_integral_spaceship(lhs.m_wrapped->raw,
                                                  make_raw_arithmetic(rhs));
   }

   template <is_integral U>
      requires(!is_safe_arithmetic_comparison<raw_type, U>
               && detail::is_arithmetic_ptr<wrapper_type>
               && ((is_unsigned_integral<raw_type> && is_signed_integral<U>)
                   || (is_signed_integral<raw_type>
                       && is_unsigned_integral<U>)))
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   friend constexpr auto
   operator==(overflow_reference lhs, U rhs) -> bool {
      return detail::raw_mixed_integral_equals(lhs.m_wrapped->raw,
                                               make_raw_arithmetic(rhs));
   }

   template <is_arithmetic U>
      requires(is_safe_arithmetic_comparison<raw_type, U>
               && !detail::is_arithmetic_ptr<wrapper_type>
               && !detail::is_idx<wrapper_type>)
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   operator<=>(U rhs) const {
      return view_undef_cmp() <=> rhs;
   }

   template <is_arithmetic U>
      requires(is_safe_arithmetic_comparison<raw_type, U>
               && !detail::is_arithmetic_ptr<wrapper_type>
               && !detail::is_idx<wrapper_type>)
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   friend constexpr auto
   operator==(overflow_reference lhs, U rhs) -> bool {
      return *lhs.m_wrapped == make_raw_arithmetic(rhs);
   }

   // Mixed-signed-ness <=> / ==. The body widens to `common_type` after peeling
   // off the negative-signed-vs-unsigned case, so this works for any
   // combination of widths and runs at runtime as well as `consteval`.
   template <is_arithmetic U>
      requires(!is_safe_arithmetic_comparison<raw_type, U>
               && !detail::is_idx<overflow_reference>
               && ((is_unsigned_integral<raw_type> && is_signed_integral<U>)
                   || (is_signed_integral<raw_type> && is_unsigned_integral<U>))
               && !detail::is_arithmetic_ptr<wrapper_type>
               && !detail::is_idx<wrapper_type>)
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   friend constexpr auto
   operator<=>(overflow_reference lhs, U rhs) {
      return detail::raw_mixed_integral_spaceship(lhs.m_wrapped->raw,
                                                  make_raw_arithmetic(rhs));
   }

   template <is_arithmetic U>
      requires(!is_safe_arithmetic_comparison<raw_type, U>
               && !detail::is_idx<overflow_reference>
               && ((is_unsigned_integral<raw_type> && is_signed_integral<U>)
                   || (is_signed_integral<raw_type> && is_unsigned_integral<U>))
               && !detail::is_arithmetic_ptr<wrapper_type>
               && !detail::is_idx<wrapper_type>)
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   friend constexpr auto
   operator==(overflow_reference lhs, U rhs) -> bool {
      return detail::raw_mixed_integral_equals(lhs.m_wrapped->raw,
                                               make_raw_arithmetic(rhs));
   }

   // Assignment from a compile-time-known raw integer that fits `raw_type`.
   // Writes through `raw` directly because `index`'s only assignment path is
   // implicit-constructor + default copy assign, whose `enable_if` requires a
   // constant expression at the call site (lost once `operand` becomes a
   // function parameter here).
   template <is_integral U>
      requires(!is_const<WrappedQual> && detail::is_idx<wrapper_type>)
   [[gnu::always_inline, gnu::nodebug]]
   constexpr overflow_reference&
   operator=(U other) __attribute__((
      enable_if(detail::raw_source_fits_implicit_storage<raw_type>(other),
                "value is out of range for assignment to cat::index"))) {
      m_wrapped->raw = static_cast<raw_type>(make_raw_arithmetic(other));
      return *this;
   }

   template <is_arithmetic U>
      requires(is_safe_arithmetic_comparison<raw_type, U>
               && !is_const<WrappedQual>
               && !detail::is_arithmetic_ptr<wrapper_type>
               && !detail::is_idx<wrapper_type>)
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   operator=(U operand) -> overflow_reference& {
      *m_wrapped = operand;
      return *this;
   }

   // Cross-signed-ness assignment for compile-time-known operands that fit the
   // wrapped storage. Mirrors the `arithmetic` `enable_if` constructor.
   template <is_arithmetic U>
      requires(!is_safe_arithmetic_comparison<raw_type, U>
               && !is_const<WrappedQual>
               && !detail::is_arithmetic_ptr<wrapper_type>
               && !detail::is_idx<wrapper_type>)
   [[gnu::always_inline, gnu::nodebug]]
   constexpr overflow_reference&
   operator=(U operand) __attribute__((
      enable_if(detail::raw_source_fits_implicit_storage<raw_type>(operand),
                "value is out of range for implicit assignment to this "
                "arithmetic type"))) {
      *m_wrapped = arithmetic<raw_type, overflow_policy>(operand);
      return *this;
   }

   template <is_integral U>
      requires(is_safe_arithmetic_conversion<U, raw_type>
               && !is_const<WrappedQual>
               && detail::is_arithmetic_ptr<wrapper_type>)
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   operator=(U operand) -> overflow_reference& {
      *m_wrapped = operand;
      return *this;
   }

   template <is_arithmetic U>
      requires(is_safe_arithmetic_comparison<raw_type, U>
               && !is_const<WrappedQual>
               && !detail::is_arithmetic_ptr<wrapper_type>
               && !detail::is_idx<wrapper_type>)
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   operator+=(U operand) -> overflow_reference& {
      *this = add(operand);
      return *this;
   }

   // -=. The `arithmetic_interface`-provided friend `operator-=` only fires
   // when `subtract_by` returns `Derived`, which is never true for
   // `overflow_reference` (it always returns the underlying `arithmetic`). We
   // mirror the += family with a dedicated member that writes the result back
   // through the wrapped storage. SFINAE intentionally rejects the promoting
   // case. `subtract_by` returns a wider type for signed undefined LHS with a
   // wider RHS, and that wider value would silently narrow to the LHS storage
   // on assignment.
   template <is_arithmetic U>
      requires(is_safe_arithmetic_comparison<raw_type, U>
               && !is_const<WrappedQual>
               && !detail::is_arithmetic_ptr<wrapper_type>
               && !detail::is_idx<wrapper_type>
               && is_same<remove_cvref<decltype(declval<overflow_reference&>()
                                                   .subtract_by(declval<U>()))>,
                          arithmetic<raw_type, overflow_policy>>)
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   operator-=(U operand) -> overflow_reference& {
      *this = subtract_by(operand);
      return *this;
   }

   // Cross-signed-ness -= for compile-time-known operands that fit the wrapped
   // storage. Mirrors the += overload above.
   template <is_arithmetic U>
      requires(!is_safe_arithmetic_comparison<raw_type, U>
               && !is_const<WrappedQual>
               && !detail::is_arithmetic_ptr<wrapper_type>
               && !detail::is_idx<wrapper_type>
               && is_same<remove_cvref<decltype(declval<overflow_reference&>()
                                                   .subtract_by(declval<U>()))>,
                          arithmetic<raw_type, overflow_policy>>)
   [[gnu::always_inline, gnu::nodebug]]
   constexpr overflow_reference&
   operator-=(U operand) __attribute__((
      enable_if(detail::raw_source_fits_implicit_storage<raw_type>(operand),
                "value is out of range for implicit operation on this "
                "arithmetic type"))) {
      *this = subtract_by(arithmetic<raw_type, overflow_policy>(operand));
      return *this;
   }

   // Cross-signed-ness += for compile-time-known operands that fit the wrapped
   // storage. The body converts `operand` into the wrapped storage first so the
   // inner `add` call goes through the same-signed-ness path (the
   // `constexpr`-ness needed by `enable_if` is consumed at the call site, not
   // inside the body).
   template <is_arithmetic U>
      requires(!is_safe_arithmetic_comparison<raw_type, U>
               && !is_const<WrappedQual>
               && !detail::is_arithmetic_ptr<wrapper_type>
               && !detail::is_idx<wrapper_type>)
   [[gnu::always_inline, gnu::nodebug]]
   constexpr overflow_reference&
   operator+=(U operand) __attribute__((
      enable_if(detail::raw_source_fits_implicit_storage<raw_type>(operand),
                "value is out of range for implicit operation on this "
                "arithmetic type"))) {
      *this = add(arithmetic<raw_type, overflow_policy>(operand));
      return *this;
   }

   template <is_integral U>
      requires(is_safe_arithmetic_conversion<U, raw_type>
               && !is_const<WrappedQual>
               && detail::is_arithmetic_ptr<wrapper_type>)
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   operator+=(U operand) -> overflow_reference& {
      *this = add(operand);
      return *this;
   }

   // += from a compile-time-known raw integer that fits `raw_type`. Writes the
   // sum through `raw` directly for the same reason as the `idx` `operator=`
   // above.
   template <is_arithmetic U>
      requires(!is_const<WrappedQual> && detail::is_idx<wrapper_type>)
   [[gnu::always_inline, gnu::nodebug]]
   constexpr overflow_reference&
   operator+=(U operand) __attribute__((
      enable_if(detail::raw_source_fits_implicit_storage<raw_type>(operand),
                "value is out of range for `+=` to cat::index"))) {
      m_wrapped->raw =
         static_cast<raw_type>(m_wrapped->raw + make_raw_arithmetic(operand));
      return *this;
   }

   // Mirrors the `requires` guard on `arithmetic::add`. With `wrap` /
   // `saturate` policies, mixed-signed-ness operands cannot be handled by the
   // underlying `wrap_add`/`sat_add` so this overload must be SFINAE'd out for
   // those cases. That makes `has_binary_plus` false at the
   // `overflow_reference` level too, which disables the inherited
   // `plus_interface::operator+` and lets the cross-signed-ness `operator+`
   // friends below take over for compile-time constants that fit the LHS
   // storage.
   template <is_arithmetic U>
      requires(!detail::is_arithmetic_ptr<wrapper_type>
               && !detail::is_idx<wrapper_type>
               && (overflow_policy == overflow_policies::undefined
                   || is_safe_arithmetic_comparison<raw_type,
                                                    raw_arithmetic_type<U>>))
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   add(U other) const
      -> detail::promoted_type<arithmetic<raw_type, overflow_policy>, U> {
      return view().add(other);
   }

   // Cross-signed-ness binary +. The inherited `operator+` requires
   // `has_binary_plus`, whose SFINAE check uses `declval` and so cannot see the
   // `enable_if` overload of `add`. This friend captures the operand directly,
   // allowing `enable_if` to fire on the call-site constant.
   template <is_arithmetic U>
      requires(!detail::is_arithmetic_ptr<wrapper_type>
               && !detail::is_idx<wrapper_type> && is_unsafe_arithmetic<U>
               && is_signed<raw_type> != is_signed<raw_arithmetic_type<U>>
               && is_floating_point<arithmetic<raw_type, overflow_policy>>
                     == is_floating_point<U>)
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   friend constexpr arithmetic<raw_type, overflow_policy>
   operator+(overflow_reference const& self, U operand) __attribute__((
      enable_if(detail::raw_source_fits_implicit_storage<raw_type>(operand),
                "value is out of range for implicit operation on this "
                "arithmetic type"))) {
      return self.view().add(arithmetic<raw_type, overflow_policy>(operand));
   }

   template <is_arithmetic U>
      requires(!detail::is_arithmetic_ptr<wrapper_type>
               && !detail::is_idx<wrapper_type> && is_unsafe_arithmetic<U>
               && is_signed<raw_type> != is_signed<raw_arithmetic_type<U>>
               && is_floating_point<arithmetic<raw_type, overflow_policy>>
                     == is_floating_point<U>)
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   friend constexpr arithmetic<raw_type, overflow_policy>
   operator+(U operand, overflow_reference const& self) __attribute__((
      enable_if(detail::raw_source_fits_implicit_storage<raw_type>(operand),
                "value is out of range for implicit operation on this "
                "arithmetic type"))) {
      return self.view().add(arithmetic<raw_type, overflow_policy>(operand));
   }

   template <is_integral U>
      requires(detail::is_arithmetic_ptr<wrapper_type>)
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   add(U operand) const {
      return view().add(operand);
   }

   template <is_unsigned_integral U>
      requires(detail::is_idx<wrapper_type>)
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   add(U other) const -> detail::promoted_type<wrapper_type, U> {
      return view().add(other);
   }

   template <is_signed_integral U>
      requires(detail::is_idx<wrapper_type>)
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   add(U other) const
      -> arithmetic<make_signed_type<__SIZE_TYPE__>, overflow_policy> {
      return view().add(other);
   }

   template <typename U>
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   add(U* p_operand) const -> U* {
      return view().add(p_operand);
   }

   // Mirror the `arithmetic::subtract_by` promotion rule. Signed undefined LHS
   // widens to a wider non-pointer RHS. Everything else (unsigned, wrap/sat,
   // pointer RHS, equal/narrower RHS) keeps the LHS shape.
   template <is_arithmetic U>
      requires(!detail::is_arithmetic_ptr<wrapper_type>
               && !detail::is_idx<wrapper_type>)
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   subtract_by(U operand) const -> conditional<
      (overflow_policy == overflow_policies::undefined
       && is_signed_integral<raw_type> && is_signed_integral<U>
       && (sizeof(raw_arithmetic_type<U>) > sizeof(raw_type))),
      detail::promoted_type<arithmetic<raw_type, overflow_policy>, U>,
      arithmetic<raw_type, overflow_policy>> {
      return view().subtract_by(operand);
   }

   template <is_integral U>
      requires(detail::is_arithmetic_ptr<wrapper_type>)
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   subtract_by(U operand) const {
      return view().subtract_by(operand);
   }

   template <is_integral U>
      requires(!is_same<remove_cvref<U>, index<overflow_policy>>
               && detail::is_idx<wrapper_type>)
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   subtract_by(U other) const
      -> arithmetic<make_signed_type<__SIZE_TYPE__>, overflow_policy> {
      return view().subtract_by(other);
   }

   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   subtract_by(index<overflow_policy> other) const
      -> arithmetic<make_signed_type<__SIZE_TYPE__>, overflow_policy>
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
      requires(!detail::is_arithmetic_ptr<wrapper_type>
               && !detail::is_idx<wrapper_type>)
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   subtract_from(U operand) const -> remove_constref<U> {
      return view().subtract_from(operand);
   }

   template <is_arithmetic U>
      requires(detail::is_arithmetic_ptr<wrapper_type>)
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
      requires(!detail::is_arithmetic_ptr<wrapper_type>
               && !detail::is_idx<wrapper_type>)
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   multiply(U operand) const
      -> detail::promoted_type<arithmetic<raw_type, overflow_policy>, U> {
      return view().multiply(operand);
   }

   template <typename U>
      requires(detail::is_arithmetic_ptr<wrapper_type>)
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   multiply(U operand) const {
      return view().multiply(operand);
   }

   template <is_integral U>
      requires(detail::is_idx<wrapper_type>)
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   multiply(U other) const -> index<overflow_policy> {
      return view().multiply(other);
   }

   template <typename U>
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   multiply(U* p_operand) const -> U* {
      return view().multiply(p_operand);
   }

   template <typename U>
      requires(!detail::is_arithmetic_ptr<wrapper_type>
               && !detail::is_idx<wrapper_type>)
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   divide_by(U operand) const -> arithmetic<raw_type, overflow_policy> {
      return view().divide_by(operand);
   }

   template <typename U>
      requires(detail::is_arithmetic_ptr<wrapper_type>)
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   divide_by(U operand) const {
      return view().divide_by(operand);
   }

   template <is_integral U>
      requires(detail::is_idx<wrapper_type>)
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   divide_by(U other) const {
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
               && !detail::is_arithmetic_ptr<wrapper_type>
               && !detail::is_idx<wrapper_type>)
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   divide_into(U operand) const -> remove_constref<U> {
      return view().divide_into(operand);
   }

   template <typename U>
      requires(detail::is_arithmetic_ptr<wrapper_type>)
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   divide_into(U operand) const {
      return view().divide_into(operand);
   }

   template <is_integral U>
      requires(detail::is_idx<wrapper_type>)
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   divide_into(U other) const {
      return view().divide_into(other);
   }

   template <typename U>
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   divide_into(U* p_operand) const -> U* {
      return view().divide_into(p_operand);
   }

   // Bitwise implementations. Only generate when the wrapped type supports
   // bitwise operators. `cat::index` and signed/floating-point `arithmetic<T>`
   // / `intptr<T>` are all disabled.
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   bit_complement() const
      requires(!detail::is_idx<wrapper_type> && is_unsigned_integral<raw_type>)
   {
      return view().bit_complement();
   }

   template <is_arithmetic U>
      requires(!detail::is_arithmetic_ptr<wrapper_type>
               && !detail::is_idx<wrapper_type>)
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   modulo_by(U operand) const -> arithmetic<raw_type, overflow_policy> {
      return view().modulo_by(operand);
   }

   template <typename U>
      requires(detail::is_arithmetic_ptr<wrapper_type>)
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   modulo_by(U operand) const {
      return view().modulo_by(operand);
   }

   template <is_integral U>
      requires(detail::is_idx<wrapper_type>)
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   modulo_by(U other) const -> index<overflow_policy> {
      return view().modulo_by(other);
   }

   template <typename U>
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   modulo_by(U* p_operand) const -> U* {
      return view().modulo_by(p_operand);
   }

   template <typename U>
      requires(detail::is_arithmetic_ptr<wrapper_type>)
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   modulo_into(U operand) const {
      return view().modulo_into(operand);
   }

   template <is_integral U>
      requires(detail::is_idx<wrapper_type>)
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   modulo_into(U operand) const -> index<overflow_policy> {
      return view().modulo_into(operand);
   }

   // `bit_and` forwarders. Only for wrappers with unsigned storage where the
   // operand fits the LHS width. A wider operand would silently truncate.
   template <is_unsigned_integral U>
      requires(!detail::is_arithmetic_ptr<wrapper_type>
               && !detail::is_idx<wrapper_type>
               && is_unsigned_integral<raw_type>
               && sizeof(raw_type) >= sizeof(U))
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   bit_and(U operand) const -> arithmetic<raw_type, overflow_policy> {
      return view().bit_and(operand);
   }

   template <typename U>
      requires(detail::is_arithmetic_ptr<wrapper_type>
               && is_unsigned_integral<raw_type>)
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   bit_and(U other) const {
      return view().bit_and(other);
   }

   template <typename U>
      requires(!detail::is_idx<wrapper_type> && is_unsigned_integral<raw_type>)
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   bit_and(U* p_operand) const -> U* {
      return view().bit_and(p_operand);
   }

   // `bit_or` forwarders. Wider-or-equal LHS unsigned only. See `bit_and`.
   template <is_unsigned_integral U>
      requires(!detail::is_arithmetic_ptr<wrapper_type>
               && !detail::is_idx<wrapper_type>
               && is_unsigned_integral<raw_type>
               && sizeof(raw_type) >= sizeof(U))
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   bit_or(U operand) const -> arithmetic<raw_type, overflow_policy> {
      return view().bit_or(operand);
   }

   template <typename Self, typename U>
      requires(detail::is_arithmetic_ptr<wrapper_type>
               && is_unsigned_integral<raw_type>)
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   bit_or(Self self, U operand) const {
      return view().bit_or(self, operand);
   }

   template <typename U>
      requires(!detail::is_idx<wrapper_type> && is_unsigned_integral<raw_type>)
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   bit_or(U* p_operand) const -> U* {
      return view().bit_or(p_operand);
   }

   // `shift_left_by` forwarders. `arithmetic` allows any integral storage
   // (signed << is the C++ arithmetic shift). `idx` always supports shifts via
   // the signed-bitcast trick. `arithmetic_ptr` only forwards for unsigned
   // storage.
   template <is_arithmetic U>
      requires(!detail::is_arithmetic_ptr<wrapper_type>
               && !detail::is_idx<wrapper_type> && is_integral<raw_type>)
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   shift_left_by(U operand) const -> arithmetic<raw_type, overflow_policy> {
      return view().shift_left_by(operand);
   }

   template <is_arithmetic U>
      requires(detail::is_idx<wrapper_type>)
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   shift_left_by(U operand) const -> index<overflow_policy> {
      return view().shift_left_by(operand);
   }

   template <typename U>
      requires(detail::is_arithmetic_ptr<wrapper_type>
               && is_unsigned_integral<raw_type>)
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   shift_left_by(U operand) const {
      return view().shift_left_by(operand);
   }

   // `shift_left_into` forwarders. Same eligibility as `shift_left_by`. Both
   // names map to the same << `operator,` just for the two dispatch sides.
   template <is_arithmetic U>
      requires(!detail::is_arithmetic_ptr<wrapper_type>
               && !detail::is_idx<wrapper_type> && is_integral<raw_type>)
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
      requires(detail::is_arithmetic_ptr<wrapper_type>
               && is_unsigned_integral<raw_type>)
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   shift_left_into(U other) const -> U {
      return view().shift_left_into(other);
   }

   // `shift_right_by` forwarders. Same eligibility rules as `shift_left_by`.
   template <is_arithmetic U>
      requires(!detail::is_arithmetic_ptr<wrapper_type>
               && !detail::is_idx<wrapper_type> && is_integral<raw_type>)
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   shift_right_by(U operand) const -> arithmetic<raw_type, overflow_policy> {
      return view().shift_right_by(operand);
   }

   template <is_arithmetic U>
      requires(detail::is_idx<wrapper_type>)
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   shift_right_by(U operand) const -> index<overflow_policy> {
      return view().shift_right_by(operand);
   }

   template <typename U>
      requires(detail::is_arithmetic_ptr<wrapper_type>
               && is_unsigned_integral<raw_type>)
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   shift_right_by(U operand) const {
      return view().shift_right_by(operand);
   }

   // `shift_right_into` forwarders. Same eligibility rules as
   // `shift_left_into`.
   template <is_arithmetic U>
      requires(!detail::is_arithmetic_ptr<wrapper_type>
               && !detail::is_idx<wrapper_type> && is_integral<raw_type>)
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   shift_right_into(U other) const -> U {
      return view().shift_right_into(other);
   }

   template <is_arithmetic U>
      requires(detail::is_idx<wrapper_type>)
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   shift_right_into(U other) const -> U {
      return view().shift_right_into(other);
   }

   template <is_arithmetic U>
      requires(detail::is_arithmetic_ptr<wrapper_type>
               && is_unsigned_integral<raw_type>)
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   shift_right_into(U other) const -> U {
      return view().shift_right_into(other);
   }
};

}  // namespace cat
