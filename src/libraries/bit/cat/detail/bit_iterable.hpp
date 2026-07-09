// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

namespace cat {

struct byte;

template <typename Iterator>
// Only storage types can be iterated bitwise. Like other
// `proxy_stepanov_iterator_interface` adaptors, the wrapped iterator must model
// random access (`advance` uses += on the underlying iterator).
   requires(
      is_random_access_stepanov_iterator<Iterator>
      && is_unsigned_integral<typename Iterator::value_type>
      /* || is_same<typename iterator::value_type, byte> */
   )
class bit_stepanov_iterator
    : public proxy_stepanov_iterator_interface<
         random_access_iterator_tag, bit_value,
         bit_reference<typename Iterator::value_type>, iword> {
 private:
   static constexpr idx word_bits = limits<typename Iterator::value_type>::bits;

 public:
   using storage_type = Iterator::value_type;
   using value_type = bit_value;
   using const_value_type = bit_value const;
   using reference = bit_reference<typename Iterator::value_type>;
   using const_reference =
      bit_reference<remove_cvref<typename Iterator::value_type> const>;

   constexpr bit_stepanov_iterator() = default;
   constexpr bit_stepanov_iterator(bit_stepanov_iterator const&) = default;
   constexpr bit_stepanov_iterator(bit_stepanov_iterator&&) = default;

   constexpr explicit bit_stepanov_iterator(
      Iterator const& other, idx current_bit = 0u
   )
       : m_internal_iterator(other), m_bit_position(current_bit) {
   }

   constexpr auto
   dereference() -> reference {
      return make_bit_reference_from_offset(
         *m_internal_iterator, m_bit_position
      );
   }

   constexpr auto
   dereference() const -> const_reference {
      return make_bit_reference_from_offset(
         *m_internal_iterator, m_bit_position
      );
   }

   [[nodiscard]]
   constexpr auto
   base() const -> Iterator {
      return m_internal_iterator;
   }

   [[nodiscard]]
   constexpr auto
   position() const -> idx {
      return m_bit_position;
   }

   constexpr void
   advance(iword offset) {
      iword advance_bits = m_bit_position + offset;
      iword bytes_overflow = advance_bits / word_bits;
      if (advance_bits < 0 && ((bytes_overflow * word_bits) != advance_bits)) {
         --bytes_overflow;
      }
      m_internal_iterator += bytes_overflow.raw;
      m_bit_position = idx(advance_bits - bytes_overflow * word_bits);
   }

   constexpr auto
   distance_to(bit_stepanov_iterator const& other) const -> iword {
      iword const word_difference =
         other.m_internal_iterator - m_internal_iterator;
      return (word_difference * word_bits) + other.m_bit_position
             - m_bit_position;
   }

 private:
   Iterator m_internal_iterator;
   idx m_bit_position;
};

namespace detail {
template <is_unsigned_integral Word>
[[nodiscard]]
constexpr auto
bit_low_mask(idx bit_count) -> Word {
   if (bit_count == 0u) {
      return Word(0u);
   }
   if (bit_count >= limits<Word>::bits) {
      return ~Word(0u);
   }
   return (Word(1u) << bit_count) - Word(1u);
}

template <is_unsigned_integral Word>
[[nodiscard]]
constexpr auto
bit_blend(Word old_word, Word new_word, idx bit_offset, idx bit_count) -> Word {
   Word const mask = bit_low_mask<Word>(bit_count) << bit_offset;
   return (old_word & ~mask) | (new_word & mask);
}

template <typename Iterator>
constexpr void
reverse_bit_words(Iterator const& first_word, idx word_count) {
   idx left = 0u;
   idx right = word_count;
   while (left < right) {
      right = idx(right - 1u);
      if (left == right) {
         first_word[left] = bit_reverse(first_word[left]);
         return;
      }
      auto const first_value = first_word[left];
      first_word[left] = bit_reverse(first_word[right]);
      first_word[right] = bit_reverse(first_value);
      ++left;
   }
}

template <typename Iterator>
constexpr void
shift_bit_words_left(Iterator first_word, idx word_count, idx bit_count) {
   using word_type = Iterator::value_type;
   iword word_index = word_count;
   while (word_index > 0u) {
      --word_index;
      word_type shifted = first_word[word_index] << bit_count;
      if (word_index > 0u) {
         shifted |= first_word[idx(word_index - 1u)]
                    >> (limits<word_type>::bits - bit_count);
      }
      first_word[idx(word_index)] = shifted;
   }
}

template <typename Iterator>
constexpr void
shift_bit_words_right(Iterator first_word, idx word_count, idx bit_count) {
   using word_type = Iterator::value_type;
   for (idx word_index = 0u; word_index < word_count; ++word_index) {
      word_type shifted = first_word[word_index] >> bit_count;
      if (word_index + 1u < word_count) {
         shifted |= first_word[word_index + 1u]
                    << (limits<word_type>::bits - bit_count);
      }
      first_word[word_index] = shifted;
   }
}

template <typename Iterator>
constexpr void
shift_left(
   bit_stepanov_iterator<Iterator> first, bit_stepanov_iterator<Iterator> last,
   idx bit_count
) {
   iword const range_bits = last - first;
   if (bit_count == 0u || range_bits <= 0) {
      return;
   }
   if (bit_count >= range_bits) {
      for (idx bit_index = 0u; bit_index < idx(range_bits); ++bit_index) {
         first[bit_index] = false;
      }
      return;
   }
   idx const remaining_bits = idx(range_bits - bit_count);
   for (idx bit_index = 0u; bit_index < remaining_bits; ++bit_index) {
      first[bit_index] = first[bit_index + bit_count];
   }
   for (idx bit_index = remaining_bits; bit_index < idx(range_bits);
        ++bit_index) {
      first[bit_index] = false;
   }
}

template <typename Iterator>
constexpr void
shift_right(
   bit_stepanov_iterator<Iterator> first, bit_stepanov_iterator<Iterator> last,
   idx bit_count
) {
   iword const range_bits = last - first;
   if (bit_count == 0u || range_bits <= 0) {
      return;
   }
   if (bit_count >= range_bits) {
      for (idx bit_index = 0u; bit_index < idx(range_bits); ++bit_index) {
         first[bit_index] = false;
      }
      return;
   }
   idx const remaining_bits = idx(range_bits - bit_count);
   for (idx bit_index = remaining_bits; bit_index > 0u;) {
      bit_index = idx(bit_index - 1u);
      first[bit_index + bit_count] = first[bit_index];
   }
   for (idx bit_index = 0u; bit_index < bit_count; ++bit_index) {
      first[bit_index] = false;
   }
}

template <typename Iterator>
constexpr void
reverse_bits_inplace(
   bit_stepanov_iterator<Iterator> first, bit_stepanov_iterator<Iterator> last
) {
   iword const range_bits = last - first;
   if (range_bits <= 1) {
      return;
   }

   using word_type = Iterator::value_type;
   constexpr idx word_bits = limits<word_type>::bits;
   idx const bit_count = idx(range_bits);
   idx const prefix_bits = first.position();
   idx const touched_bits = prefix_bits + bit_count;
   idx word_count = touched_bits / word_bits;
   if ((touched_bits % word_bits) != 0u) {
      ++word_count;
   }
   idx const suffix_bits =
      idx(word_count * word_bits - prefix_bits - bit_count);
   Iterator first_word = first.base();
   word_type const original_first = *first_word;
   word_type const original_last = first_word[idx(word_count - 1u)];

   reverse_bit_words(first_word, word_count);

   if (prefix_bits > suffix_bits) {
      shift_bit_words_left(
         first_word, word_count, idx(prefix_bits - suffix_bits)
      );
   } else if (suffix_bits > prefix_bits) {
      shift_bit_words_right(
         first_word, word_count, idx(suffix_bits - prefix_bits)
      );
   }

   *first_word = bit_blend(*first_word, original_first, 0u, prefix_bits);
   idx const last_word_bits = idx(word_bits - suffix_bits);
   first_word[idx(word_count - 1u)] = bit_blend(
      original_last, first_word[idx(word_count - 1u)], 0u, last_word_bits
   );
}

template <typename Bits>
class bit_iteration_context {
 private:
   Bits* _Nonnull m_p_bits;
   idx m_position;
   idx m_stop;

 public:
   using element_type = decltype(declval<Bits&>()[idx{}]);

   constexpr bit_iteration_context(Bits& bits, idx begin, idx end)
       : m_p_bits(__builtin_addressof(bits)), m_position(begin), m_stop(end) {
   }

   bit_iteration_context(bit_iteration_context const&) = delete;
   bit_iteration_context(bit_iteration_context&&) = delete;
   auto
   operator=(bit_iteration_context const&) -> bit_iteration_context& = delete;
   auto
   operator=(bit_iteration_context&&) -> bit_iteration_context& = delete;

   template <typename LoopBody>
   constexpr auto
   run_while(LoopBody&& loop_body) -> iteration_result {
      while (m_position < m_stop) {
         idx const position = m_position;
         ++m_position;
         if (!loop_body((*m_p_bits)[position])) {
            return static_cast<iteration_result>(false);
         }
      }
      return static_cast<iteration_result>(true);
   }
};

template <typename Bits>
class bit_reverse_iteration_context {
 private:
   Bits* _Nonnull m_p_bits;
   idx m_position;
   idx m_stop;

 public:
   using element_type = decltype(declval<Bits&>()[idx{}]);

   constexpr bit_reverse_iteration_context(Bits& bits, idx begin, idx end)
       : m_p_bits(__builtin_addressof(bits)), m_position(begin), m_stop(end) {
   }

   bit_reverse_iteration_context(bit_reverse_iteration_context const&) = delete;
   bit_reverse_iteration_context(bit_reverse_iteration_context&&) = delete;
   auto
   operator=(bit_reverse_iteration_context const&)
      -> bit_reverse_iteration_context& = delete;
   auto
   operator=(bit_reverse_iteration_context&&)
      -> bit_reverse_iteration_context& = delete;

   template <typename LoopBody>
   constexpr auto
   run_while(LoopBody&& loop_body) -> iteration_result {
      while (m_stop < m_position) {
         m_position = idx(m_position - 1u);
         if (!loop_body((*m_p_bits)[m_position])) {
            return static_cast<iteration_result>(false);
         }
      }
      return static_cast<iteration_result>(true);
   }
};
}  // namespace detail

template <typename Iterator>
constexpr void
stepanov_reverse_inplace(
   bit_stepanov_iterator<Iterator> first, bit_stepanov_iterator<Iterator> last
) {
   detail::reverse_bits_inplace(first, last);
}

template <typename Derived>
struct bit_collection_interface : iterable_interface<Derived> {
   [[nodiscard]]
   constexpr auto
   begin_pos(this Derived const& self) -> idx {
      static_cast<void>(self);
      return 0u;
   }

   [[nodiscard]]
   constexpr auto
   end_pos(this Derived const& self) -> idx {
      return self.size();
   }

   constexpr void
   inc_pos(idx& position) const {
      ++position;
   }

   constexpr void
   dec_pos(idx& position) const {
      position = idx(position - 1u);
   }

   constexpr void
   offset_pos(idx& position, iword offset) const {
      position = idx(position + offset);
   }

   [[nodiscard]]
   constexpr auto
   distance(idx left, idx right) const -> iword {
      return right - left;
   }

   [[nodiscard]]
   constexpr auto
   read_at_unchecked(this auto& self, idx position) -> decltype(auto) {
      using reference_type = remove_cvref<decltype(self[position])>;
      static_assert(is_specialization<reference_type, bit_reference>);
      return self[position];
   }

   constexpr auto
   iterate(this auto& self) {
      using self_type = remove_reference<decltype(self)>;
      return detail::bit_iteration_context<self_type>{self, 0u, self.size()};
   }

   constexpr auto
   reverse_iterate(this auto& self) {
      using self_type = remove_reference<decltype(self)>;
      return detail::bit_reverse_iteration_context<self_type>{
         self,
         self.size(),
         0u,
      };
   }
};

}  // namespace cat
