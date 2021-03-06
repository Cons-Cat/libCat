// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/meta>

// TODO: Support a function returning `void`.

template <cat::Invocable Function>
consteval auto cat::constant_evaluate(auto value) {
    return value();
}

consteval auto cat::constant_evaluate(auto value) {
    return value;
}
