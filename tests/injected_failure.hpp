// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

// Element types and helpers for testing the exception safety of containers.

#if __has_feature(cxx_exceptions)
#include <cat/arithmetic>

namespace {
// Thrown by `fragile` after `fragile::arm()`.
struct injected_failure {};

// Copying and moving count as constructors. `arm(n)` makes the nth later
// constructor throw, and `live` counts objects whose constructors finished.
struct fragile {
   static inline cat::iword live = 0;
   static inline cat::iword until_throw = 0;
   cat::int4 value;

   static void
   arm(cat::iword count) {
      until_throw = count;
   }

   static void
   disarm() {
      until_throw = 0;
   }

   explicit constexpr fragile(cat::int4 input) : value(input) {
      constructed();
   }

   constexpr fragile() : value(0) {
      constructed();
   }

   constexpr fragile(fragile const& other) : value(other.value) {
      constructed();
   }

   constexpr fragile(fragile&& other) : value(other.value) {
      constructed();
   }

   constexpr auto
   operator=(fragile const& other) -> fragile& {
      value = other.value;
      return *this;
   }

   constexpr auto
   operator=(fragile&& other) -> fragile& {
      value = other.value;
      return *this;
   }

   constexpr ~fragile() {
      --live;
   }

 private:
   static constexpr void
   constructed() {
      if (until_throw > 0) {
         --until_throw;
         if (until_throw == 0) {
            throw injected_failure{};
         }
      }
      ++live;
   }
};

template <typename Fn>
auto
throws_injected_failure(Fn&& fn) -> bool {
   try {
      fn();
   } catch (injected_failure const&) {
      return true;
   }
   return false;
}
}  // namespace
#endif
