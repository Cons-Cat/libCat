#pragma once

#include <any>
#include <concepts>
#include <cstdlib>
#include <type_traits>
#include <utility>

/* This file contains a general-purpose error handling tool. Result<> is a
 * templated struct that either contains some data (unless it holds void) or an
 * error-code. The error-code is a 64-bit value that can be either an integer or
 * a pointer to a string that may be printed. To extract a value out of a
 *result, call one of its member functions.
 *
 * An error code or string pointer must be wrapped in an Error, such as:
 ** return Error(1);
 *
 * Otherwise, it is presumed that a Result holds a value rather than an error.
 * This is done to prevent monomorphizing the Result on its error types like
 * Rust, which provides type-safety at the cost of pessimized compile-times and
 * runtime overhead. This Result is a zero-overhead abstraction over C-style
 * error handling.
 *
 * Errors are implicitly convertible into a Result, and a T is implicitly
 * convertible into a Result<T>.
 *
 * Result types are marked [[nodiscard]]. With confidence that an invocation of
 * a function returning a Result will never fail, you may call .unsafe_value(),
 * which will skip error-handling when compiler optimizations are enabled. */

template <typename T = void>
struct Result;

struct [[nodiscard]] Error {
    /* TODO: This should be a more precise type. It holds either an i64 or a
     * char const*. */
    std::any code;

    // TODO: Rethink Error and Result constructors.
    Error() = default;

    // This must be explicit to disambiguate returning an integer to a Result.
    Error(auto in_code) {
        code = static_cast<std::any>(in_code);
    }

    template <typename T>
    operator Result<T>() const {
        return Result<T>(code);
    }

    // template <std::integral auto T> does not work for some reason.
    template <typename T>
    constexpr operator T() const
        // TODO: Make is_bool<> traits.
        requires(std::is_same_v<T, std::any>() || std::is_same_v<T, bool>) {
        return code;
    }

    auto operator==(Error right_error) const -> bool {
        return this->code == right_error.code;
    }
};

template <typename T>
struct [[nodiscard("To skip error-handling, call .unsafe_value()")]] Result {
    /* The error code must be laid out first, so that _start() can reliably
     * extract it from %rax. */
    Error error_code;  // Error is a 64-bit value.

  private:
    /* char should be a relatively unintrusive dummy data for when this holds
     * void. Reflection TS in future C++ will provide conditional-members, which
     * would be a better solution. */
    using ValueType = std::conditional_t<std::is_void_v<T>, char, T>;
    ValueType const value;
    bool const is_ok;

  public:
    Result(Error in_code) : error_code(in_code), value(), is_ok(false) {
    }

    Result(ValueType in_value) : error_code(), value(in_value), is_ok(true) {
    }

    auto or_return(ValueType const& in_value)->ValueType {
        return in_value;
    }

    // TODO: Add std::invokable concept
    auto or_do(auto callback) {
        if (!is_ok) {
            return callback();
        }
        if constexpr (!std::is_void_v<T>) {
            if constexpr (!std::is_void_v<T>) {
                return this->value;
            }
        }
    }

    // This function is intended to be the return value of its calling function.
    [[nodiscard]] auto or_propagate()->Result<T> {
        return *this;
    }

    /* If this object does not hold a value, the return of this function is
     * undefined. When building -O0, this function panics if a value is not
     * held. When optimizations are enabled, that safety check is elided. */
    auto unsafe_value()->T {
#ifdef __OPTIMIZE__
        return value;
#endif
        if (is_ok) {
            return value;
        }
        // TODO: Error message.
        exit(EXIT_FAILURE);
    }

    // TODO: Pass in the exit code and error message with overloads.
    auto or_panic()->T {
        if (is_ok) {
            if constexpr (!std::is_void_v<T>) {
                return this->value;
            }
            return;
        }
        exit(EXIT_FAILURE);
    }

    auto or_panic(char const* /*error_message*/)->T {
        if (is_ok) {
            if constexpr (!std::is_void_v<T>) {
                return this->data;
            }
            return;
        }
        // TODO: print(error_message);
        exit(EXIT_FAILURE);
    }

    /* Because the error code is 64-bit, it could contain a non-null pointer to
     * an error string. This prints that error message. */
    auto or_print_panic()->T {
        if (is_ok) {
            if constexpr (!std::is_void_v<T>) {
                return this->value;
            }
            return;
        }
        // TODO: print(*error_code);
        exit(EXIT_FAILURE);
    }

    /* It may be desirable to skip the costs of printing error messages in
     * release builds of an application. These following functions only print an
     * error message when building -O0. */
    auto or_panic_debug(char const* /*error_message*/)->T {
        if (is_ok) {
            if constexpr (!std::is_void_v<T>) {
                return this->data;
            }
            return;
        }
#ifdef __OPTIMIZE__
        // TODO: print(error_message);
#endif
        exit(EXIT_FAILURE);
    }

    /* Because the error code is 64-bit, it could contain a non-null pointer to
     * an error string. This prints that error message. */
    auto or_print_debug()->T {
        if (is_ok) {
            if constexpr (!std::is_void_v<T>) {
                return this->value;
            }
            return;
        }
#ifdef __OPTIMIZE__
        // TODO: print(error_code);
#endif
        exit(EXIT_FAILURE);
    }
};
