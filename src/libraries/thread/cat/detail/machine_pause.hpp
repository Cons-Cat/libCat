// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/arithmetic>

namespace cat {

// Insert a `pause` instruction(s) in place. Note that on some architectures, the
// `pause` instruction's latency is 5 cycles, but 140 cycles from Skylake
// onward.
// One `pause` is executed per the value of `delay`.
inline void
machine_pause(int4 delay = 1) {
   // This idea is borrowed from `tbb::detail::machine_pause()`.
   while (delay > 0) {
      __builtin_ia32_pause();
      delay = delay - 1;
   }
}

}  // namespace cat
