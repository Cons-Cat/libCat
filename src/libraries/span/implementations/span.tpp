// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/span>

namespace cat {

template <typename T, idx Extent>
template <is_random_access R>
   requires(is_same<remove_cv<typename R::value_type>, remove_cv<T>>
            && is_equality_comparable<remove_cv<T>>)
constexpr auto
span<T, Extent>::operator==(R const& rhs) const -> bool {
   // TODO: Replace this with a generic container equality algorithm.
   if (this->size() != rhs.size()) {
      return false;
   }

   for (idx i = 0u; i < this->size(); ++i) {
      if (!(this->data()[i] == rhs.data()[i])) {
         return false;
      }
   }
   return true;
}

template <typename T, idx Extent>
template <is_random_access R>
   requires(is_same<remove_cv<typename R::value_type>, remove_cv<T>>
            && requires(remove_cv<T> const& left, remove_cv<T> const& right) {
                  { left < right } -> is_convertible<bool>;
                  { right < left } -> is_convertible<bool>;
               })
constexpr auto
span<T, Extent>::operator<=>(R const& rhs) const {
   // TODO: Replace this with a generic container ordering algorithm.
   auto synth_three_way = []<typename U>(U const& left, U const& right) {
      if constexpr (is_threeway_comparable<U>) {
         return left <=> right;
      } else {
         if (left < right) {
            return std::weak_ordering::less;
         }
         if (right < left) {
            return std::weak_ordering::greater;
         }
         return std::weak_ordering::equivalent;
      }
   };

   using element_type = remove_cv<T>;
   using category_type = decltype(synth_three_way(
      declval<element_type const&>(), declval<element_type const&>()));

   idx i = 0u;
   idx const lhs_size = this->size();
   idx const rhs_size = rhs.size();
   idx const shared_size = lhs_size < rhs_size ? lhs_size : rhs_size;

   while (i < shared_size) {
      auto comparison = synth_three_way(this->data()[i.raw], rhs.data()[i.raw]);
      if (comparison != 0) {
         return comparison;
      }
      ++i;
   }

   if (lhs_size < rhs_size) {
      return category_type(std::strong_ordering::less);
   }
   if (rhs_size < lhs_size) {
      return category_type(std::strong_ordering::greater);
   }
   return category_type(std::strong_ordering::equal);
}

}  // namespace cat
