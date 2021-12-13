// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <utility.h>

template <typename T, usize Size>
struct Buffer {
    T value[Size];

    Buffer() = default;

    template <typename... U>
    constexpr Buffer(U&&... arguments) requires(sizeof...(arguments) == Size) {
        T unpacked_data[Size] = {arguments...};
        for (i32 i = 0; i < Size; i++) {
            this->value[i] = unpacked_data[i];
        }
    }

    constexpr Buffer(Buffer<T, Size>&& in_buffer) {
        auto moved_buffer = forward<Buffer<T, Size>>(in_buffer);
        for (i32 i = 0; i < Size; i++) {
            this->value[i] = moved_buffer.value[i];
        }
    }

    constexpr auto operator=(Buffer<T, Size>&&) -> Buffer<T, Size>& = default;
};
