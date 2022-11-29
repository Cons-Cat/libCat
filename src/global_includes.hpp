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
    struct [[maybe_unused]] Unused {
        template <typename T>
        constexpr void operator=(T const&){};
        // `Unused` cannot be assigned to any variable.
        operator auto() = delete;
        // `Unused` cannot be assigned to itself, i.e. `_ = _`.
        auto operator=(Unused) = delete;
    };

    // `InPlace` is consumed by wrapper classes to default-initialize their
    // storage.
    struct InPlace {};
}  // namespace detail

// A `Monostate` is an object that can hold anything, and convert into
// anything or from anything. It has no storage or behavior.
struct Monostate {
    constexpr Monostate() = default;
    // constexpr Monostate(auto){};
    constexpr operator auto(){};
};

template <typename T, T state>
struct MonostateStorage {
    constexpr MonostateStorage() = default;
    constexpr MonostateStorage(Monostate&) : storage(state){};
    constexpr MonostateStorage(Monostate const&) : storage(state){};
    constexpr MonostateStorage(T input) : storage(input){};

    constexpr operator auto() const {
        return this->storage;
    };

    constexpr auto operator=(Monostate) -> MonostateStorage<T, state>& {
        return *this;
    }

    constexpr friend auto operator<=>(MonostateStorage<T, state> const& self,
                                      auto const& rhs) {
        return self.storage <=> rhs;
    }

    constexpr friend auto operator==(MonostateStorage<T, state> const& self,
                                     auto const& rhs) -> bool {
        return self.storage == rhs;
    }

    T storage;
};

// `sentinel` should be either a `T` or an error type for `Scaredy`s.
template <typename T, auto predicate, auto sentinel>
    requires(!predicate(static_cast<T>(sentinel)))
struct Compact {
    using Type = T;
    static constexpr auto predicate_function = predicate;
    static constexpr T sentinel_value = sentinel;
    // `Compact`s can only be instantiated at compile-time.
    consteval Compact() = default;
};

template <typename T, auto predicate>
struct CompactScaredy {
    using Type = T;
    static constexpr auto predicate_function = predicate;
    // `CompactScaredy`s can only be instantiated at compile-time.
    consteval CompactScaredy() = default;
};

namespace detail {
    // This is a function instead of a lambda to fix clangd crashes.
    template <typename T, T sentinel>
    constexpr auto sentinel_predicate(T value) -> bool {
        return value != sentinel;
    }
}  // namespace detail

template <typename T, T sentinel>
using Sentinel = Compact<T, detail::sentinel_predicate<T, sentinel>, sentinel>;

}  // namespace cat

// `_` can consume any value to explicitly disregard a `[[nodiscard]]`
// attribute from a function with side effects. Consuming a `Result` value is
// not possible.
[[maybe_unused]] inline cat::detail::Unused _;

// `in_place` is consumed by wrapper classes to default-initialize their
// storage.
inline constexpr cat::detail::InPlace in_place;

// `monostate` can be consumed by wrapper classes to represent no storage.
inline constexpr cat::Monostate monostate;

// Including the `<cat/runtime>` library is required to link a libCat program,
// because it contains the `_start` symbol.
#include <cat/runtime>

// Necessary forward declarations.
class String;

// `assert()` is used throughout the library.
#include <cat/debug>

// `NoType` is required for `TRY()`.
#include <cat/notype>

// Unwrap an error-like container such as `cat::Scaredy` or `cat::Maybe` iff
// it holds a value, otherwise propagate it up the call stack. This works due to
// a GCC extension, statement expressions.
#define TRY(container)                                                      \
    ({                                                                      \
        using TRY_T = decltype(container);                                  \
        /* `if constexpr` does not work right in a statement expression. */ \
        /* If a `nullopt` is returned, this can fail to compile even if */  \
        /* the constant expression is false. In that case, `NoType` is */   \
        /* the return type instead, but of course `NoType` will never be */ \
        /* returned.*/                                                      \
        using ReturnType =                                                  \
            cat::Conditional<cat::is_specialization<TRY_T, cat::Maybe>,     \
                             cat::detail::NullOpt, cat::NoType>;            \
                                                                            \
        if (!((container).has_value())) {                                   \
            if constexpr (cat::is_specialization<TRY_T, cat::Maybe>) {      \
                return ReturnType{};                                        \
            } else {                                                        \
                return (container);                                         \
            }                                                               \
        }                                                                   \
        (container).value();                                                \
    })

// Placement `new`.
[[nodiscard]] inline constexpr auto operator new(unsigned long, void* p_address)
    -> void* {
    return p_address;
}

// `new[]` and `delete[]` are defined for use in a `constexpr` context.
[[nodiscard]] inline auto operator new[](unsigned long, void* p_address)
    -> void* {
    return p_address;
}

// NOLINTNEXTLINE Let this be `inline`.
[[nodiscard]] inline auto operator new[](unsigned long) -> void* {
    return reinterpret_cast<void*>(1ul);
}

namespace std {
enum class align_val_t : __SIZE_TYPE__ {
};
}  // namespace std

// NOLINTNEXTLINE Let this be `inline`.
inline void operator delete[](void*){};
inline void operator delete[](void*, unsigned long){};
inline void operator delete[](void*, unsigned long, std::align_val_t){};

// NOLINTNEXTLINE Let this be `inline`.
[[nodiscard]] inline auto operator new[](unsigned long, std::align_val_t align)
    -> void* {
    return reinterpret_cast<void*>(align);
}
