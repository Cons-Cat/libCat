// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <utility.h>

template <typename T, isize Size>
struct Buffer {
    T value[Size];

    constexpr Buffer() = default;
    constexpr Buffer(Buffer<T, Size>&& in_buffer) = default;
    constexpr auto operator=(Buffer<T, Size>&&) -> Buffer<T, Size>& = default;

    template <typename... U>
    constexpr Buffer(U&&... arguments) requires(sizeof...(arguments) == Size) {
        T unpacked_data[Size] = {arguments...};
        for (i4 i = 0; i < Size; i++) {
            this->value[i] = unpacked_data[i];
        }
    }

    auto operator[](isize i) -> T& {
        return value[i];
    }
};
