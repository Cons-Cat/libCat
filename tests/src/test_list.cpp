#include <cat/insert_iterators>
#include <cat/linear_allocator>
#include <cat/list>
#include <cat/page_allocator>

#include "../unit_tests.hpp"

test(list) {
   // Initialize an allocator.
   cat::page_allocator pager;
   cat::span page = pager.alloc_multi<cat::byte>(4_uki).or_exit();
   defer {
      pager.free(page);
   };
   auto allocator = cat::make_linear_allocator(page);

   // Test insert.
   cat::list list_1 = cat::make_list<int4>(allocator).verify();
   auto _ = list_1.insert(list_1.begin(), 3).verify();
   auto _ = list_1.insert(list_1.begin(), 2).verify();
   auto _ = list_1.insert(list_1.begin(), 1).verify();
   cat::verify(list_1.front() == 1);
   cat::verify(list_1.back() == 3);

   // Test iteration.
   int i = 1;
   for (auto& node : list_1) {
      cat::verify(node == i);
      ++i;
   }

   list_1.pop_front();
   cat::verify(list_1.front() == 2);
   list_1.pop_back();
   cat::verify(list_1.back() == 2);

   // Test push.
   cat::list list_2 = cat::make_list<int4>(allocator).verify();
   auto _ = list_2.push_front(0).verify();
   auto _ = list_2.push_back(4).verify();
   cat::verify(list_2.front() == 0);
   cat::verify(list_2.back() == 4);
   auto _ = list_2.insert(++list_2.begin(), 1).verify();
   cat::verify(list_2.front() == 0);
   cat::verify(*++list_2.begin() == 1);

   // Test the `list` iterator after `.push_back()`.
   for (auto&& node : list_2) {
      auto _ = node;
      asm volatile("nop");  // Don't optimize out this loop.
   }

   // Test emplace.
   cat::list<int4, cat::linear_allocator> list_3 =
      cat::make_list<int4>(allocator).verify();
   auto _ = list_3.emplace_front(1).verify();
   auto _ = list_3.emplace_front(2).verify();
   auto _ = list_3.emplace_back(3).verify();
   auto _ = list_3.emplace(++list_3.begin(), 4);
   cat::verify(list_3.front() == 2);
   cat::verify(list_3.back() == 3);
   cat::verify((*(++list_3.begin())) == 4);

   // Test the `list` iterator after `emplace_back()`.
   for (auto&& node : list_3) {
      auto _ = node;
      asm volatile("nop");  // Don't optimize out this loop.
   }

   // Test special iterators.
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

   // Test freeing nodes.
   list_1.erase(list_1.begin());
   for (int i = 0; i < 10; ++i) {
      list_1.pop_front();
   }

   list_1.clear();
   list_2.clear();
   cat::verify(list_2.size() == 0);

   // Deep copy a `list`.
   auto _ = list_1.push_front(3).verify();
   auto _ = list_1.push_front(2).verify();
   auto _ = list_1.push_front(1).verify();
   auto _ = list_1.push_front(0).verify();
   cat::list list_5 = list_1.clone(allocator).verify();

   auto list_it_1 = list_1.begin();
   auto list_it_5 = list_5.begin();

   // Prove all elements copied correctly.
   while (list_it_1 != list_1.end()) {
      cat::verify(*list_it_1 == *list_it_5);
      ++list_it_1;
      ++list_it_5;
   }

   // Test that the copy was deep.
   list_1.clear();
   cat::verify(*(list_5.begin()) == 0);
   cat::verify(*(list_5.begin() + 1u) == 1);
   cat::verify(*(list_5.begin() + 2u) == 2);
   cat::verify(*(list_5.begin() + 3u) == 3);

   // Test moving `list`.
   list_1.push_front(2).verify();
   list_1.push_front(1).verify();
   list_1.push_front(0).verify();
   cat::list list_4 = mov list_1;
   cat::verify(list_4.size() == 3);
   cat::verify(list_4.front() == 0);
   cat::verify(*(list_4.begin() + 1u) == 1);
   cat::verify(*(list_4.begin() + 2u) == 2);

   // Test initialized `list`.
   cat::list list_init_1 = cat::make_list(allocator, 1).verify();
   cat::verify(list_init_1.size() == 1);
   auto list_it = list_init_1.begin();
   cat::verify(*list_it == 1);

   cat::list list_init_2 = cat::make_list(allocator, 1, 2, 3).verify();
   cat::verify(list_init_2.size() == 3);
   list_it = list_init_2.begin();
   cat::verify(*list_it == 1);
   cat::verify(*++list_it == 2);
   cat::verify(*++list_it == 3);

   // Test filling out a `list`.
   cat::list list_fill = cat::make_list_filled(allocator, 4u, 1).verify();
   cat::verify(list_fill.size() == 4);
   {
      auto it = list_fill.begin();
      for (idx i = 0; i < list_fill.size(); ++i) {
         cat::verify(*it == 1);
         ++it;
      }
   }

   // Test propagating allocation failure.
   cat::null_allocator null_alloc = cat::make_null_allocator();
   cat::list null_list = cat::make_list<int>(null_alloc).value();
   cat::verify(null_list.size() == 0);
   cat::verify(!null_list.insert(null_list.begin(), 1).has_value());
   cat::verify(!null_list.push_back(1).has_value());
   cat::verify(!null_list.push_front(1).has_value());
   cat::verify(!null_list.emplace(null_list.begin(), 1).has_value());
   cat::verify(!null_list.emplace_back(1).has_value());
   cat::verify(!null_list.emplace_front(1).has_value());

   // Test `slist`.
   cat::slist slist_1 = cat::make_slist<int4>(allocator).verify();
   cat::verify(slist_1.size() == 0);
   auto _ = slist_1.push_front(0).verify();
   auto _ = slist_1.emplace_front(1).verify();
   // auto _ = slist_1.insert_after(slist_1.begin() + 1, 2).verify();
   // auto _ = slist_1.emplace_after(slist_1.end(), 3).verify();
   cat::verify(slist_1.size() == 2);

   cat::verify(*slist_1.begin() == 1);
   cat::verify(*(slist_1.begin() + 1u) == 0);
   // cat::verify(*(slist_1.begin() + 2) == 2);
   // cat::verify(*(slist_1.begin() + 3) == 3);

   // Test filling out an `slist`.
   cat::slist slist_fill = cat::make_slist_filled(allocator, 4u, 1).verify();
   cat::verify(slist_fill.size() == 4);
   {
      auto it = slist_fill.begin();
      for (idx i = 0; i < slist_fill.size(); ++i) {
         cat::verify(*it == 1);
         ++it;
      }
   }

   // Deep copy a `slist`.
   cat::slist slist_2 = slist_1.clone(allocator).verify();
   auto forward_it_1 = slist_1.begin();
   auto forward_it_2 = slist_2.begin();

   // Prove all elements copied correctly.
   cat::verify(slist_1.size() == 2);
   cat::verify(slist_2.size() == 2);
   for (idx i = 0; i < slist_2.size(); ++i) {
      cat::verify(*forward_it_1 == *forward_it_2);
      ++forward_it_1;
      ++forward_it_2;
   }

   // Remove elements from `slist`.
   // slist_1.erase_after(slist_1.begin());
   // cat::verify(*(slist_1.begin() + 1) == 2);

   slist_1.pop_front();
   // cat::verify(*slist_1.begin() == 2);

   // Test that the copy was deep, after modifying the original.
   // cat::verify(*(slist_2.begin()) == 1);
   // cat::verify(*(slist_2.begin() + 1) == 0);
   // cat::verify(*(slist_2.begin() + 2) == 2);
   // cat::verify(*(slist_2.begin() + 3) == 3);

   // Test `back_insert_iterator`.
   cat::list back_list = cat::make_list<int4>(allocator).verify();
   cat::back_insert_iterator back_iterator(back_list);
   cat::front_insert_iterator front_iterator(back_list);
   back_iterator.insert(10).verify();
   cat::verify(back_list.front() == 10);

   front_iterator.insert(2).verify();
   cat::verify(back_list.front() == 2);
   cat::verify(back_list.back() == 10);
}
