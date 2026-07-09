// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/span>

namespace cat {

template <idx bits_count>
class bitset;

template <is_unsigned_integral Storage>
class bit_span;

template <is_unsigned_integral Storage>
[[nodiscard]]
constexpr auto
popcount(bit_span<Storage> bits) -> idx;

namespace detail {
template <typename WordStorage>
concept has_bit_span_value_type =
   requires { typename remove_cvref<WordStorage>::value_type; };

template <typename WordStorage, typename Storage>
concept bit_span_word_container =
   requires(WordStorage& storage) {
      storage.data();
      storage.size();
   } && has_bit_span_value_type<WordStorage>
   && is_unsigned_integral<typename remove_cvref<WordStorage>::value_type>
   && !requires { typename remove_cvref<WordStorage>::abi_type; }
   && is_convertible<decltype(declval<WordStorage&>().data()), Storage*>;

template <typename WordStorage, typename Storage>
concept bit_span_bit_container =
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
   using const_iterator =
      bit_stepanov_iterator<const_word_iterator>;
   using reverse_iterator = reverse_proxy_stepanov_iterator<iterator>;
   using const_reverse_iterator =
      reverse_proxy_stepanov_iterator<const_iterator>;

   constexpr bit_span() = default;
   constexpr bit_span(bit_span const&) = default;
   constexpr bit_span(bit_span&&) = default;

   constexpr bit_span(decltype(nullptr)) {
   }

   constexpr bit_span(
      Storage* _Nullable p_storage [[clang::lifetimebound]], idx bit_offset,
      idx bit_count
   )
       : m_p_storage(p_storage),
         m_bit_offset(bit_offset),
         m_bit_size(bit_count) {
   }

   template <idx bits_count>
   constexpr explicit bit_span(
      bitset<bits_count>& storage [[clang::lifetimebound]]
   );

   template <idx bits_count>
   constexpr explicit bit_span(
      bitset<bits_count> const& storage [[clang::lifetimebound]]
   );

   template <typename WordStorage>
      requires(
         detail::bit_span_word_container<WordStorage, Storage>
         || detail::bit_span_bit_container<WordStorage, Storage>
      )
   constexpr explicit bit_span(
      WordStorage& storage [[clang::lifetimebound]]
   )
       : bit_span(
            storage.data(), bit_offset_for(storage), bit_count_for(storage)
         ) {
   }

   template <typename WordStorage>
      requires(
         detail::bit_span_word_container<WordStorage, Storage>
         || detail::bit_span_bit_container<WordStorage, Storage>
      )
   constexpr bit_span(
      WordStorage& storage [[clang::lifetimebound]],
      idx bit_offset, idx bit_count
   )
       : bit_span(storage.data(), bit_offset, bit_count) {
   }

   constexpr auto
   operator=(bit_span const&) -> bit_span& = default;

   constexpr auto
   operator=(bit_span&&) -> bit_span& = default;

   [[nodiscard]]
   constexpr auto
   size() const -> idx {
      return m_bit_size;
   }

   [[nodiscard]]
   constexpr auto
   data() const -> Storage* _Nullable {
      return m_p_storage;
   }

   [[nodiscard]]
   constexpr auto
   begin() const -> iterator {
      return iterator(
         word_iterator(m_bit_offset / limits<Storage>::bits, m_p_storage),
         m_bit_offset % limits<Storage>::bits
      );
   }

   [[nodiscard]]
   constexpr auto
   end() const -> iterator {
      idx const bit_end = m_bit_offset + m_bit_size;
      return iterator(
         word_iterator(bit_end / limits<Storage>::bits, m_p_storage),
         bit_end % limits<Storage>::bits
      );
   }

   [[nodiscard]]
   constexpr auto
   cbegin() const -> const_iterator {
      return const_iterator(
         const_word_iterator(
            m_bit_offset / limits<Storage>::bits, m_p_storage
         ),
         m_bit_offset % limits<Storage>::bits
      );
   }

   [[nodiscard]]
   constexpr auto
   cend() const -> const_iterator {
      idx const bit_end = m_bit_offset + m_bit_size;
      return const_iterator(
         const_word_iterator(bit_end / limits<Storage>::bits, m_p_storage),
         bit_end % limits<Storage>::bits
      );
   }

   [[nodiscard]]
   constexpr auto
   rbegin() const -> reverse_iterator {
      if !consteval {
         assert(m_bit_size > 0u);
      }
      iterator iter = end();
      --iter;
      return reverse_iterator(iter);
   }

   [[nodiscard]]
   constexpr auto
   rend() const -> reverse_iterator {
      iterator iter = begin();
      --iter;
      return reverse_iterator(iter);
   }

   [[nodiscard]]
   constexpr auto
   crbegin() const -> const_reverse_iterator {
      if !consteval {
         assert(m_bit_size > 0u);
      }
      const_iterator iter = cend();
      --iter;
      return const_reverse_iterator(iter);
   }

   [[nodiscard]]
   constexpr auto
   crend() const -> const_reverse_iterator {
      const_iterator iter = cbegin();
      --iter;
      return const_reverse_iterator(iter);
   }

   [[nodiscard]]
   constexpr auto
   bit_offset() const -> idx {
      return m_bit_offset;
   }

   [[nodiscard]]
   constexpr auto
   operator[](idx bit_index) const [[clang::lifetimebound]] -> reference {
      if !consteval {
         assert(bit_index < m_bit_size);
      }
      return *make_bit_ptr_from_offset(m_p_storage, m_bit_offset + bit_index);
   }

   [[nodiscard]]
   constexpr auto
   slice(idx first, idx last) const -> bit_span {
      if !consteval {
         assert(first <= last);
         assert(last <= m_bit_size);
      }
      return bit_span(m_p_storage, m_bit_offset + first, idx(last - first));
   }

   // Count set bits in this bit span.
   [[nodiscard]]
   constexpr auto
   popcount() const -> idx {
      return cat::popcount(*this);
   }

   [[nodiscard]]
   constexpr auto
   has_single_bit() const -> bool {
      return popcount() == 1u;
   }

   [[nodiscard]]
   constexpr auto
   countl_zero() const -> idx {
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
   countl_one() const -> idx {
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
   countr_zero() const -> idx {
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
   countr_one() const -> idx {
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
   bit_width() const -> idx {
      return idx(m_bit_size - countl_zero());
   }

   constexpr auto
   fill(bit_value value = true) const -> bit_span {
      for (idx i = 0u; i < m_bit_size; ++i) {
         (*this)[i] = value;
      }
      return *this;
   }

   constexpr auto
   clear() const -> bit_span {
      return fill(false);
   }

   constexpr auto
   copy_from(bit_span source) const -> bit_span {
      if !consteval {
         assert(source.size() == m_bit_size);
      }
      if (
         (m_bit_offset % limits<Storage>::bits) == 0u
         && (source.m_bit_offset % limits<Storage>::bits) == 0u
      ) {
         idx const words = div_ceil(m_bit_size, limits<Storage>::bits);
         Storage* _Nonnull const p_destination =
            m_p_storage + (m_bit_offset / limits<Storage>::bits);
         Storage const* _Nonnull const p_source =
            source.m_p_storage + (source.m_bit_offset / limits<Storage>::bits);
         for (idx i = 0u; i < words; ++i) {
            p_destination[i] = p_source[i];
         }
         if (words != 0u) {
            idx const tail_bits = m_bit_size % limits<Storage>::bits;
            if (tail_bits != 0u) {
               p_destination[idx(words - 1u)] &=
                  (Storage(1u) << tail_bits) - Storage(1u);
            }
         }
         return *this;
      }
      for (idx i = 0u; i < m_bit_size; ++i) {
         (*this)[i] = source[i];
      }
      return *this;
   }

   constexpr auto
   move_from(bit_span source) const -> bit_span {
      return copy_from(source);
   }

   [[nodiscard]]
   constexpr auto
   equal(bit_span other) const -> bool {
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

   constexpr auto
   shift_left(idx count) const -> bit_span {
      if (count >= m_bit_size) {
         return clear();
      }
      for (idx i = 0u; i < m_bit_size - count; ++i) {
         (*this)[i] = (*this)[i + count];
      }
      for (idx i = idx(m_bit_size - count); i < m_bit_size; ++i) {
         (*this)[i] = false;
      }
      return *this;
   }

   constexpr auto
   shift_right(idx count) const -> bit_span {
      if (count >= m_bit_size) {
         return clear();
      }
      for (iword i = iword(m_bit_size - count); i > 0; --i) {
         idx const source = idx(i - 1);
         (*this)[source + count] = (*this)[source];
      }
      for (idx i = 0u; i < count; ++i) {
         (*this)[i] = false;
      }
      return *this;
   }

   constexpr auto
   reverse_inplace() const -> bit_span {
      cat::stepanov_reverse_inplace(begin(), end());
      return *this;
   }

   constexpr auto
   rotate_left(idx count) const -> bit_span {
      if (m_bit_size == 0u) {
         return *this;
      }
      count %= m_bit_size;
      if (count == 0u) {
         return *this;
      }
      slice(0u, count).reverse_inplace();
      slice(count, m_bit_size).reverse_inplace();
      reverse_inplace();
      return *this;
   }

   constexpr auto
   rotate_right(idx count) const -> bit_span {
      if (m_bit_size == 0u) {
         return *this;
      }
      count %= m_bit_size;
      if (count == 0u) {
         return *this;
      }
      rotate_left(idx(m_bit_size - count));
      return *this;
   }

   template <typename Callback>
   constexpr auto
   transform(Callback callback) const -> bit_span {
      for (idx i = 0u; i < m_bit_size; ++i) {
         (*this)[i] = callback(bit_value{(*this)[i]});
      }
      return *this;
   }

 private:
   template <typename WordStorage>
   [[nodiscard]]
   static constexpr auto
   bit_count_for(WordStorage& storage) -> idx {
      if constexpr (requires { storage.word_count(); }) {
         return storage.size();
      } else {
         return storage.size() * limits<Storage>::bits;
      }
   }

   template <typename WordStorage>
   [[nodiscard]]
   static constexpr auto
   bit_offset_for(WordStorage& storage) -> idx {
      if constexpr (requires { WordStorage::leading_skipped_bits; }) {
         return WordStorage::leading_skipped_bits;
      } else {
         static_cast<void>(storage);
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
popcount(bit_span<Storage> bits) -> idx {
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
popcount(Bits& bits) -> idx {
   return popcount(bit_span(bits));
}

template <is_unsigned_integral Storage>
[[nodiscard]]
constexpr auto
has_single_bit(bit_span<Storage> bits) -> bool {
   return bits.has_single_bit();
}

template <typename Bits>
   requires(detail::bit_span_constructible<Bits>)
[[nodiscard]]
constexpr auto
has_single_bit(Bits& bits) -> bool {
   return has_single_bit(bit_span(bits));
}

template <is_unsigned_integral Storage>
[[nodiscard]]
constexpr auto
bit_width(bit_span<Storage> bits) -> idx {
   return bits.bit_width();
}

template <typename Bits>
   requires(detail::bit_span_constructible<Bits>)
[[nodiscard]]
constexpr auto
bit_width(Bits& bits) -> idx {
   return bit_width(bit_span(bits));
}

template <is_unsigned_integral Storage>
[[nodiscard]]
constexpr auto
countl_zero(bit_span<Storage> bits) -> idx {
   return bits.countl_zero();
}

template <typename Bits>
   requires(detail::bit_span_constructible<Bits>)
[[nodiscard]]
constexpr auto
countl_zero(Bits& bits) -> idx {
   return countl_zero(bit_span(bits));
}

template <is_unsigned_integral Storage>
[[nodiscard]]
constexpr auto
countl_one(bit_span<Storage> bits) -> idx {
   return bits.countl_one();
}

template <typename Bits>
   requires(detail::bit_span_constructible<Bits>)
[[nodiscard]]
constexpr auto
countl_one(Bits& bits) -> idx {
   return countl_one(bit_span(bits));
}

template <is_unsigned_integral Storage>
[[nodiscard]]
constexpr auto
countr_zero(bit_span<Storage> bits) -> idx {
   return bits.countr_zero();
}

template <typename Bits>
   requires(detail::bit_span_constructible<Bits>)
[[nodiscard]]
constexpr auto
countr_zero(Bits& bits) -> idx {
   return countr_zero(bit_span(bits));
}

template <is_unsigned_integral Storage>
[[nodiscard]]
constexpr auto
countr_one(bit_span<Storage> bits) -> idx {
   return bits.countr_one();
}

template <typename Bits>
   requires(detail::bit_span_constructible<Bits>)
[[nodiscard]]
constexpr auto
countr_one(Bits& bits) -> idx {
   return countr_one(bit_span(bits));
}

}  // namespace cat
