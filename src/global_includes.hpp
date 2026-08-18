// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/meta>

// This file should be implicitly included in all other files. With Clang, this
// is done using the `--include` flag, as in `--include global_includes.hpp`.
// The `CMakeLists.txt` in this repository's top level directory does this.

// Clang reports ordinary string-literal encoding through
// `__clang_literal_encoding__`. libCat requires UTF-8 (`-fexec-charset=UTF-8`).
#ifndef __clang_literal_encoding__
#error "libCat requires Clang's `__clang_literal_encoding__` predefined macro!"
#endif
#ifndef __clang_wide_literal_encoding__
#error \
   "libCat requires Clang's `__clang_wide_literal_encoding__` predefined macro!"
#endif
namespace cat::detail {
consteval auto
literal_encoding_is_utf8() -> bool {
   return __builtin_strcmp(__clang_literal_encoding__, "UTF-8") == 0;
}
}  // namespace cat::detail

static_assert(
   cat::detail::literal_encoding_is_utf8(),
   "libCat requires `-fexec-charset=UTF-8` so ordinary `char` string literals "
   "are UTF-8!"
);

namespace cat::detail {
template <typename F>
class deferrer_callback {
 private:
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
   operator<<(F&& callback) const -> deferrer_callback<F> {
      return deferrer_callback<F>(callback);
   }
} const deferrer [[maybe_unused]];
}  // namespace cat::detail

// `$defer` is a macro that evaluates its body at the end of its scope
// to perform resource cleanup.
// For example:
//    void* p_mem1 = allocator.alloc();
//    void* p_mem2 = allocator.alloc();
//    $defer {
//        allocator.free(p_mem1);
//        allocator.free(p_mem2);
//    };
#define CAT_DEFER auto _ = ::cat::detail::deferrer << [&]->void

// `CAT_DEFER` should never be `#undef`'d. The redefinable macro `$defer` exists
// to make this macro more ergonomic.
#pragma clang final(CAT_DEFER)

#define $defer CAT_DEFER

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

template <typename T, typename CharT>
struct formatter;

// Implementing this here is a circular dependency. The implementation can be
// found in <cat/format/implementations/format_monostate.tpp>.
template <typename CharT>
struct formatter<monostate_type, CharT>;

template <typename T, T constant_state>
struct monotype_storage {
   constexpr monotype_storage() = default;

   constexpr monotype_storage(monostate_type& /*unused*/)
       : m_storage(constant_state) {
   }

   constexpr monotype_storage(monostate_type const& /*unused*/)
       : m_storage(constant_state) {
   }

   constexpr monotype_storage(T input) : m_storage(input) {
   }

   constexpr
   operator auto() const {
      return this->m_storage;
   };

   constexpr auto
   operator=(monostate_type /*unused*/)
      -> monotype_storage<T, constant_state>& {
      return *this;
   }

   friend constexpr auto
   operator<=>(
      monotype_storage<T, constant_state> const& self, auto const& rhs
   ) {
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

template <typename T>
class maybe;

// Customization point for default niche-value optimization in `maybe<T>`.
template <typename T>
struct default_compact_trait;

template <typename T, auto predicate, auto>
   requires(is_predicate<typeof_unqual(predicate), T const&>)
struct compact;

template <typename T, auto predicate, is_invocable auto get_nullopt>
   requires(
      is_predicate<typeof_unqual(predicate), T const&>
      && !predicate(get_nullopt())
   )
struct compact<T, predicate, get_nullopt> {
   // The engaged value type. `maybe_compact_storage<compact<...>>` exposes
   // this as `value_type` so the surrounding `maybe<T>` machinery can
   // talk about the engaged type independently from `nullopt_state`'s
   // potentially different return type.
   using value_type = T;

   static constexpr auto
   has_value(T const& value) -> bool {
      return predicate(value);
   }

   // `nullopt_state` is permitted to return a type distinct from `T` as
   // long as that type is convertible to `T` (the predicate takes
   // `T const&`, so the `requires`-clause above will already reject any
   // niche value that can't be stored in `T`). Returning a different
   // type is useful when the niche bit pattern wouldn't be a meaningful
   // value of `T`.
   // `maybe::value_or_niche` then exposes the result as
   // `common_type<value_type, decltype(nullopt_state())>`.
   static constexpr auto
   nullopt_state() {
      return get_nullopt();
   }

   consteval compact() = default;
};

template <typename T, auto predicate, auto nullopt_value>
   requires(
      is_predicate<typeof_unqual(predicate), T const&>
      && !predicate(T{nullopt_value})
   )
struct compact<T, predicate, nullopt_value> : compact<
                                                 T, predicate,
                                                 // Sentinel accessor:
                                                 [] constexpr -> T {
                                                    return T{nullopt_value};
                                                 }> {};

template <typename T, auto predicate>
struct compact_scaredy {
   using type = T;
   static constexpr auto predicate_function = predicate;
   // `compact_scaredy`s can only be instantiated at compile-time.
   consteval compact_scaredy() = default;
};

namespace detail {

template <typename T, T constant_state>
consteval auto
is_monostate_storage_impl(monotype_storage<T, constant_state>) -> bool {
   return true;
}

consteval auto
is_monostate_storage_impl(auto) -> bool {
   return false;
}

template <typename T>
inline constexpr bool is_monostate_storage = is_monostate_storage_impl(T());

// This is a function instead of a lambda to fix clangd crashes.
template <typename T, T in_sentinel>
constexpr auto
sentinel_predicate(T value) -> bool {
   return value != in_sentinel;
}

template <typename T, T (*_Nonnull get_nullopt)()>
constexpr auto
sentinel_predicate_for_callable(T value) -> bool {
   return value != get_nullopt();
}
}  // namespace detail

template <is_structural T, auto value>
using sentinel = compact<
   T, detail::sentinel_predicate<T, static_cast<T>(value)>,
   static_cast<T>(value)>;

template <typename T, T (*_Nonnull get_nullopt)()>
using sentinel_fn = compact<
   T, &detail::sentinel_predicate_for_callable<T, get_nullopt>, get_nullopt>;

// `in_place` is consumed by wrapper classes to default-initialize their
// storage.
inline constexpr in_place_type in_place;

// `monostate` can be consumed by wrapper classes to represent no storage.
inline constexpr cat::monostate_type monostate;

template <typename T>
inline constexpr bool is_monostate =
   is_same<T, monostate_type> || detail::is_monostate_storage<T>;

}  // namespace cat

// NOLINTBEGIN(bugprone-std-namespace-modification)
namespace std {

template <typename>
struct tuple_size;

template <typename T>
inline constexpr __SIZE_TYPE__ tuple_size_v = tuple_size<T>::value;

template <__SIZE_TYPE__, typename>
struct tuple_element;

template <__SIZE_TYPE__ index, typename T>
using tuple_element_t = tuple_element<index, T>::type;

}  // namespace std

// NOLINTEND(bugprone-std-namespace-modification)

// Including the `<cat/runtime>` library is required to link a libCat program,
// because it contains the `_start` symbol.
#include <cat/maybe>
#include <cat/runtime>

#include "implementations/simd_mask_bitset.tpp"

// `assert()` is used throughout the library.
#include <cat/debug>

// `cat::propagate_error` is a configuration point for the `$prop` macros. It
// uses overload resolution to deduce the value that should be returned within a
// function expanding these macros. In libCat, overloads are implemented for
// `cat::maybe` and `cat::scaredy`.

// Unwrap an error-like container such as `cat::scaredy` or `cat::maybe` iff
// it holds a value, otherwise propagate it up the call stack, using a statement
// expression.
#define CAT_PROPAGATE(container)                                       \
   ({                                                                  \
      auto libcat_temp_expr = (container);                             \
      if (libcat_temp_expr.is_empty()) {                               \
         return ::cat::propagate_error(::cat::move(libcat_temp_expr)); \
      }                                                                \
      ::cat::move(libcat_temp_expr).value();                           \
   })

// `CAT_PROPAGATE` should never be `#undef`'d. The redefinable macro `$prop`
// exists to make this macro more ergonomic.
#pragma clang final(CAT_PROPAGATE)

#define $prop CAT_PROPAGATE

// Propagate error-like container such as `cat::scaredy` or `cat::maybe` iff
// it holds an error, otherwise evaluate to a non-error state `or_value`.
#define CAT_PROPAGATE_OR(container, or_value)                          \
   ({                                                                  \
      auto libcat_temp_expr = (container);                             \
      if (libcat_temp_expr.is_empty()) {                               \
         return ::cat::propagate_error(::cat::move(libcat_temp_expr)); \
      }                                                                \
      (or_value);                                                      \
   })

// `CAT_PROPAGATE_OR` should never be `#undef`'d. The redefinable macro
// `$prop_or` exists to make this macro more ergonomic.
#pragma clang final(CAT_PROPAGATE_OR)

#define $prop_or CAT_PROPAGATE_OR

// Unwrap an error-like container such as `cat::scaredy` or `cat::maybe` iff
// it holds a value, otherwise propagate an error-state `error_value`.
#define CAT_PROPAGATE_AS(container, or_value) \
   ({                                         \
      auto libcat_temp_expr = (container);    \
      if (libcat_temp_expr.is_empty()) {      \
         return (or_value);                   \
      }                                       \
      ::cat::move(libcat_temp_expr).value();  \
   })

// `CAT_PROPAGATE_AS` should never be `#undef`'d. The redefinable macro
// `$prop_as` exists to make this macro more ergonomic.
#pragma clang final(CAT_PROPAGATE_AS)

#define $prop_as CAT_PROPAGATE_AS
