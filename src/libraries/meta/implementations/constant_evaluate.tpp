// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/meta>

// TODO: add a `cat::invocable` concept.
/*
template <typename Function>
consteval auto cat::constant_evaluate(auto value) requires(
    cat::is_invocable<Function>) {
    return value();
}
*/

consteval auto cat::constant_evaluate(auto value) {
    return value;
}
