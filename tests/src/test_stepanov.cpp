#include <cat/array>
#include <cat/stepanov>

#include "../unit_tests.hpp"

$test(iterator_concepts_std_parity_pointers) {
   static_assert(cat::is_same<cat::iter_value_t<int*>, int>);
   static_assert(cat::is_same<cat::iter_reference_t<int*>, int&>);
   static_assert(cat::is_same<cat::iter_difference_t<int*>, cat::iword>);
   static_assert(cat::is_same<cat::iter_rvalue_reference_t<int*>, int&&>);
   static_assert(cat::is_same<cat::iter_value_t<int const*>, int>);
   static_assert(cat::is_same<cat::iter_reference_t<int const*>, int const&>);
   static_assert(cat::is_stepanov_weakly_incrementable<int*>);
   static_assert(cat::is_stepanov_indirectly_readable<int*>);
   static_assert(cat::is_input_stepanov_iterator<int*>);
   static_assert(cat::is_stepanov_incrementable<int*>);
   static_assert(cat::is_forward_stepanov_iterator<int*>);
   static_assert(cat::is_bidirectional_stepanov_iterator<int*>);
   static_assert(cat::is_random_access_stepanov_iterator<int*>);
   static_assert(cat::is_contiguous_stepanov_iterator<int*>);
}

$test(iterator_concepts_std_parity_container_stepanov_iterator) {
   cat::array<int, 3u> arr{};
   using iter = decltype(arr.begin());

   static_assert(cat::is_stepanov_iterable<decltype(arr)>);
   static_assert(cat::is_stepanov_bidi_iterable<decltype(arr)>);

   static_assert(cat::is_same<cat::iter_value_t<iter>, int>);
   static_assert(cat::is_same<cat::iter_difference_t<iter>, cat::iword>);
   static_assert(cat::is_same<cat::iter_rvalue_reference_t<iter>, int&&>);
   static_assert(cat::is_stepanov_weakly_incrementable<iter>);
   static_assert(cat::is_stepanov_indirectly_readable<iter>);
   static_assert(cat::is_input_stepanov_iterator<iter>);
   static_assert(cat::is_stepanov_incrementable<iter>);
   static_assert(cat::is_forward_stepanov_iterator<iter>);
   static_assert(cat::is_bidirectional_stepanov_iterator<iter>);
   static_assert(cat::is_random_access_stepanov_iterator<iter>);
   static_assert(!cat::is_contiguous_stepanov_iterator<iter>);
}
