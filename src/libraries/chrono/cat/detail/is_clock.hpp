// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/maybe>
#include <cat/meta>
#include <cat/time_units>

namespace cat {

template <typename Clock>
concept is_clock = requires {
                      requires is_duration<typename Clock::duration>;
                      typename Clock::time_point;
                      { Clock::is_steady } -> is_convertible<bool>;
                      {
                         Clock::now()
                      } -> is_same<maybe<typename Clock::time_point>>;
                   };

}  // namespace cat
