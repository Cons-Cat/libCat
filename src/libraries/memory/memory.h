// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

/* RAII wrappers are trivially constructible types that cannot be copied. They
 * must be moved to pass into functions. Their destructor will call a free()
 * method on the type they wrap iff that types has a free() method. */

template <typename T>
struct RAII : T {
    RAII() = default;
    RAII(RAII<T>&&){};

    ~RAII() = default;
    // clangd gives a false-positive error due to this destructor:
    ~RAII() requires(T::free()) {
        this->free();
    }
};
