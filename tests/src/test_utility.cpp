#include <cat/unique>
#include <cat/utility>

#include "../unit_tests.hpp"

namespace {

struct move_only_value {
   int value;

   constexpr move_only_value(int input) : value(input) {
   }

   move_only_value(move_only_value const&) = delete;

   auto
   operator=(move_only_value const&) -> move_only_value& = delete;

   constexpr move_only_value(move_only_value&& other) : value(other.value) {
      other.value = -1;
   }

   constexpr auto
   operator=(move_only_value&& other) -> move_only_value& {
      value = other.value;
      other.value = -1;
      return *this;
   }
};

}  // namespace

$test(swap) {
   int left = 1;
   int right = 2;
   cat::swap(left, right);
   cat::verify(left == 2);
   cat::verify(right == 1);

   int4 cat_left = 3;
   int4 cat_right = 4;
   cat::swap(cat_left, cat_right);
   cat::verify(cat_left == 4);
   cat::verify(cat_right == 3);

   uint4 unsigned_left = 5u;
   uint4 unsigned_right = 6u;
   cat::swap(unsigned_left, unsigned_right);
   cat::verify(unsigned_left == 6u);
   cat::verify(unsigned_right == 5u);

   idx index_left = 7u;
   idx index_right = 8u;
   cat::swap(index_left, index_right);
   cat::verify(index_left == 8u);
   cat::verify(index_right == 7u);

   move_only_value first{3};
   move_only_value second{4};
   cat::swap(first, second);
   cat::verify(first.value == 4);
   cat::verify(second.value == 3);

   int left_array[] = {1, 2, 3};
   int right_array[] = {4, 5, 6};
   cat::swap(left_array, right_array);
   cat::verify(left_array[0] == 4);
   cat::verify(left_array[2] == 6);
   cat::verify(right_array[0] == 1);
   cat::verify(right_array[2] == 3);

   cat::unique<int> unique_left{7};
   cat::unique<int> unique_right{8};
   cat::swap(unique_left, unique_right);
   cat::verify(unique_left.borrow() == 8);
   cat::verify(unique_right.borrow() == 7);

   cat::unique_weak<int> weak_left{9};
   cat::unique_weak<int> weak_right{10};
   auto _ = weak_right.borrow();
   cat::swap(weak_left, weak_right);
   cat::verify(!weak_left.has_ownership());
   cat::verify(weak_right.has_ownership());
}
