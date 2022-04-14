// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <utility>

#include "global_includes.h"

consteval auto meta::ssizeof(auto anything) -> ssize {
    return static_cast<ssize>(sizeof(anything));
}

template <typename T>
consteval auto meta::ssizeof() -> ssize {
    return static_cast<ssize>(sizeof(T));
}
