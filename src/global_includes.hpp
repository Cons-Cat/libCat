// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

// This file should be implicitly included in all other files. With GCC, this is
// done using the `--include` flag, as in `--include global_includes.hpp`. The
// `CMakeLists.txt` in this repository's top level directory does this.

#include <cat/compare>

// libCat provides a `_` variable that consumes a function's output, but
// cannot be assigned to any variable.
namespace cat {
namespace detail {
    struct [[maybe_unused]] unused_type {
        template <typename T>
        constexpr void operator=(T const&) {
        }

        // `unused_type` cannot be assigned to any variable.
        operator auto() = delete;
        // `unused_type` cannot be assigned to itself, i.e. `_ = _`.
        auto operator=(unused_type) = delete;
    };
}  // namespace detail

// `in_place_type` is consumed by wrapper classes to default-initialize their
// storage.
struct in_place_type {};

// A `monostate_type` is an object that can hold anything, and convert into
// anything or from anything. It has no storage or behavior.
struct monostate_type {
    constexpr monostate_type() = default;

    // constexpr monostate_type(auto){}
    constexpr operator auto() {
        // Converting `monostate_type` into another type is no-op.
    }
};

template <typename T, T constant_state>
struct monotype_storage {
    constexpr monotype_storage() = default;

    constexpr monotype_storage(monostate_type&) : storage(constant_state) {
    }

    constexpr monotype_storage(monostate_type const&)
        : storage(constant_state) {
    }

    constexpr monotype_storage(T input) : storage(input) {
    }

    constexpr operator auto() const {
        return this->storage;
    };

    constexpr auto operator=(monostate_type)
        -> monotype_storage<T, constant_state>& {
        return *this;
    }

    constexpr friend auto operator<=>(
        monotype_storage<T, constant_state> const& self, auto const& rhs) {
        return self.storage <=> rhs;
    }

    constexpr friend auto operator==(
        monotype_storage<T, constant_state> const& self, auto const& rhs)
        -> bool {
        return self.storage == rhs;
    }

    T storage;
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
    constexpr auto sentinel_predicate(T value) -> bool {
        return value != in_sentinel;
    }
}  // namespace detail

template <typename T, T in_sentinel>
using sentinel =
    compact<T, detail::sentinel_predicate<T, in_sentinel>, in_sentinel>;

}  // namespace cat

// `_` can consume any value to explicitly disregard a `[[nodiscard]]`
// attribute from a function with side effects.
[[maybe_unused]] inline cat::detail::unused_type _;

// `in_place` is consumed by wrapper classes to default-initialize their
// storage.
inline constexpr cat::in_place_type in_place;

// `monostate` can be consumed by wrapper classes to represent no storage.
inline constexpr cat::monostate_type monostate;

// Including the `<cat/runtime>` library is required to link a libCat program,
// because it contains the `_start` symbol.
#include <cat/runtime>

// `assert()` is used throughout the library.
#include <cat/debug>

// `no_type` is required for the `TRY()` macro.
#include <cat/notype>

// Unwrap an error-like container such as `cat::scaredy` or `cat::maybe` iff
// it holds a value, otherwise propagate it up the call stack. This works due to
// a GCC extension, statement expressions.
#define TRY(container)                                                          \
    ({                                                                          \
        using try_type = decltype(container);                                   \
        /* static_assert(cat::is_maybe<try_type>||cat::is_scaredy<try_type>);*/ \
        /* `if constexpr` does not short circuit for the purposes of */         \
        /* type-deduction within a statement expression. */                     \
        /* If a `nullopt` is returned, this can fail to compile even if */      \
        /* the constant expression is false. In that case, `no_type` is */      \
        /* the return type instead, but of course `no_type` will never be */    \
        /* returned.*/                                                          \
        using return_type =                                                     \
            cat::conditional<cat::is_specialization<try_type, cat::maybe>,      \
                             cat::detail::nullopt_type, cat::no_type>;          \
                                                                                \
        if (!((container).has_value())) {                                       \
            if constexpr (cat::is_maybe<try_type>) {                            \
                return return_type();                                           \
            } else {                                                            \
                return (container);                                             \
            }                                                                   \
        }                                                                       \
        (container).value();                                                    \
    })

// Placement `new`.
[[nodiscard]]
inline constexpr auto
operator new(unsigned long, void* p_address) -> void* {
    return p_address;
}

// `new[]` and `delete[]` are defined for use in a `constexpr` context.
[[nodiscard]]
inline auto
operator new[](unsigned long, void* p_address) -> void* {
    return p_address;
}

// NOLINTNEXTLINE Let this be `inline`.
[[nodiscard]]
inline auto
operator new[](unsigned long) -> void* {
    return reinterpret_cast<void*>(1ul);
}

namespace std {
enum class align_val_t : __SIZE_TYPE__ {
};
}  // namespace std

// NOLINTNEXTLINE Let this be `inline`.
inline void operator delete[](void*) {
}

inline void operator delete[](void*, unsigned long) {
}

inline void operator delete[](void*, unsigned long, std::align_val_t) {
}

// NOLINTNEXTLINE Let this be `inline`.
[[nodiscard]]
inline auto
operator new[](unsigned long, std::align_val_t align) -> void* {
    return reinterpret_cast<void*>(align);
}
