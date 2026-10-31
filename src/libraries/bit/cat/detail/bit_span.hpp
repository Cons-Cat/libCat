// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

// `cat::bit_span` is a non-owning view over a contiguous sequence of bits,
// parameterized by a word `Storage` type. It is the bit analogue of
// `cat::span`, or a fat analogue of `cat::bit_ptr`, providing bit-level
// indexing and algorithms over untyped storage from containers such as
// `cat::bitset` and `cat::bitvec`.

// TODO: These algorithms are not SIMD optimized yet.

#include <cat/arithmetic>
#include <cat/debug>
#include <cat/limits>
#include <cat/math>
#include <cat/meta>
#include <cat/span>
#include <cat/stepanov>

namespace cat {

template <idx bits_count, typename T>
class bitset;

template <is_unsigned_integral Storage>
class bit_span;

template <is_unsigned_integral Storage>
[[nodiscard]]
constexpr auto
popcount(bit_span<Storage> bits) noexcept -> idx;

namespace detail {
template <typename WordStorage>
concept has_bit_span_value_type =
   requires { typename remove_cvref<WordStorage>::value_type; };

template <typename WordStorage, typename Storage>
concept is_bit_span_word_container =
   requires(WordStorage& storage) {
      storage.data();
      storage.size();
   } && has_bit_span_value_type<WordStorage>
   && is_unsigned_integral<typename remove_cvref<WordStorage>::value_type>
   && !requires { typename remove_cvref<WordStorage>::abi_type; }
   && is_convertible<decltype(declval<WordStorage&>().data()), Storage*>;

template <typename WordStorage, typename Storage>
concept is_bit_span_bit_container =
   requires(WordStorage& storage) {
      storage.data();
      storage.size();
      storage.word_count();
   } && is_convertible<decltype(declval<WordStorage&>().data()), Storage*>;
}  // namespace detail

template <is_unsigned_integral Storage = uword>
class [[clang::trivial_abi, gsl::Pointer(Storage)]]
bit_span : public bit_collection_interface<bit_span<Storage>> {
 public:
   using storage_type = Storage;
   using value_type = bit_value;
   using const_value_type = bit_value const;
   using reference = bit_reference<Storage>;
   using const_reference = bit_reference<remove_cv<Storage> const>;
   using pointer = bit_ptr<Storage>;
   using const_pointer = bit_ptr<remove_cv<Storage> const>;
   using size_type = idx;
   using difference_type = iword;
   using word_span = span<Storage>;
   using word_iterator = word_span::iterator;
   using const_word_iterator = word_span::const_iterator;
   using iterator = bit_stepanov_iterator<word_iterator>;
   using const_iterator = bit_stepanov_iterator<const_word_iterator>;
   using reverse_iterator = reverse_proxy_stepanov_iterator<iterator>;
   using const_reverse_iterator =
      reverse_proxy_stepanov_iterator<const_iterator>;

   constexpr bit_span() = default;
   constexpr bit_span(bit_span const&) = default;
   constexpr bit_span(bit_span&&) = default;

   constexpr bit_span(decltype(nullptr)) noexcept {
   }

   // A view over mutable storage converts to a view over the same storage as
   // `const`.
   template <typename MutableStorage>
      requires(
         is_same<MutableStorage const, Storage>
         && !is_same<MutableStorage, Storage>
      )
   constexpr bit_span(bit_span<MutableStorage> bits) noexcept
       : bit_span(bits.data(), bits.bit_offset(), bits.size()) {
   }

   constexpr bit_span(
      Storage* _Nullable p_storage [[clang::lifetimebound]], idx bit_offset,
      idx bit_count
   ) noexcept
       : m_p_storage(p_storage),
         m_bit_offset(bit_offset),
         m_bit_size(bit_count) {
   }

   template <idx bits_count, typename T>
   constexpr explicit bit_span(
      bitset<bits_count, T>& storage [[clang::lifetimebound]]
   ) noexcept;

   template <idx bits_count, typename T>
   constexpr explicit bit_span(
      bitset<bits_count, T> const& storage [[clang::lifetimebound]]
   ) noexcept;

   template <typename WordStorage>
      requires(
         detail::is_bit_span_word_container<WordStorage, Storage>
         || detail::is_bit_span_bit_container<WordStorage, Storage>
      )
   constexpr explicit bit_span(
      WordStorage& storage [[clang::lifetimebound]]
   ) noexcept
       : bit_span(
            storage.data(), bit_offset_for(storage), bit_count_for(storage)
         ) {
   }

   template <typename WordStorage>
      requires(
         detail::is_bit_span_word_container<WordStorage, Storage>
         || detail::is_bit_span_bit_container<WordStorage, Storage>
      )
   constexpr bit_span(
      WordStorage& storage [[clang::lifetimebound]]
      ,
      idx bit_offset, idx bit_count
   ) noexcept
       : bit_span(storage.data(), bit_offset, bit_count) {
   }

   constexpr auto
   operator=(bit_span const&) noexcept -> bit_span& = default;

   constexpr auto
   operator=(bit_span&&) noexcept -> bit_span& = default;

   [[nodiscard]]
   constexpr auto
   size() const noexcept -> idx {
      return m_bit_size;
   }

   [[nodiscard]]
   constexpr auto
   data() const noexcept -> Storage* _Nullable {
      return m_p_storage;
   }

   [[nodiscard]]
   constexpr auto
   begin() const noexcept -> iterator {
      return iterator(
         word_iterator(m_bit_offset / limits<Storage>::bits, m_p_storage),
         m_bit_offset % limits<Storage>::bits
      );
   }

   [[nodiscard]]
   constexpr auto
   end() const noexcept -> iterator {
      idx const bit_end = m_bit_offset + m_bit_size;
      return iterator(
         word_iterator(bit_end / limits<Storage>::bits, m_p_storage),
         bit_end % limits<Storage>::bits
      );
   }

   [[nodiscard]]
   constexpr auto
   cbegin() const noexcept -> const_iterator {
      return const_iterator(
         const_word_iterator(m_bit_offset / limits<Storage>::bits, m_p_storage),
         m_bit_offset % limits<Storage>::bits
      );
   }

   [[nodiscard]]
   constexpr auto
   cend() const noexcept -> const_iterator {
      idx const bit_end = m_bit_offset + m_bit_size;
      return const_iterator(
         const_word_iterator(bit_end / limits<Storage>::bits, m_p_storage),
         bit_end % limits<Storage>::bits
      );
   }

   [[nodiscard]]
   constexpr auto
   rbegin() const noexcept(!__has_feature(cxx_exceptions)) -> reverse_iterator {
      assert(m_bit_size > 0u);
      iterator iter = end();
      --iter;
      return reverse_iterator(iter);
   }

   [[nodiscard]]
   constexpr auto
   rend() const noexcept -> reverse_iterator {
      iterator iter = begin();
      --iter;
      return reverse_iterator(iter);
   }

   [[nodiscard]]
   constexpr auto
   crbegin() const noexcept(!__has_feature(cxx_exceptions))
      -> const_reverse_iterator {
      assert(m_bit_size > 0u);
      const_iterator iter = cend();
      --iter;
      return const_reverse_iterator(iter);
   }

   [[nodiscard]]
   constexpr auto
   crend() const noexcept -> const_reverse_iterator {
      const_iterator iter = cbegin();
      --iter;
      return const_reverse_iterator(iter);
   }

   [[nodiscard]]
   constexpr auto
   bit_offset() const noexcept -> idx {
      return m_bit_offset;
   }

   [[nodiscard]]
   constexpr auto
   operator[](idx bit_index) const noexcept(!__has_feature(cxx_exceptions))
      [[clang::lifetimebound]] -> reference {
      assert(bit_index < m_bit_size);
      return *make_bit_ptr_from_offset(m_p_storage, m_bit_offset + bit_index);
   }

   [[nodiscard]]
   constexpr auto
   slice(idx first, idx last) const noexcept(!__has_feature(cxx_exceptions))
      -> bit_span {
      assert(first <= last);
      assert(last <= m_bit_size);
      return bit_span(m_p_storage, m_bit_offset + first, idx(last - first));
   }

   // Count set bits in this bit span.
   [[nodiscard]]
   constexpr auto
   popcount() const noexcept -> idx {
      return cat::popcount(*this);
   }

   [[nodiscard]]
   constexpr auto
   has_single_bit() const noexcept -> bool {
      return popcount() == 1u;
   }

   [[nodiscard]]
   constexpr auto
   countl_zero() const noexcept -> idx {
      idx count = 0u;
      for (idx i = 0u; i < m_bit_size; ++i) {
         if ((*this)[i]) {
            break;
         }
         ++count;
      }
      return count;
   }

   [[nodiscard]]
   constexpr auto
   countl_one() const noexcept -> idx {
      idx count = 0u;
      for (idx i = 0u; i < m_bit_size; ++i) {
         if (!(*this)[i]) {
            break;
         }
         ++count;
      }
      return count;
   }

   [[nodiscard]]
   constexpr auto
   countr_zero() const noexcept -> idx {
      idx count = 0u;
      for (idx i = m_bit_size; i > 0u;) {
         i = idx(i - 1u);
         if ((*this)[i]) {
            break;
         }
         ++count;
      }
      return count;
   }

   [[nodiscard]]
   constexpr auto
   countr_one() const noexcept -> idx {
      idx count = 0u;
      for (idx i = m_bit_size; i > 0u;) {
         i = idx(i - 1u);
         if (!(*this)[i]) {
            break;
         }
         ++count;
      }
      return count;
   }

   [[nodiscard]]
   constexpr auto
   bit_width() const noexcept -> idx {
      return idx(m_bit_size - countl_zero());
   }

   constexpr auto
   fill(bit_value value = true) const noexcept -> bit_span
      requires(!is_const<Storage>)
   {
      for (idx i = 0u; i < m_bit_size; ++i) {
         (*this)[i] = value;
      }
      return *this;
   }

   constexpr auto
   clear() const noexcept -> bit_span
      requires(!is_const<Storage>)
   {
      return fill(false);
   }

   constexpr auto
   copy_from(bit_span<remove_cv<Storage> const> source) const noexcept(
      !__has_feature(cxx_exceptions)
   ) -> bit_span
      requires(!is_const<Storage>)
   {
      assert(source.size() == m_bit_size);
      if (
         (m_bit_offset % limits<Storage>::bits) == 0u
         && (source.bit_offset() % limits<Storage>::bits) == 0u
      ) {
         idx const whole_words = m_bit_size / limits<Storage>::bits;
         Storage* _Nonnull const p_destination =
            m_p_storage + (m_bit_offset / limits<Storage>::bits);
         Storage const* _Nonnull const p_source =
            source.data() + (source.bit_offset() / limits<Storage>::bits);
         for (idx i = 0u; i < whole_words; ++i) {
            p_destination[i] = p_source[i];
         }
         idx const tail_bits = m_bit_size % limits<Storage>::bits;
         if (tail_bits != 0u) {
            // Keep the destination bits past the end of this span.
            Storage const tail_mask = (Storage(1u) << tail_bits) - Storage(1u);
            p_destination[whole_words] =
               (p_destination[whole_words] & ~tail_mask)
               | (p_source[whole_words] & tail_mask);
         }
         return *this;
      }
      for (idx i = 0u; i < m_bit_size; ++i) {
         (*this)[i] = source[i];
      }
      return *this;
   }

   constexpr auto
   move_from(bit_span<remove_cv<Storage> const> source) const noexcept(
      !__has_feature(cxx_exceptions)
   ) -> bit_span
      requires(!is_const<Storage>)
   {
      return copy_from(source);
   }

   [[nodiscard]]
   constexpr auto
   equal(bit_span<remove_cv<Storage> const> other) const noexcept -> bool {
      if (other.size() != m_bit_size) {
         return false;
      }
      for (idx i = 0u; i < m_bit_size; ++i) {
         if ((*this)[i] != other[i].is_set()) {
            return false;
         }
      }
      return true;
   }

   // Shift left by `count` positions in place, like `cat::shift_left()`. Bit
   // `i` moves to bit `i + count`, bits shifted past the end are dropped, and
   // the low `count` bits become 0. A `count` of at least `size()` clears this
   // span.
   constexpr auto
   shift_left(idx count) const noexcept -> bit_span
      requires(!is_const<Storage>)
   {
      if (count >= m_bit_size) {
         return clear();
      }
      rotate_toward_front(idx(m_bit_size - count));
      for (idx i = 0u; i < count; ++i) {
         (*this)[i] = false;
      }
      return *this;
   }

   // Shift right by `count` positions in place, like `cat::shift_right()`. Bit
   // `i + count` moves to bit `i`, bits shifted past the start are dropped, and
   // the high `count` bits become 0. A `count` of at least `size()` clears
   // this span.
   constexpr auto
   shift_right(idx count) const noexcept -> bit_span
      requires(!is_const<Storage>)
   {
      if (count >= m_bit_size) {
         return clear();
      }
      rotate_toward_front(count);
      for (idx i = idx(m_bit_size - count); i < m_bit_size; ++i) {
         (*this)[i] = false;
      }
      return *this;
   }

   constexpr auto
   reverse_inplace() const noexcept -> bit_span
      requires(!is_const<Storage>)
   {
      cat::stepanov_reverse_inplace(begin(), end());
      return *this;
   }

   // Bitwise rotate left by `shift` positions in place, across all bits of this
   // span, like `bitset::rotate_left()`. Bit `i` moves to bit `i + shift`.
   // `shift` is reduced mod `size()`. Negative shifts rotate the other way.
   constexpr auto
   rotate_left(iword shift) const noexcept -> bit_span
      requires(!is_const<Storage>)
   {
      if (m_bit_size != 0u) {
         idx const left_shift = reduced_rotation(shift);
         rotate_toward_front(
            left_shift == 0u ? left_shift : idx(m_bit_size - left_shift)
         );
      }
      return *this;
   }

   // Bitwise rotate right by `shift` positions in place, across all bits of
   // this span, like `bitset::rotate_right()`. Bit `i + shift` moves to bit
   // `i`. `shift` is reduced mod `size()`. Negative shifts rotate the other
   // way.
   constexpr auto
   rotate_right(iword shift) const noexcept -> bit_span
      requires(!is_const<Storage>)
   {
      if (m_bit_size != 0u) {
         rotate_toward_front(reduced_rotation(shift));
      }
      return *this;
   }

   template <typename Callback>
   constexpr auto
   transform(Callback callback) const -> bit_span
      requires(!is_const<Storage>)
   {
      for (idx i = 0u; i < m_bit_size; ++i) {
         (*this)[i] = callback(bit_value{(*this)[i]});
      }
      return *this;
   }

 private:
   // Reduce a signed rotation to a count in `[0, size())`. This span must not
   // be empty.
   [[nodiscard]]
   constexpr auto
   reduced_rotation(iword shift) const noexcept -> idx {
      iword const remainder = shift % iword(m_bit_size);
      return remainder < 0 ? idx(remainder + iword(m_bit_size))
                           : idx(remainder);
   }

   // Move every bit `count` positions toward the front, wrapping around, for a
   // `count` less than `size()`. Each reversal is word-level.
   constexpr void
   rotate_toward_front(idx count) const noexcept {
      if (count == 0u) {
         return;
      }
      iterator const split = begin() + count;
      cat::stepanov_reverse_inplace(begin(), split);
      cat::stepanov_reverse_inplace(split, end());
      cat::stepanov_reverse_inplace(begin(), end());
   }

   template <typename WordStorage>
   [[nodiscard]]
   static constexpr auto
   bit_count_for(WordStorage& storage) noexcept -> idx {
      if constexpr (requires { storage.word_count(); }) {
         return storage.size();
      } else {
         return storage.size() * limits<Storage>::bits;
      }
   }

   template <typename WordStorage>
   [[nodiscard]]
   static constexpr auto
   bit_offset_for([[maybe_unused]] WordStorage& storage) noexcept -> idx {
      if constexpr (requires { WordStorage::leading_skipped_bits; }) {
         return WordStorage::leading_skipped_bits;
      } else {
         return 0u;
      }
   }

   Storage* _Nullable m_p_storage = nullptr;
   idx m_bit_offset = 0u;
   idx m_bit_size = 0u;
};

template <typename WordStorage>
using bit_span_storage_type =
   remove_reference<decltype(*declval<WordStorage&>().data())>;

template <typename WordStorage>
bit_span(WordStorage&) -> bit_span<bit_span_storage_type<WordStorage>>;

template <typename WordStorage>
bit_span(WordStorage&, idx, idx)
   -> bit_span<bit_span_storage_type<WordStorage>>;

namespace detail {
template <typename Bits>
concept bit_span_constructible =
   !is_specialization<remove_cvref<Bits>, bit_span>
   && requires(Bits& bits) { bit_span(bits); };
}  // namespace detail

// Count set bits in a bit span.
template <is_unsigned_integral Storage>
[[nodiscard]]
constexpr auto
popcount(bit_span<Storage> bits) noexcept -> idx {
   if (bits.size() == 0u) {
      return 0u;
   }
   if ((bits.bit_offset() % limits<Storage>::bits) == 0u) {
      idx const first_word = bits.bit_offset() / limits<Storage>::bits;
      idx const word_count = div_ceil(bits.size(), limits<Storage>::bits);
      Storage const* const p_words = bits.data() + first_word;
      if ((bits.size() % limits<Storage>::bits) == 0u) {
         idx count = 0u;
         for (idx word_index = 0u; word_index < word_count; ++word_index) {
            count += cat::popcount(p_words[word_index]);
         }
         return count;
      }

      idx count = 0u;
      idx const last_word = idx(word_count - 1u);
      for (idx word_index = 0u; word_index < last_word; ++word_index) {
         count += cat::popcount(p_words[word_index]);
      }
      idx const tail_bits = bits.size() % limits<Storage>::bits;
      Storage const tail_mask = (Storage(1u) << tail_bits) - Storage(1u);
      count += cat::popcount(p_words[last_word] & tail_mask);
      return count;
   }

   idx count = 0u;
   for (idx i = 0u; i < bits.size(); ++i) {
      count += bits[i] ? 1u : 0u;
   }
   return count;
}

template <typename Bits>
   requires(detail::bit_span_constructible<Bits>)
[[nodiscard]]
constexpr auto
popcount(Bits& bits) noexcept -> idx {
   return popcount(bit_span(bits));
}

template <is_unsigned_integral Storage>
[[nodiscard]]
constexpr auto
has_single_bit(bit_span<Storage> bits) noexcept -> bool {
   return bits.has_single_bit();
}

template <typename Bits>
   requires(detail::bit_span_constructible<Bits>)
[[nodiscard]]
constexpr auto
has_single_bit(Bits& bits) noexcept -> bool {
   return has_single_bit(bit_span(bits));
}

template <is_unsigned_integral Storage>
[[nodiscard]]
constexpr auto
bit_width(bit_span<Storage> bits) noexcept -> idx {
   return bits.bit_width();
}

template <typename Bits>
   requires(detail::bit_span_constructible<Bits>)
[[nodiscard]]
constexpr auto
bit_width(Bits& bits) noexcept -> idx {
   return bit_width(bit_span(bits));
}

template <is_unsigned_integral Storage>
[[nodiscard]]
constexpr auto
countl_zero(bit_span<Storage> bits) noexcept -> idx {
   return bits.countl_zero();
}

template <typename Bits>
   requires(detail::bit_span_constructible<Bits>)
[[nodiscard]]
constexpr auto
countl_zero(Bits& bits) noexcept -> idx {
   return countl_zero(bit_span(bits));
}

template <is_unsigned_integral Storage>
[[nodiscard]]
constexpr auto
countl_one(bit_span<Storage> bits) noexcept -> idx {
   return bits.countl_one();
}

template <typename Bits>
   requires(detail::bit_span_constructible<Bits>)
[[nodiscard]]
constexpr auto
countl_one(Bits& bits) noexcept -> idx {
   return countl_one(bit_span(bits));
}

template <is_unsigned_integral Storage>
[[nodiscard]]
constexpr auto
countr_zero(bit_span<Storage> bits) noexcept -> idx {
   return bits.countr_zero();
}

template <typename Bits>
   requires(detail::bit_span_constructible<Bits>)
[[nodiscard]]
constexpr auto
countr_zero(Bits& bits) noexcept -> idx {
   return countr_zero(bit_span(bits));
}

template <is_unsigned_integral Storage>
[[nodiscard]]
constexpr auto
countr_one(bit_span<Storage> bits) noexcept -> idx {
   return bits.countr_one();
}

template <typename Bits>
   requires(detail::bit_span_constructible<Bits>)
[[nodiscard]]
constexpr auto
countr_one(Bits& bits) noexcept -> idx {
   return countr_one(bit_span(bits));
}

}  // namespace cat
