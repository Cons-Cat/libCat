#pragma once

// clang-format off
#include <cstdint>

extern "C" void _start();
// void exit(i32);  // NOLINT

template <typename T>
struct Result;

#include <result.hpp>
#include <start_exit.hpp>
// clang-format on
