#include <cat/forward_list>
#include <cat/insert_iterators>
#include <cat/iterable>
#include <cat/linear_allocator>
#include <cat/list>
#include <cat/null_allocator>
#include <cat/page_allocator>

#include "../unit_tests.hpp"

namespace {

struct linear_arena {
   cat::page_allocator pager;
   cat::span<cat::byte> page = pager.alloc_multi<cat::byte>(4_uki).verify();
   cat::linear_allocator alloc = cat::make_linear_allocator(page);

   ~linear_arena() {
      pager.free(page);
   }
};

consteval auto
constexpr_list_func() -> int4 {
   cat::page_allocator pager;
   cat::list<int> list;
   auto _ = list.push_back<cat::page_allocator>(pager, 1);
   auto _ = list.push_back<cat::page_allocator>(pager, 2);
   auto _ = list.push_front<cat::page_allocator>(pager, 3);
   list.reverse_inplace();
   int4 result = list.front() + list.back();
   list.free<cat::page_allocator>(pager);
   return result;
}

consteval auto
constexpr_forward_list_func() -> int4 {
   cat::page_allocator pager;
   cat::forward_list<int> list;
   auto _ = list.push_front<cat::page_allocator>(pager, 1);
   auto _ = list.push_front<cat::page_allocator>(pager, 2);
   auto _ = list.emplace_front<cat::page_allocator>(pager, 3);
   int4 result = list.front();
   list.pop_front<cat::page_allocator>(pager);
   result += list.front();
   list.free<cat::page_allocator>(pager);
   return result;
}

}  // namespace

$test(list_constexpr_usage) {
   static_assert(constexpr_list_func() == 5);
   static_assert(constexpr_forward_list_func() == 5);
}

$test(manual_list_modifiers) {
   linear_arena arena;
   auto& allocator = arena.alloc;
   auto allocator_ref = cat::allocator_ref<cat::linear_allocator>(allocator);

   int values[] = {2, 3, 3};
   int prefix[] = {0, 1};
   cat::list<int> list;
   list.assign(allocator_ref, 2u, 5).verify();
   cat::verify(list.size() == 2);
   cat::verify(list.front() == 5);
   cat::verify(list.back() == 5);

   list.assign_range(allocator_ref, cat::span<int const>(values, 3u)).verify();
   cat::verify(list.size() == 3);
   cat::verify(list.front() == 2);
   cat::verify(list.back() == 3);

   list.prepend_range(allocator_ref, cat::span<int const>(prefix, 2u)).verify();
   cat::verify(list.size() == 5);
   cat::verify(list.front() == 0);
   cat::verify(*(list.begin() + 3u) == 3);

   int suffix[] = {4, 4, 5};
   list.append_range(allocator_ref, cat::span<int const>(suffix, 3u)).verify();
   cat::verify(list.back() == 5);
   cat::verify(list.size() == 8);

   cat::verify(list.remove(allocator_ref, 4) == 2);
   cat::verify(list.size() == 6);
   cat::verify(list.remove_if(allocator_ref, [](int value) -> bool {
      return value == 0;
   }) == 1);
   cat::verify(list.front() == 1);

   cat::verify(list.unique_inplace(allocator_ref) == 1);
   cat::verify(list.size() == 4);
   list.reverse_inplace();
   cat::verify(list.front() == 5);
   cat::verify(list.back() == 1);

   auto second = ++list.begin();
   list.erase(allocator_ref, second);
   cat::verify(list.size() == 3);
   cat::verify(*(++list.begin()) == 2);

   list.resize(allocator_ref, 5u, 9).verify();
   cat::verify(list.size() == 5);
   cat::verify(list.back() == 9);
   list.resize(allocator_ref, 2u).verify();
   cat::verify(list.size() == 2);

   cat::list<int> clone = list.clone(allocator_ref).verify();
   cat::verify(clone.size() == list.size());
   cat::verify(clone.front() == list.front());

   clone.free(allocator_ref);
   list.free(allocator_ref);
}

$test(manual_forward_list_modifiers) {
   linear_arena arena;
   auto& allocator = arena.alloc;
   auto allocator_ref = cat::allocator_ref<cat::linear_allocator>(allocator);

   cat::forward_list<int> list =
      cat::make_forward_list<int>(allocator_ref, 2, 4).verify();
   cat::verify(list.size() == 2);
   cat::verify(list.front() == 2);

   auto second = ++list.begin();
   list.insert(allocator_ref, second, 3).verify();
   cat::verify(list.size() == 3);
   cat::verify(*(++list.begin()) == 3);

   list.erase_after(allocator_ref, list.begin());
   cat::verify(list.size() == 2);
   cat::verify(*(++list.begin()) == 4);

   list.emplace(allocator_ref, ++list.begin(), 3).verify();
   cat::forward_list<int> clone = list.clone(allocator_ref).verify();
   cat::verify(clone.size() == 3);
   cat::verify(clone.front() == 2);
   cat::verify(*(clone.begin() + 1u) == 3);
   cat::verify(*(clone.begin() + 2u) == 4);

   list.clear();
   cat::verify(list.size() == 0);
   clone.free(allocator_ref);
   list.free(allocator_ref);
}

$test(list_reverse_inplace) {
   linear_arena arena;
   auto& allocator = arena.alloc;
   auto allocator_ref = cat::allocator_ref<cat::linear_allocator>(allocator);

   cat::list<int> manual =
      cat::make_list<int>(allocator_ref, 1, 2, 3, 4).verify();
   manual | cat::reverse_inplace();
   cat::verify(manual.front() == 4);
   cat::verify(manual.back() == 1);
   manual.reverse_inplace();
   cat::verify(manual.front() == 1);
   cat::verify(manual.back() == 4);

   cat::raii::list managed =
      cat::raii::make_list<int>(allocator, 5, 6, 7).verify();
   managed | cat::reverse_inplace();
   cat::verify(managed.front() == 7);
   cat::verify(managed.back() == 5);
   managed.reverse_inplace();
   cat::verify(managed.front() == 5);
   cat::verify(managed.back() == 7);

   manual.free(allocator_ref);
}

$test(raii_list_modifiers_release) {
   linear_arena arena;
   auto& allocator = arena.alloc;

   cat::raii::list list = cat::raii::make_list<int>(allocator, 1, 2).verify();
   int tail[] = {2, 3, 3, 4};
   list.append_range(cat::span<int const>(tail, 4u)).verify();
   cat::verify(list.size() == 6);

   cat::verify(list.remove(2) == 2);
   cat::verify(list.unique_inplace() == 1);
   cat::verify(list.size() == 3);
   list.reverse_inplace();
   cat::verify(list.front() == 4);
   cat::verify(list.back() == 1);

   list.assign(3u, 7).verify();
   cat::verify(list.size() == 3);
   cat::verify(list.front() == 7);
   list.resize(5u, 8).verify();
   cat::verify(list.back() == 8);
   list.resize(2u).verify();
   cat::verify(list.size() == 2);

   cat::list<int> released = list.release();
   cat::verify(released.size() == 2);
   cat::verify(list.size() == 0);
   released.free(allocator);

   cat::raii::list reset_list =
      cat::raii::make_list<int>(allocator, 1, 2, 3).verify();
   reset_list.reset();
   cat::verify(reset_list.size() == 0);
}

$test(list_factories) {
   linear_arena arena;
   auto& allocator = arena.alloc;
   auto allocator_ref = cat::allocator_ref<cat::linear_allocator>(allocator);

   cat::list manual_list = cat::make_list<int>(allocator_ref, 1, 2, 3).verify();
   cat::verify(manual_list.size() == 3);
   cat::verify(manual_list.front() == 1);
   cat::verify(manual_list.back() == 3);

   cat::list manual_fill =
      cat::make_list_filled<int>(allocator_ref, 3u, 6).verify();
   cat::verify(manual_fill.size() == 3);
   cat::verify(manual_fill.front() == 6);
   cat::verify(manual_fill.back() == 6);

   cat::forward_list manual_forward_list =
      cat::make_forward_list<int>(allocator_ref, 4, 5).verify();
   cat::verify(manual_forward_list.size() == 2);
   cat::verify(manual_forward_list.front() == 4);
   cat::verify(*(++manual_forward_list.begin()) == 5);

   cat::forward_list manual_forward_list_fill =
      cat::make_forward_list_filled<int>(allocator_ref, 2u, 9).verify();
   cat::verify(manual_forward_list_fill.size() == 2);
   cat::verify(manual_forward_list_fill.front() == 9);

   manual_forward_list_fill.free(allocator_ref);
   manual_forward_list.free(allocator_ref);
   manual_fill.free(allocator_ref);
   manual_list.free(allocator_ref);
}

$test(raii_list_maybe_niche) {
   static_assert(
      sizeof(cat::maybe<cat::raii::list<int4>>) == sizeof(cat::raii::list<int4>)
   );
   cat::maybe<cat::raii::list<int4>> empty_list;
   cat::verify(!empty_list.has_value());

   linear_arena arena;
   auto engaged = cat::raii::make_list<int4>(arena.alloc);
   cat::verify(engaged.has_value());
}

$test(list) {
   linear_arena arena;
   auto& allocator = arena.alloc;
   cat::dyn_allocator dynamic_allocator = allocator;
   auto dynamic_ref = cat::allocator_ref<cat::dyn_allocator>(dynamic_allocator);

   cat::raii::list list_1 = cat::raii::make_list<int4>(allocator).verify();
   auto _ = list_1.insert(list_1.begin(), 3).verify();
   auto _ = list_1.insert(list_1.begin(), 2).verify();
   auto _ = list_1.insert(list_1.begin(), 1).verify();
   cat::verify(list_1.front() == 1);
   cat::verify(list_1.back() == 3);

   int i = 1;
   for (auto& node : list_1) {
      cat::verify(node == i);
      ++i;
   }

   list_1.pop_front();
   cat::verify(list_1.front() == 2);
   list_1.pop_back();
   cat::verify(list_1.back() == 2);

   cat::raii::list list_2 = cat::raii::make_list<int4>(allocator).verify();
   auto _ = list_2.push_front(0).verify();
   auto _ = list_2.push_back(4).verify();
   cat::verify(list_2.front() == 0);
   cat::verify(list_2.back() == 4);
   auto _ = list_2.insert(++list_2.begin(), 1).verify();
   cat::verify(list_2.front() == 0);
   cat::verify(*++list_2.begin() == 1);

   for (auto&& node : list_2) {
      auto _ = node;
      asm volatile("nop");
   }

   cat::raii::list<int4, cat::linear_allocator> list_3 =
      cat::raii::make_list<int4, cat::linear_allocator>(allocator).verify();
   auto _ = list_3.emplace_front(1).verify();
   auto _ = list_3.emplace_front(2).verify();
   auto _ = list_3.emplace_back(3).verify();
   auto _ = list_3.emplace(++list_3.begin(), 4);
   cat::verify(list_3.front() == 2);
   cat::verify(list_3.back() == 3);
   cat::verify((*(++list_3.begin())) == 4);

   for (auto&& node : list_3) {
      auto _ = node;
      asm volatile("nop");
   }

   auto _ = list_1.emplace(list_1.begin()++, 0);
   auto _ = list_1.cbegin();
   auto _ = list_1.cend();
   auto _ = list_1.rbegin();
   auto _ = list_1.rend();
   auto iter = list_1.crbegin();
   auto _ = list_1.crend();
   cat::verify(*iter == 2);
   ++iter;
   cat::verify(*iter == 0);

   list_1.erase(list_1.begin());
   for (int i = 0; i < 10; ++i) {
      list_1.pop_front();
   }

   list_1.clear();
   list_2.clear();
   cat::verify(list_2.size() == 0);

   auto _ = list_1.push_front(3).verify();
   auto _ = list_1.push_front(2).verify();
   auto _ = list_1.push_front(1).verify();
   auto _ = list_1.push_front(0).verify();
   cat::raii::list list_5 = list_1.clone(allocator).verify();

   auto list_it_1 = list_1.begin();
   auto list_it_5 = list_5.begin();

   while (list_it_1 != list_1.end()) {
      cat::verify(*list_it_1 == *list_it_5);
      ++list_it_1;
      ++list_it_5;
   }

   list_1.clear();
   cat::verify(*(list_5.begin()) == 0);
   cat::verify(*(list_5.begin() + 1u) == 1);
   cat::verify(*(list_5.begin() + 2u) == 2);
   cat::verify(*(list_5.begin() + 3u) == 3);

   list_1.push_front(2).verify();
   list_1.push_front(1).verify();
   list_1.push_front(0).verify();
   cat::raii::list list_4 = cat::move(list_1);
   cat::verify(list_4.size() == 3);
   cat::verify(list_4.front() == 0);
   cat::verify(*(list_4.begin() + 1u) == 1);
   cat::verify(*(list_4.begin() + 2u) == 2);

   cat::raii::list list_init_1 =
      cat::raii::make_list<int, cat::dyn_allocator>(dynamic_ref, 1).verify();
   cat::verify(list_init_1.size() == 1);
   auto list_it = list_init_1.begin();
   cat::verify(*list_it == 1);

   cat::raii::list list_init_2 =
      cat::raii::make_list<int, cat::dyn_allocator>(dynamic_ref, 1, 2, 3)
         .verify();
   cat::verify(list_init_2.size() == 3);
   list_it = list_init_2.begin();
   cat::verify(*list_it == 1);
   cat::verify(*++list_it == 2);
   cat::verify(*++list_it == 3);

   cat::raii::list swap_left =
      cat::raii::make_list<int, cat::dyn_allocator>(dynamic_ref, 7, 8).verify();
   cat::raii::list swap_right =
      cat::raii::make_list<int, cat::dyn_allocator>(dynamic_ref, 9, 10, 11)
         .verify();
   cat::swap(swap_left, swap_right);
   cat::verify(swap_left.size() == 3);
   cat::verify(swap_left.front() == 9);
   cat::verify(swap_left.back() == 11);
   cat::verify(swap_right.size() == 2);
   cat::verify(swap_right.front() == 7);
   cat::verify(swap_right.back() == 8);

   cat::raii::list list_fill =
      cat::raii::make_list_filled<int, cat::dyn_allocator>(dynamic_ref, 4u, 1)
         .verify();
   cat::verify(list_fill.size() == 4);
   {
      auto it = list_fill.begin();
      for (idx i = 0; i < list_fill.size(); ++i) {
         cat::verify(*it == 1);
         ++it;
      }
   }

   cat::null_allocator null_alloc = cat::make_null_allocator();
   cat::raii::list null_list = cat::raii::make_list<int>(null_alloc).value();
   cat::verify(null_list.size() == 0);
   cat::verify(!null_list.insert(null_list.begin(), 1).has_value());
   cat::verify(!null_list.push_back(1).has_value());
   cat::verify(!null_list.push_front(1).has_value());
   cat::verify(!null_list.emplace(null_list.begin(), 1).has_value());
   cat::verify(!null_list.emplace_back(1).has_value());
   cat::verify(!null_list.emplace_front(1).has_value());

   cat::raii::forward_list forward_list_1 =
      cat::raii::make_forward_list<int4, cat::dyn_allocator>(dynamic_ref)
         .verify();
   cat::verify(forward_list_1.size() == 0);
   auto _ = forward_list_1.push_front(0).verify();
   auto _ = forward_list_1.emplace_front(1).verify();
   cat::verify(forward_list_1.size() == 2);

   cat::verify(*forward_list_1.begin() == 1);
   cat::verify(*(forward_list_1.begin() + 1u) == 0);

   cat::raii::forward_list forward_list_fill =
      cat::raii::make_forward_list_filled<int, cat::dyn_allocator>(
         dynamic_ref, 4u, 1
      )
         .verify();
   cat::verify(forward_list_fill.size() == 4);
   {
      auto it = forward_list_fill.begin();
      for (idx i = 0; i < forward_list_fill.size(); ++i) {
         cat::verify(*it == 1);
         ++it;
      }
   }

   cat::raii::forward_list forward_list_2 =
      forward_list_1.clone(allocator).verify();
   auto forward_it_1 = forward_list_1.begin();
   auto forward_it_2 = forward_list_2.begin();

   cat::verify(forward_list_1.size() == 2);
   cat::verify(forward_list_2.size() == 2);
   for (idx i = 0; i < forward_list_2.size(); ++i) {
      cat::verify(*forward_it_1 == *forward_it_2);
      ++forward_it_1;
      ++forward_it_2;
   }

   forward_list_1.pop_front();

   cat::raii::list back_list = cat::raii::make_list<int4>(allocator).verify();
   auto back_iterator = cat::as_back_inserter(back_list);
   auto front_iterator = cat::as_front_inserter(back_list);
   back_iterator.insert(10).verify();
   cat::verify(back_list.front() == 10);

   front_iterator.insert(2).verify();
   cat::verify(back_list.front() == 2);
   cat::verify(back_list.back() == 10);
}

$test(list_iterable) {
   using flux_test_list = cat::raii::list<int, cat::page_allocator>;
   using flux_test_forward_list =
      cat::raii::forward_list<int, cat::page_allocator>;

   static_assert(cat::is_iterable<flux_test_list>);
   static_assert(cat::is_iterable<flux_test_forward_list>);
   static_assert(!cat::is_collection<flux_test_list>);
   static_assert(!cat::is_collection<flux_test_forward_list>);
   static_assert(cat::is_reverse_iterable<flux_test_list>);
   static_assert(!cat::is_reverse_iterable<flux_test_forward_list>);

   cat::page_allocator allocator;
   cat::dyn_allocator dynamic_allocator = allocator;
   auto dynamic_ref = cat::allocator_ref<cat::dyn_allocator>(dynamic_allocator);
   auto list_values =
      cat::raii::make_list<int, cat::dyn_allocator>(dynamic_ref, 1, 2, 3, 4)
         .verify();
   cat::verify((list_values | cat::sum()) == 10);
   auto list_tail = cat::ref(list_values) | cat::reverse() | cat::take(2u);
   cat::verify(list_tail.sum() == 7);
   auto doubled_tail = cat::ref(list_values)
                          .filter([](int value) -> bool {
                             return value > 1;
                          })
                          .transform([](int value) -> int {
                             return value * 2;
                          });
   cat::verify(doubled_tail.sum() == 18);

   auto context = cat::iterate(list_values);
   int first_total = 0;
   auto first = context.run_while([&first_total](int value) -> bool {
      first_total += value;
      return first_total < 3;
   });
   cat::verify(first == cat::iteration_result::incomplete);
   cat::verify(first_total == 3);

   int rest_total = 0;
   auto rest = context.run_while([&rest_total](int value) -> bool {
      rest_total += value;
      return true;
   });
   cat::verify(rest == cat::iteration_result::complete);
   cat::verify(rest_total == 7);

   auto forward_list_values =
      cat::raii::make_forward_list<int, cat::dyn_allocator>(
         dynamic_ref, 4, 5, 6
      )
         .verify();
   cat::verify((forward_list_values | cat::sum()) == 15);
   auto shifted_edges = cat::ref(forward_list_values)
                           .filter([](int value) -> bool {
                              return value != 5;
                           })
                           .transform([](int value) -> int {
                              return value + 1;
                           });
   cat::verify(shifted_edges.sum() == 12);
}
