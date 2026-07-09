// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

namespace cat {

template <is_unsigned_integral Storage>
class bit_reference {
 public:
   constexpr bit_reference(bit_reference<Storage> const&) = default;
   constexpr bit_reference(bit_reference<Storage>&&) = default;

   constexpr bit_reference(bit_value& other)
       : m_p_storage(__builtin_addressof(detail::get_bit_value(other))),
         m_bit_mask(1u) {
   }

   constexpr bit_reference(bit_value const& other)
       : m_p_storage(__builtin_addressof(detail::get_bit_value(other))),
         m_bit_mask(1u) {
   }

   // You cannot take a reference of r-values.
   constexpr bit_reference(bit_value&& other) = delete;

   constexpr auto
   operator=(bit_reference<Storage> const& other) & -> bit_reference& = default;

   constexpr auto
   operator=(bit_reference<Storage>&& other) & -> bit_reference& {
      m_p_storage = other.m_p_storage;
      m_bit_mask = other.m_bit_mask;
      return *this;
   }

   // Assign a true or false value to this bit.
   constexpr auto
   operator=(Storage value) -> bit_reference& {
      this->assign(value);
      return *this;
   }

   // Assign a true or false value to this bit.
   constexpr auto
   operator=(bool value) -> bit_reference&
      requires(!is_same<Storage, bool>)
   {
      this->assign(value);
      return *this;
   }

   // An explicit `operator==(bit_reference, bit_reference)` is required by
   // C++20.
   friend constexpr auto
   operator==(
      bit_reference<Storage> const& self, bit_reference<Storage> const& other
   ) -> bool {
      return self.is_set() == other;
   }

   // Because `bit_reference` and `bit_value` convert to `bool`, this comparison
   // works between all the above.
   friend constexpr auto
   operator==(bit_reference<Storage> const& self, bool other) -> bool {
      return self.is_set() == other;
   }

   // Implicitly convert this bit to a true or false value.
   constexpr
   operator bool() const {
      return this->is_set();
   }

   // Rebind this `bit_reference` to a different bit position specified by
   // `mask` into a `Storage`.
   constexpr void
   rebind(
      Storage& reference [[clang::lifetime_capture_by(this)]], Storage mask
   ) {
      if !consteval {
         assert(has_single_bit(mask));
      }
      m_p_storage = __builtin_addressof(reference);
      m_bit_mask = mask;
   }

   // Assign a true or false value to this bit.
   constexpr auto
   assign(bool value) -> bit_reference& {
      value ? this->set() : this->unset();
      return *this;
   }

   // Assign another bit's value to this bit.
   template <is_unsigned_integral T>
   constexpr auto
   assign(Storage value) -> bit_reference& {
      if !consteval {
         assert(has_single_bit(value));
      }
      value & 1u ? set() : unset();
      return *this;
   }

   // Flag this bit on.
   constexpr auto
   set() -> bit_reference& {
      *m_p_storage |= m_bit_mask;
      return *this;
   }

   // Flag this bit off.
   constexpr auto
   unset() -> bit_reference& {
      *m_p_storage &= ~m_bit_mask;
      return *this;
   }

   // Flag this bit as the opposite of its current state.
   constexpr auto
   flip() -> bit_reference& {
      *m_p_storage ^= m_bit_mask;
      return *this;
   }

   [[nodiscard]]
   constexpr auto
   is_set() const -> bool {
      return (*m_p_storage & m_bit_mask) != 0u;
   }

   [[nodiscard]]
   constexpr auto
   mask() const -> Storage const& {
      return m_bit_mask;
   }

 private:
   friend bit_value;
   friend class bit_ptr<Storage>;
   template <is_unsigned_integral FriendStorage, is_unsigned_integral Mask>
   friend constexpr auto
   make_bit_reference_from_mask(FriendStorage& reference, Mask mask)
      -> bit_reference<FriendStorage>;
   friend constexpr auto
   make_bit_reference_from_offset<Storage>(Storage& reference, uword offset)
      -> bit_reference<Storage>;

   // This constructor is only used by the free factory functions.
   constexpr bit_reference(Storage& in_storage, Storage in_mask)
       : m_p_storage(__builtin_addressof(in_storage)), m_bit_mask(in_mask) {
      if !consteval {
         assert(has_single_bit(in_mask));
      }
   }

   Storage* _Nonnull m_p_storage;

   // If this references a `bit_value`, the type would be `bool`, which can be
   // problematic and confusing for bit operations such as negation.
   using mask_integer_type =
      conditional<is_same<Storage, bool>, unsigned char, Storage>;
   remove_cv<mask_integer_type> m_bit_mask;
};

template <is_same<bit_value> T>
bit_reference(T) -> bit_reference<unsigned char>;

bit_reference(bit_value&) -> bit_reference<bool>;

bit_reference(bit_value const&) -> bit_reference<bool const>;

template <is_unsigned_integral Storage>
   requires(!is_const<Storage>)
constexpr void
swap(bit_reference<Storage> left, bit_reference<Storage> right) {
   bool const left_value = left;
   left = right;
   right = left_value;
}

template <is_unsigned_integral Storage, is_unsigned_integral Mask>
constexpr auto
make_bit_reference_from_mask(Storage& reference, Mask mask)
   -> bit_reference<Storage> {
   return bit_reference<Storage>(reference, Storage(mask));
}

template <is_unsigned_integral Storage>
constexpr auto
make_bit_reference_from_offset(Storage& reference, uword offset)
   -> bit_reference<Storage> {
   if !consteval {
      assert(offset < limits<Storage>::bits);
   }
   return bit_reference<Storage>(reference, Storage(1u) << offset);
}

}  // namespace cat
