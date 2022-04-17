// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <type_traits>

// TODO: add a `meta::invocable` concept.
template <typename Function>
consteval auto meta::constant_evaluate(auto value) requires(
    meta::is_invocable<Function>) {
    return value();
}

consteval auto meta::constant_evaluate(auto value) {
    return value;
}
