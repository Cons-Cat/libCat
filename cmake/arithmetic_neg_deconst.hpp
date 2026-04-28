// -*- mode: c++ -*-
// Config-time negative TUs only. Same idea as `deconst_number` in
// the `test_arithmetic` unit tests. Wrapping
// a literal (or a prvalue) in this strips its constant-expression-ness
// without changing the value, so it no longer short-circuits an `enable_if` or
// a `consteval` overload the way a bare integral literal in a constructor or
// on the RHS of `+=` can.

#pragma once

namespace cat_neg_probe {
auto
deconst_number(auto i) {
   return i;
}
}  // namespace cat_neg_probe
