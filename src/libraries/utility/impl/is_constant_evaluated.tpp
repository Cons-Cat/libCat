// -*- mode: c++ -*-
// vim: set ft=cpp:

constexpr auto meta::is_constant_evaluated() -> bool {
    return __builtin_is_constant_evaluated();
}