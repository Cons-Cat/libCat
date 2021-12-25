// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <type_traits.h>
#include <utility.h>

/* This smart pointer is, by design, significantly dumber than the std::any
 * in the C++20 STL. This type's basic purpose is to make casting to void* less
 * tedious, at a low overhead. There is a cost in casting any 4-byte type into a
 * 8-byte type, but that is inherent to the idea of using void* for holding
 * generic data. Monomorphism may be a better solution than Any in some cases.
 *
 * The primary disadvantages of this type to the canonical std::any is that it
 * cannot destruct the data it represents, and it cannot allocate large
 * data-types automatically. Invisible memory allocation is a non-goal of
 * libCat, and the STL achieves both of those with a high performance overhead.
 * For instance, the canonical std::any cannot be held in a 8-byte register,
 * whereas libCat's Any can. */

struct Any {
    void* value;

    constexpr Any() = default;

    constexpr Any(auto input) requires(sizeof(input) <= sizeof(value)) {
        meta::bit_cast<void*>(input);
    };

    template <typename T>
    operator T() const {
        return meta::bit_cast<T>(this->value);
    }
};
