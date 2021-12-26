// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <type_traits.h>
#include <utility.h>

/* This smart pointer is, by design, significantly dumber than the `std::any`
 * in the C++20 STL. This type's basic purpose is to make casting to `void*`
 * less tedious, at a low overhead. There is a cost in casting any 4-byte type
 * into a 8-byte type, but that is inherent to the idea of using `void*` for
 * holding generic data. Monomorphism may be preferable to `Any` in some cases.
 *
 * The primary disadvantages of this type to the canonical `std::any` are that
 * this cannot destruct the data it represents, and this cannot allocate large
 * data-types automatically. Invisible memory allocation is a non-goal of
 * libCat, and the STL achieves both of those with a high performance overhead.
 * For instance, the canonical `std::any` cannot be held in an 8-byte register,
 * whereas libCat's `Any` can. */

struct Any {
    void* value;

    constexpr Any() = default;

    constexpr Any(auto const& input) requires(sizeof(input) <= sizeof(value)) {
        this->value = meta::bit_cast<void*>(input);
    };

    template <typename T>
    operator T() const requires(!meta::is_same_v<void, T>) {
        return meta::bit_cast<T>(this->value);
    }
};
