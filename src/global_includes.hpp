// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

// This file should be implicitly included in all other files. With GCC, this is
// done using the `--include` flag, as in `--include global_includes.hpp`. The
// `CMakeLists.txt` in this repository's top level directory does this.

#include <cat/compare>

namespace cat::detail {
template <typename F>
class deferrer_callback {
   F m_callback;

 public:
   template <typename T>
   deferrer_callback(T&& f) : m_callback(f) {  // NOLINT
   }

   ~deferrer_callback() {
      m_callback();
   }
};

inline constinit struct {
   template <typename F>
   auto
   operator<<(F&& callback) -> deferrer_callback<F> {
      return deferrer_callback<F>(callback);
   }
} deferrer [[maybe_unused]];
}  // namespace cat::detail

// `defer` is a macro that instantiates a scoped object which executes some
// arbitrary closure in its destructor.
// For example:
//     void* p_mem1 = allocator.alloc();
//     void* p_mem2 = allocator.alloc();
//     defer {
//         allocator.free(p_mem1);
//         allocator.free(p_mem2);
//     };
#define CAT_DEFER auto _ = ::cat::detail::deferrer << [&]

// `CAT_DEFER` should never be `#undef`'d. The redefinable macro `defer` exists
// to make this macro more ergonomic.
#pragma clang final(CAT_DEFER)

#define defer CAT_DEFER

namespace cat {

// `in_place_type` is consumed by wrapper classes to default-initialize their
// storage.
struct in_place_type {};

// A `monostate_type` is an object that can hold anything, and convert into
// anything or from anything. It has no storage or behavior.
struct monostate_type {
   constexpr monostate_type() = default;

   // constexpr monostate_type(auto){}
   constexpr
   operator auto() {
      // Converting `monostate_type` into another type is no-op.
   }
};

template <typename T, T constant_state>
struct monotype_storage {
   constexpr monotype_storage() = default;

   constexpr monotype_storage(monostate_type&) : m_storage(constant_state) {
   }

   constexpr monotype_storage(monostate_type const&)
       : m_storage(constant_state) {
   }

   constexpr monotype_storage(T input) : m_storage(input) {
   }

   constexpr
   operator auto() const {
      return this->m_storage;
   };

   constexpr auto
   operator=(monostate_type) -> monotype_storage<T, constant_state>& {
      return *this;
   }

   friend constexpr auto
   operator<=>(monotype_storage<T, constant_state> const& self,
               auto const& rhs) {
      return self.m_storage <=> rhs;
   }

   friend constexpr auto
   operator==(monotype_storage<T, constant_state> const& self, auto const& rhs)
      -> bool {
      return self.m_storage == rhs;
   }

   [[no_unique_address]]
   T m_storage;
};

// `in_sentinel` should be either a `T` or an error type for `scaredy`s.
template <typename T, auto predicate, auto in_sentinel>
   requires(!predicate(static_cast<T>(in_sentinel)))
struct compact {
   using type = T;
   static constexpr auto predicate_function = predicate;
   static constexpr T sentinel_value = in_sentinel;
   // `compact`s can only be instantiated at compile-time.
   consteval compact() = default;
};

template <typename T, auto predicate>
struct compact_scaredy {
   using type = T;
   static constexpr auto predicate_function = predicate;
   // `compact_scaredy`s can only be instantiated at compile-time.
   consteval compact_scaredy() = default;
};

namespace detail {
// This is a function instead of a lambda to fix clangd crashes.
template <typename T, T in_sentinel>
constexpr auto
sentinel_predicate(T value) -> bool {
   return value != in_sentinel;
}
}  // namespace detail

template <typename T, T in_sentinel>
using sentinel =
   compact<T, detail::sentinel_predicate<T, in_sentinel>, in_sentinel>;

// `in_place` is consumed by wrapper classes to default-initialize their
// storage.
inline constexpr in_place_type in_place;

// `monostate` can be consumed by wrapper classes to represent no storage.
inline constexpr cat::monostate_type monostate;

}  // namespace cat

// Including the `<cat/runtime>` library is required to link a libCat program,
// because it contains the `_start` symbol.
#include <cat/runtime>

// `assert()` is used throughout the library.
#include <cat/debug>

// `no_type` is required for the `prop()` macro.
#include <cat/notype>

// Unwrap an error-like container such as `cat::scaredy` or `cat::maybe` iff
// it holds a value, otherwise propagate it up the call stack, using a statement
// expression.
#define CAT_PROPAGATE(container)                                              \
   ({                                                                         \
      using try_type = decltype(container);                                   \
      /* static_assert(cat::is_maybe<try_type>||cat::is_scaredy<try_type>);*/ \
      /* `if constexpr` does not short circuit for the purposes of */         \
      /* type-deduction within a statement expression. */                     \
      /* If a `nullopt` is returned, this can fail to compile even if */      \
      /* the constant expression is false. In that case, `no_type` is */      \
      /* the return type instead, but of course `no_type` will never be */    \
      /* returned.*/                                                          \
      using return_type =                                                     \
         ::cat::conditional<::cat::is_specialization<try_type, ::cat::maybe>, \
                            ::cat::detail::nullopt_type, ::cat::no_type>;     \
                                                                              \
      if (!((container).has_value())) {                                       \
         if constexpr (::cat::is_maybe<try_type>) {                           \
            return return_type();                                             \
         } else {                                                             \
            return (container);                                               \
         }                                                                    \
      }                                                                       \
      (container).value();                                                    \
   })

// `CAT_PROPAGATE` should never be `#undef`'d. The redefinable macro `prop`
// exists to make this macro more ergonomic.
#pragma clang final(CAT_PROPAGATE)

#define prop(container) CAT_PROPAGATE(container)

// Unwrap an error-like container such as `cat::scaredy` or `cat::maybe` iff
// it holds a value, otherwise propagate an error state `or_value`.
#define CAT_PROPAGATE_OR(container, or_value)                                 \
   ({                                                                         \
      using try_type = decltype(container);                                   \
      /* static_assert(cat::is_maybe<try_type>||cat::is_scaredy<try_type>);*/ \
      /* `if constexpr` does not short circuit for the purposes of */         \
      /* type-deduction within a statement expression. */                     \
      /* If a `nullopt` is returned, this can fail to compile even if */      \
      /* the constant expression is false. In that case, `no_type` is */      \
      /* the return type instead, but of course `no_type` will never be */    \
      /* returned.*/                                                          \
      using return_type =                                                     \
         ::cat::conditional<::cat::is_specialization<try_type, ::cat::maybe>, \
                            ::cat::detail::nullopt_type, ::cat::no_type>;     \
                                                                              \
      if (!((container).has_value())) {                                       \
         if constexpr (::cat::is_maybe<try_type>) {                           \
            return return_type();                                             \
         } else {                                                             \
            return (container).error();                                       \
         }                                                                    \
      }                                                                       \
      (or_value);                                                             \
   })

// `CAT_PROPAGATE_OR` should never be `#undef`'d. The redefinable macro
// `prop_or` exists to make this macro more ergonomic.
#pragma clang final(CAT_PROPAGATE_OR)

#define prop_or(container, or_value) CAT_PROPAGATE_OR(container, or_value)

namespace std {

template <typename>
struct tuple_size;

template <typename T>
inline constexpr __SIZE_TYPE__ tuple_size_v = tuple_size<T>::value;

template <__SIZE_TYPE__, typename>
struct tuple_element;

template <__SIZE_TYPE__ in_index, typename T>
using tuple_element_t = tuple_element<in_index, T>::type;

// TODO: Does this actually have to be in `std::`?
enum class align_val_t : __SIZE_TYPE__ {
};
}  // namespace std

auto
operator new(unsigned long, void* p_address) -> void*;

// `new[]` and `delete[]` are defined for use in a `constexpr` context.
[[nodiscard]]
auto
operator new[](unsigned long, void* p_address) -> void*;

[[nodiscard]]
auto
operator new[](unsigned long) -> void*;

void
operator delete[](void*);

void
operator delete[](void*, unsigned long);

void
operator delete[](void*, unsigned long, std::align_val_t);

[[nodiscard]]
auto
operator new[](unsigned long, std::align_val_t align) -> void*;
