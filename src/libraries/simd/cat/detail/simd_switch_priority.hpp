// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>

namespace cat::detail {

// Highest SIMD priority supported by the host CPU.
extern "C"
#ifndef CAT_BUILD_SHARED
   [[gnu::visibility("hidden")]]
#endif
   // NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
   constinit inline int4 simd_dispatch_priority = 40;

}  // namespace cat::detail
