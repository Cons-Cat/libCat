// -*- mode: c++ -*-
// vim: set ft=cpp:

// TODO: add a `meta::invocable` concept.
consteval auto meta::constant_evaluate(auto value) {
    return value();
}
