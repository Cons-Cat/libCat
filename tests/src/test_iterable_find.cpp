#include <cat/iterable>
#include <cat/maybe>
#include <cat/utility>

#include "../unit_tests.hpp"

// Reuse the collection and iterable-only fixtures from `test_iterable.cpp`.
template <typename T, cat::idx extent>
struct tiny_array
    : cat::contiguous_collection_interface<tiny_array<T, extent>> {
   T m_data[extent]{};

   [[nodiscard]]
   constexpr auto
   data() -> T* {
      return m_data;
   }

   [[nodiscard]]
   constexpr auto
   data() const -> T const* {
      return m_data;
   }

   [[nodiscard]]
   constexpr auto
   size() const -> cat::idx {
      return extent;
   }
};

template <typename T, cat::idx capacity>
class tiny_list : public cat::iterable_interface<> {
 private:
   struct node {
      T value{};
      cat::idx next = capacity;
   };

   node m_nodes[capacity]{};
   cat::idx m_size = 0u;
   cat::idx m_head = capacity;

 public:
   constexpr auto
   push_back(T value) -> void {
      cat::verify(m_size < capacity);
      cat::idx const slot = m_size;
      m_nodes[slot] = node{value, capacity};
      ++m_size;
      if (m_head == capacity) {
         m_head = slot;
         return;
      }
      cat::idx tail = m_head;
      while (m_nodes[tail].next != capacity) {
         tail = m_nodes[tail].next;
      }
      m_nodes[tail].next = slot;
   }

   class iteration_context {
    private:
      tiny_list const* m_p_list;
      cat::idx m_current;

    public:
      using element_type = T const&;

      constexpr iteration_context(tiny_list const* p_list, cat::idx start)
          : m_p_list(p_list), m_current(start) {
      }

      iteration_context(iteration_context const&) = delete;
      iteration_context(iteration_context&&) = delete;
      auto
      operator=(iteration_context const&) -> iteration_context& = delete;
      auto
      operator=(iteration_context&&) -> iteration_context& = delete;

      template <cat::is_predicate<element_type> Pred>
      constexpr auto
      run_while(Pred&& pred) -> cat::iteration_result {
         while (m_current != capacity) {
            cat::idx const here = m_current;
            m_current = m_p_list->m_nodes[here].next;
            if (!pred(m_p_list->m_nodes[here].value)) {
               return cat::iteration_result::incomplete;
            }
         }
         return cat::iteration_result::complete;
      }
   };

   constexpr auto
   iterate() const -> iteration_context {
      return iteration_context{this, m_head};
   }
};

constexpr auto
find_literal_constexpr() -> bool {
   constexpr char haystack[] = "abc abc";
   cat::span<char const> input{haystack};
   return (input | cat::find("abc")).value() == 0u
          && (input | cat::find("missing")).is_empty();
}

static_assert(find_literal_constexpr());

$test(flux_find_value_collection) {
   tiny_array<int, 6u> arr = {
      {},
      {10, 20, 30, 40, 50, 60}
   };

   auto found = arr | cat::find(30);
   cat::verify(found.has_value());
   cat::verify(found.value() == 2u);

   auto missed = arr | cat::find(999);
   cat::verify(missed.is_empty());
}

$test(flux_find_span) {
   int const haystack[] = {1, 2, 3, 4, 2, 3};
   static constexpr int needle[] = {2, 3};
   cat::span<int const> input{haystack};
   cat::span<int const> subspan{needle};
   cat::verify((input | cat::find(subspan)).value() == 1u);
}

$test(flux_find_zstr_span_and_literal) {
   char const haystack[] = "abc abc";
   cat::span<char const> input{haystack};
   cat::zstr_view needle{"abc"};
   char mutable_needle[] = "abc";
   cat::zstr_span mutable_span{mutable_needle, 4u};
   cat::verify((input | cat::find(needle)).value() == 0u);
   cat::verify((input | cat::find(mutable_span)).value() == 0u);
   cat::verify((input | cat::find("abc")).value() == 0u);
   cat::verify((input | cat::find("missing")).is_empty());
   cat::verify((input | cat::find("")).value() == 0u);
}

$test(flux_find_first_element) {
   tiny_array<int, 4u> arr = {
      {},
      {7, 8, 9, 10}
   };

   auto found = arr | cat::find(7);
   cat::verify(found.has_value());
   cat::verify(found.value() == 0u);
}

$test(flux_find_byte_with_character_needle) {
   tiny_array<cat::byte, 4u> arr{};
   arr.m_data[0u] = cat::byte{'a'};
   arr.m_data[1u] = cat::byte{'b'};
   arr.m_data[2u] = cat::byte{'c'};
   arr.m_data[3u] = cat::byte{'d'};
   cat::verify((arr | cat::find('c')).value() == 2u);
   cat::verify((arr | cat::find('z')).is_empty());
}

$test(flux_find_empty) {
   tiny_array<int, 4u> arr = {
      {},
      {0, 0, 0, 0}
   };

   auto present = arr | cat::find(0);
   cat::verify(present.has_value());
   cat::verify(present.value() == 0u);

   auto empty_view = arr | cat::take(0u);
   auto missed = empty_view | cat::find(0);
   cat::verify(missed.is_empty());
}

$test(flux_find_iterable_only) {
   tiny_list<int, 8u> list;
   list.push_back(10);
   list.push_back(20);
   list.push_back(30);
   list.push_back(40);

   auto found = list | cat::find(30);
   cat::verify(found.has_value());
   cat::verify(found.value() == 2u);

   auto missed = list | cat::find(999);
   cat::verify(missed.is_empty());
}

$test(flux_find_composes_with_filter) {
   tiny_array<int, 6u> arr = {
      {},
      {1, 2, 3, 4, 5, 6}
   };

   auto found = arr | cat::filter(cat::is_even) | cat::find(4);
   cat::verify(found.has_value());
   cat::verify(found.value() == 1u);

   auto missed = arr | cat::filter(cat::is_even) | cat::find(5);
   cat::verify(missed.is_empty());
}
