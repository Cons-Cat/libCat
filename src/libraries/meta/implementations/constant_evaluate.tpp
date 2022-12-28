// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/meta>

// TODO: Support a function returning `void`.

consteval auto cat::constant_evaluate(is_invocable auto value)
    -> decltype(auto) {
    return value();
}

consteval auto cat::constant_evaluate(auto value) -> decltype(auto) {
    return value;
}
