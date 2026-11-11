// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic_interface>

// Included by <cat/complex> after `basic_complex_int` and `basic_complex_float`
// are defined.

namespace cat::detail {

// A complex proxy reference is read as its value. Any other operand is passed
// through.
template <typename T>
[[nodiscard, gnu::always_inline, gnu::nodebug]]
constexpr auto
complex_reference_value(T const& operand) noexcept {
   if constexpr (is_complex_reference<T>) {
      return static_cast<complex_reference_view<T>>(operand);
   } else {
      return operand;
   }
}

// The parts that `complex_precision_reference` and `complex_overflow_reference`
// share. `View` is the type of `Complex` with the reference's policy. The
// arithmetic interfaces of `Derived` generate operators from the hooks here,
// and compound assignments write their result back through `operator=`.
template <typename Derived, typename Complex, typename View>
class complex_reference {
 public:
   using wrapper_type = __typeof_unqual(Complex);
   using value_type = View::value_type;
   using component_type = View::component_type;
   using quantity_type = View::quantity_type;
   static constexpr auto quantity_reference = View::quantity_reference;

   constexpr explicit complex_reference(
      Complex& wrapped [[clang::lifetimebound]]
   ) noexcept
       : m_wrapped(addressof(wrapped)) {
   }

   // P3207R0. This type is reference-like.
   friend constexpr void
   operator&(complex_reference const volatile&) = delete;

   friend constexpr void
   operator&(complex_reference const volatile&&) = delete;

   constexpr complex_reference(complex_reference const&) = default;

   // Assigning another reference of this type writes its value through, as
   // assigning a built-in reference does. `rebind()` changes the address.
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   operator=(complex_reference const& other) noexcept -> complex_reference&
      requires(!is_const<Complex>)
   {
      *m_wrapped = *other.m_wrapped;
      return *this;
   }

   // Rebind this reference wrapper to a different address.
   constexpr void
   rebind(Complex& wrapped [[clang::lifetime_capture_by_this]]) noexcept {
      m_wrapped = addressof(wrapped);
   }

   // A reference converts implicitly into its own value with this policy, and
   // explicitly into anything that value converts into.
   [[gnu::always_inline, gnu::nodebug]]
   constexpr
   operator View() const noexcept {
      return view();
   }

   template <typename U>
      requires(!is_same<U, View> && is_constructible<U, View>)
   [[gnu::always_inline, gnu::nodebug]]
   constexpr explicit
   operator U() const noexcept {
      return U(view());
   }

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   real() const noexcept {
      return view().real();
   }

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   imag() const noexcept {
      return view().imag();
   }

   // Tuple protocol. Index 0 is the real part and index 1 is the imaginary
   // part, each by value with this reference's policy.
   template <idx index>
      requires(index.raw < 2u)
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   get() const noexcept {
      if constexpr (index.raw == 0u) {
         return real();
      } else {
         return imag();
      }
   }

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   conjugate() const noexcept -> View {
      return view().conjugate();
   }

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   norm() const noexcept {
      return view().norm();
   }

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   operator+() const noexcept -> View {
      return view();
   }

   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   operator-() const noexcept -> View {
      return -view();
   }

   template <typename U>
      requires(requires(View self, U const& operand) {
                  self.add(complex_reference_value(operand));
               })
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   add(U const& operand) const noexcept {
      return view().add(complex_reference_value(operand));
   }

   template <typename U>
      requires(requires(View self, U const& operand) {
                  self.add_noncommutative(complex_reference_value(operand));
               })
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   add_noncommutative(U const& operand) const noexcept {
      return view().add_noncommutative(complex_reference_value(operand));
   }

   template <typename U>
      requires(requires(View self, U const& operand) {
                  self.subtract_by(complex_reference_value(operand));
               })
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   subtract_by(U const& operand) const noexcept {
      return view().subtract_by(complex_reference_value(operand));
   }

   template <typename U>
      requires(requires(View self, U const& operand) {
                  self.subtract_from(complex_reference_value(operand));
               })
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   subtract_from(U const& operand) const noexcept {
      return view().subtract_from(complex_reference_value(operand));
   }

   template <typename U>
      requires(requires(View self, U const& operand) {
                  self.multiply(complex_reference_value(operand));
               })
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   multiply(U const& operand) const noexcept {
      return view().multiply(complex_reference_value(operand));
   }

   template <typename U>
      requires(requires(View self, U const& operand) {
                  self.multiply_noncommutative(
                     complex_reference_value(operand)
                  );
               })
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   multiply_noncommutative(U const& operand) const noexcept {
      return view().multiply_noncommutative(complex_reference_value(operand));
   }

   template <typename U>
      requires(requires(View self, U const& operand) {
                  self.divide_by(complex_reference_value(operand));
               })
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   divide_by(U const& operand) const noexcept {
      return view().divide_by(complex_reference_value(operand));
   }

   template <typename U>
      requires(requires(View self, U const& operand) {
                  self.divide_into(complex_reference_value(operand));
               })
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   divide_into(U const& operand) const noexcept {
      return view().divide_into(complex_reference_value(operand));
   }

   template <typename U>
      requires(requires(View self, U const& operand) {
                  self == complex_reference_value(operand);
               })
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   friend constexpr auto
   operator==(Derived const& self, U const& operand) noexcept -> bool {
      return static_cast<complex_reference const&>(self).view()
             == complex_reference_value(operand);
   }

   // Assigning a result that does not convert implicitly into the wrapped type,
   // such as one that widens, is rejected, so compound assignments that would
   // produce one are too.
   template <typename U>
      requires(
         !is_const<Complex>
         && is_convertible<
            __typeof_unqual(complex_reference_value(declval<U const&>())),
            wrapper_type>
      )
   [[gnu::always_inline, gnu::nodebug]]
   constexpr auto
   operator=(U const& operand) noexcept -> Derived& {
      *m_wrapped = wrapper_type(complex_reference_value(operand));
      return static_cast<Derived&>(*this);
   }

 protected:
   // The wrapped complex number, copied bit for bit with this reference's
   // policy.
   [[nodiscard, gnu::always_inline, gnu::nodebug]]
   constexpr auto
   view() const noexcept -> View {
      return View(complex_in_raw<component_type>(*m_wrapped));
   }

   Complex* _Nonnull m_wrapped;
};

}  // namespace cat::detail
