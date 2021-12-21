// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

namespace std::detail {

struct none {};

}  // namespace std::detail

constexpr std::detail::none none;

/* TODO: Any is not a good solution to this, because it cannot hold values
 * larger than 8 bytes. That is an edge case issue, because you would never
 * actually use a sentinel if it is larger than 8 bytes. */
template <Any value>
struct Sentinel {
    inline static auto failure_value = value;
};

/* In order to overload the template parameter list of a struct, specializing a
 * variadic template is required. */
template <typename T, Sentinel...>
struct Maybe {};

template <typename T>
struct Maybe<T> {
  private:
    T const value;

  public:
    bool const has_some;

    Maybe() = delete;
    constexpr Maybe(std::detail::none) : value(), has_some(false) {
    }
    constexpr Maybe(T in_value) : value(in_value), has_some(false) {
    }

    constexpr auto or_return(T fallback_value) -> T {
        if (has_some) {
            return value;
        }
        return fallback_value;
    }

    /* If this object does not hold a value, the return of this function is
     * undefined. When building -O0, this function panics if a value is not
     * held. When optimizations are enabled, that safety check is elided. */
    constexpr auto unsafe_value() -> T {
#ifdef __OPTIMIZE__
        return value;
#else
        if (has_some) {
            return value;
        }
        // TODO: Error message.
        exit(1);
#endif
    }

    // TODO: Use an invocable concept.
    constexpr auto and_then(auto callback) -> Maybe<T> {
        if (has_some) {
            return callback(value);
        }
        return none;
    }
};

// TODO: Reduce excessive code duplication.
template <typename T, Sentinel sentinel>
struct Maybe<T, sentinel> {
    T const value;

    Maybe() = delete;
    constexpr Maybe(std::detail::none) : value() {
    }
    constexpr Maybe(T in_value) : value(in_value) {
    }

    constexpr auto or_return(T fallback_value) -> T {
        if (value != sentinel) {
            return value;
        }
        return fallback_value;
    }

    /* If this object does not hold a value, the return of this function is
     * undefined. When building -O0, this function panics if a value is not
     * held. When optimizations are enabled, that safety check is elided. */
    constexpr auto unsafe_value() -> T {
#ifdef __OPTIMIZE__
        return value;
#else
        if (value != sentinel) {
            return value;
        }
        // TODO: Error message.
        exit(1);
#endif
    }

    // TODO: Use an invocable concept.
    constexpr auto and_then(auto callback) -> Maybe<T, sentinel> {
        if (value != sentinel) {
            return callback(value);
        }
        return none;
    }
};
