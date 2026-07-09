// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/iterable>
#include <cat/simd_dispatch>
#include <cat/simd_ops>
#include <cat/utility>

namespace cat {
namespace detail {
template <typename T>
concept has_void_reverse_inplace_member =
   requires {
      static_cast<void (remove_cvref<T>::*)()>(
         &remove_cvref<T>::reverse_inplace
      );
   };

template <typename T>
concept has_const_self_reverse_inplace_member =
   requires {
      static_cast<remove_cvref<T> (remove_cvref<T>::*)() const>(
         &remove_cvref<T>::reverse_inplace
      );
   };

template <typename Element>
constexpr void
reverse_inplace_scalar_impl(Element* _Nonnull p_data, idx size) {
   idx left = 0u;
   iword right = size;

   while (left < right) {
      --right;
      if (left == right) {
         break;
      }
      // `right` must be >= 0 here:
      cat::swap(p_data[left], p_data[idx(right)]);
      ++left;
   }
}

template <typename Vector, typename Element>
constexpr void
reverse_inplace_simd_impl(Element* _Nonnull p_data, idx size) {
   constexpr idx lanes = Vector::abi_type::lanes;
   using memory_lane = Vector::memory_lane;
   memory_lane* _Nonnull p_lanes =
      __builtin_bit_cast(memory_lane*, p_data);
   idx left = 0u;
   iword right = size;

   while (left + (lanes * 2u) <= right) {
      right -= lanes;
      Vector left_values;
      Vector right_values;
      left_values.load_unaligned(p_lanes + left);
      right_values.load_unaligned(p_lanes + right);
      simd_reverse(right_values).store_unaligned(p_lanes + left);
      simd_reverse(left_values).store_unaligned(p_lanes + right);
      left += lanes;
   }

   // `right` must be >= 0 here:
   reverse_inplace_scalar_impl(p_data + left, idx(right - left));
}

struct reverse_inplace_impl {
   template <is_iterable Iterable>
      requires(!is_const<Iterable>)
   friend constexpr auto
   operator|(Iterable&& incoming, reverse_inplace_impl /*reverse_inplace_impl*/)
      -> decltype(auto) {
      auto&& range = unwrap_ref(incoming);
      if constexpr (has_void_reverse_inplace_member<decltype(range)>) {
         range.reverse_inplace();
      } else if constexpr (
         has_const_self_reverse_inplace_member<decltype(range)>
      ) {
         static_cast<void>(range.reverse_inplace());
      } else {
         if constexpr (
            is_random_access_collection<decltype(range)>
            && requires {
                  range.data();
                  range.size();
               }
         ) {
            using value_type = remove_reference<decltype(*range.data())>;
            if constexpr (is_arithmetic<value_type>) {
               if consteval {
                  reverse_inplace_scalar_impl(range.data(), range.size());
               } else {
                  if (range.size() * sizeof(value_type) < 64u) {
                     reverse_inplace_scalar_impl(range.data(), range.size());
                  } else {
                     $simd_switch(
                        $abi(
                           (sse2, avx512),
                           {
                              reverse_inplace_simd_impl<native_simd<value_type>>(
                                 range.data(), range.size()
                              );
                           }
                        ),
                        $abi(
                           avx2,
                           {
                              reverse_inplace_simd_impl<native_simd<value_type>>(
                                 range.data(), range.size()
                              );
                              x64::zero_upper_avx_registers();
                           }
                        )
                     );
                  }
               }
            } else {
               reverse_inplace_scalar_impl(range.data(), range.size());
            }
         } else {
            auto first = range.begin();
            auto last = range.end();
            while (first != last) {
               --last;
               if (first == last) {
                  break;
               }
               cat::swap(*first, *last);
               ++first;
            }
         }
      }
      return $fwd(incoming);
   }
};
}  // namespace detail

// Reverse the incoming container in-place. This is a terminal algorithm.
constexpr auto
reverse_inplace() -> detail::reverse_inplace_impl {
   return {};
}

}  // namespace cat
