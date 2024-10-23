#include <cat/linear_allocator>
#include <cat/page_allocator>
#include <cat/vec>

#include "../unit_tests.hpp"

// Test that `vec` works in a `constexpr` context.
consteval auto
const_func() -> int4 {
   cat::page_allocator allocator;
   cat::vec vector = cat::make_vec_empty<int4>(allocator);
   auto _ = vector.resize(8);

   vector[0] = 1;
   vector[1] = 2;
   vector[7] = 10;
   auto _ = vector.push_back(10);
   return vector[8];
}

TEST(test_vec) {
   // Initialize an allocator.
   cat::page_allocator pager;
   cat::span page = pager.alloc_multi<cat::byte>(4_uki).or_exit();
   defer {
      pager.free(page);
   };
   auto allocator = cat::make_linear_allocator(page);

   // Test default constructing a `vector`.
   cat::vec int_vec = cat::make_vec_empty<int4>(allocator);
   cat::verify(int_vec.size() == 0);
   cat::verify(int_vec.capacity() >= 0);

   // Test pushing back to a `vector`.
   int_vec.push_back(1).or_exit();
   int_vec.push_back(2).or_exit();
   int_vec.push_back(3).or_exit();
   cat::verify(int_vec.size() == 3);
   cat::verify(int_vec.capacity() >= 4);

   int_vec.push_back(6).or_exit();
   int_vec.push_back(12).or_exit();
   int_vec.push_back(24).or_exit();
   cat::verify(int_vec.size() == 6);
   cat::verify(int_vec.capacity() >= 8);

   // Test resizing a `vector`.
   int_vec.resize(0).or_exit();
   cat::verify(int_vec.size() == 0);
   cat::verify(int_vec.capacity() >= 8);

   int_vec.resize(4).or_exit();
   cat::verify(int_vec.size() == 4);
   cat::verify(int_vec.capacity() >= 8);

   // Test reserving storage for a `vector`.
   int_vec.reserve(128).or_exit();
   cat::verify(int_vec.size() == 4);
   cat::verify(int_vec.capacity() >= 128);

   // Test reserve constructor.
   cat::vec reserved_vec = cat::make_vec_reserved<int4>(allocator, 6).or_exit();
   cat::verify(reserved_vec.capacity() >= 6);

   // Test filled constructor.
   cat::vec filled_vec = cat::make_vec_filled(allocator, 8, 1_i4).or_exit();
   cat::verify(filled_vec.size() == 8);
   cat::verify(filled_vec.capacity() >= 8);
   for (int4 integer : filled_vec) {
      cat::verify(integer == 1);
   }

   // Test cloned constructor.
   cat::vec cloned_vec = filled_vec.clone(allocator).or_exit();
   cat::verify(cloned_vec.size() == 8);
   cat::verify(cloned_vec.capacity() >= 8);
   for (int4 integer : cloned_vec) {
      cat::verify(integer == 1);
   }

   // Test vector member types.
   using iterator = cat::vec<int4>::iterator;
   static_assert(cat::is_same<iterator, decltype(int_vec.begin())>);
   static_assert(cat::is_same<iterator, decltype(int_vec.end())>);

   static_assert(cat::is_same<iterator::value_type, int4>);
   static_assert(cat::is_same<iterator::reference, int4&>);

   using const_iterator = cat::vec<int4>::const_iterator;
   static_assert(cat::is_same<const_iterator, decltype(int_vec.cbegin())>);
   static_assert(cat::is_same<const_iterator, decltype(int_vec.cend())>);

   static_assert(cat::is_same<const_iterator::value_type, int4 const>);
   static_assert(cat::is_same<const_iterator::reference, int4 const&>);

   using reverse_iterator = cat::vec<int4>::reverse_iterator;
   static_assert(cat::is_same<reverse_iterator, decltype(int_vec.rbegin())>);
   static_assert(cat::is_same<reverse_iterator, decltype(int_vec.rend())>);

   static_assert(cat::is_same<reverse_iterator::value_type, int4>);
   static_assert(cat::is_same<reverse_iterator::reference, int4&>);

   using const_reverse_iterator = cat::vec<int4>::const_reverse_iterator;
   static_assert(
      cat::is_same<const_reverse_iterator, decltype(int_vec.crbegin())>);
   static_assert(
      cat::is_same<const_reverse_iterator, decltype(int_vec.crend())>);

   static_assert(cat::is_same<const_reverse_iterator::value_type, int4 const>);
   static_assert(cat::is_same<const_reverse_iterator::reference, int4 const&>);

   static_assert(cat::is_same<int, cat::vec<int>::value_type>);

   // Test `vector` in a `constexpr` context.
   static_assert(const_func() == 10);

   // Test getters.
   cat::vec default_vector = cat::make_vec_empty<int>(allocator);
   cat::verify(default_vector.is_empty());

   auto _ = default_vector.reserve(2);
   cat::verify(default_vector.is_empty());

   auto _ = default_vector.push_back(0);
   auto _ = default_vector.push_back(0);
   cat::verify(!default_vector.is_empty());

   // Resize the vector to be larger, then check it's full.
   auto _ = default_vector.resize(default_vector.capacity() + 1u).verify();
   cat::verify(default_vector.is_full());

   // Resize the vector to be smaller, then check it's not full.
   default_vector.resize(2).verify();
   cat::verify(!default_vector.is_full());

   // TODO: Test insert iterators.

   // Test algorithms.
   cat::vec origin_vector = cat::make_vec_filled(allocator, 6, 1).verify();
   auto copy_vector = cat::make_vec_filled(allocator, 6, 0).verify();
   auto move_vector = cat::make_vec_filled(allocator, 6, 0).verify();
   auto relocate_vector = cat::make_vec_filled(allocator, 6, 0).verify();

   // `copy()`.
   cat::verify(copy_vector[5] == 0);
   cat::copy(origin_vector.begin(), origin_vector.end(), copy_vector.begin());
   cat::verify(copy_vector[5] == 1);

   copy_vector[5] = 0;
   origin_vector.copy_to(copy_vector);
   cat::verify(copy_vector[5] == 1);

   // `move()`.
   cat::verify(move_vector[5] == 0);
   cat::move(origin_vector.begin(), origin_vector.end(), move_vector.begin());
   cat::verify(move_vector[5] == 1);

   move_vector[5] = 0;
   origin_vector.move_to(move_vector);
   cat::verify(move_vector[5] == 1);

   // `relocate()`.
   cat::verify(relocate_vector[5] == 0);
   cat::relocate(origin_vector.begin(), origin_vector.end(),
                 relocate_vector.begin());
   cat::verify(relocate_vector[5] == 1);

   relocate_vector[5] = 0;
   origin_vector.relocate_to(relocate_vector);
   cat::verify(relocate_vector[5] == 1);
}
