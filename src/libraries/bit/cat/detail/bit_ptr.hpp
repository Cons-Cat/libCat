// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

namespace cat {

template <is_unsigned_integral Storage>
class [[gsl::Pointer]]
bit_ptr : public plus_interface<bit_ptr<Storage>>,
          public minus_interface<bit_ptr<Storage>> {
   static constexpr idx storage_bits = limits<Storage>::bits;

   template <is_unsigned_integral FriendStorage, is_unsigned_integral Mask>
   friend constexpr auto
   make_bit_ptr_from_mask(FriendStorage* _Nonnull p_storage, Mask mask)
      -> bit_ptr<FriendStorage>;
   template <is_unsigned_integral FriendStorage, is_arithmetic Offset>
   friend constexpr auto
   make_bit_ptr_from_offset(FriendStorage* _Nonnull p_storage, Offset offset)
      -> bit_ptr<FriendStorage>;

 public:
   using storage_type = Storage;
   using value_type = bit_value;
   using reference = bit_reference<Storage>;
   using difference_type = iword;

   constexpr bit_ptr() = default;
   constexpr bit_ptr(bit_ptr const&) = default;
   constexpr bit_ptr(bit_ptr&&) = default;

   constexpr bit_ptr(decltype(nullptr)) {
   }

   constexpr explicit bit_ptr(Storage* _Nullable p_storage)
       : m_p_storage(p_storage) {
   }

   constexpr auto
   operator=(bit_ptr const&) -> bit_ptr& = default;

   constexpr auto
   operator=(bit_ptr&&) -> bit_ptr& = default;

   constexpr auto
   operator=(decltype(nullptr)) -> bit_ptr& {
      m_p_storage = nullptr;
      m_bit_position = 0u;
      return *this;
   }

   [[nodiscard]]
   constexpr explicit
   operator bool() const {
      return m_p_storage != nullptr;
   }

   [[nodiscard]]
   constexpr auto
   operator*() const -> reference {
      return make_bit_reference_from_offset(*m_p_storage, m_bit_position);
   }

   [[nodiscard]]
   constexpr auto
   operator[](difference_type offset) const -> reference {
      return *(*this + offset);
   }

   constexpr auto
   operator++() -> bit_ptr& {
      *this += 1;
      return *this;
   }

   constexpr auto
   operator--() -> bit_ptr& {
      *this -= 1;
      return *this;
   }

   constexpr auto
   operator++(int) -> bit_ptr {
      bit_ptr old = *this;
      ++*this;
      return old;
   }

   constexpr auto
   operator--(int) -> bit_ptr {
      bit_ptr old = *this;
      --*this;
      return old;
   }

   template <is_arithmetic Offset>
   [[nodiscard]]
   constexpr auto
   add(Offset offset) const -> bit_ptr {
      difference_type const advanced_bit = m_bit_position + offset;
      difference_type word_offset = advanced_bit / storage_bits;
      if (advanced_bit < 0 && word_offset * storage_bits != advanced_bit) {
         --word_offset;
      }
      return make_bit_ptr_from_offset(
         m_p_storage + word_offset, advanced_bit - word_offset * storage_bits
      );
   }

   template <is_arithmetic Offset>
   constexpr auto
   subtract_by(Offset offset) const -> bit_ptr {
      return add(-offset);
   }

   [[nodiscard]]
   constexpr auto
   storage() const -> Storage* _Nullable {
      return m_p_storage;
   }

   [[nodiscard]]
   constexpr auto
   bit_position() const -> idx {
      return m_bit_position;
   }

   [[nodiscard]]
   friend constexpr auto
   operator-(bit_ptr left, bit_ptr right) -> difference_type {
      difference_type const storage_difference =
         left.m_p_storage - right.m_p_storage;
      return storage_difference * storage_bits + left.m_bit_position
             - right.m_bit_position;
   }

   [[nodiscard]]
   friend constexpr auto
   operator==(bit_ptr left, bit_ptr right) -> bool {
      return left.m_p_storage == right.m_p_storage
             && left.m_bit_position == right.m_bit_position;
   }

   [[nodiscard]]
   friend constexpr auto
   operator==(bit_ptr pointer, decltype(nullptr)) -> bool {
      return pointer.m_p_storage == nullptr;
   }

   [[nodiscard]]
   friend constexpr auto
   operator==(decltype(nullptr), bit_ptr pointer) -> bool {
      return pointer == nullptr;
   }

   [[nodiscard]]
   friend constexpr auto
   operator<=>(bit_ptr left, bit_ptr right) {
      if (left.m_p_storage != right.m_p_storage) {
         return left.m_p_storage <=> right.m_p_storage;
      }
      return left.m_bit_position <=> right.m_bit_position;
   }

 private:
   constexpr bit_ptr(Storage* _Nonnull p_storage, idx bit_offset)
       : m_p_storage(p_storage + (bit_offset / storage_bits)),
         m_bit_position(bit_offset % storage_bits) {
   }

   Storage* _Nullable m_p_storage = nullptr;
   idx m_bit_position = 0u;
};

template <is_unsigned_integral Storage>
bit_ptr(Storage* _Nullable) -> bit_ptr<Storage>;

template <is_unsigned_integral Storage, is_unsigned_integral Mask>
constexpr auto
make_bit_ptr_from_mask(Storage* _Nonnull p_storage, Mask mask)
   -> bit_ptr<Storage> {
   Storage const storage_mask = Storage(mask);
   if !consteval {
      assert(has_single_bit(storage_mask));
   }
   return bit_ptr<Storage>(p_storage, countr_zero(storage_mask));
}

template <is_unsigned_integral Storage, is_arithmetic Offset>
constexpr auto
make_bit_ptr_from_offset(Storage* _Nonnull p_storage, Offset offset)
   -> bit_ptr<Storage> {
   if !consteval {
      assert(offset >= 0);
   }
   return bit_ptr<Storage>(p_storage, idx(offset));
}

}  // namespace cat
