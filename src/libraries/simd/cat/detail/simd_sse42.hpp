#pragma once

#include <cat/detail/simd_sse42_fwd.hpp>

#include <cat/meta>

namespace x64 {

template <string_control control_mask>
constexpr auto compare_implicit_length_strings(auto const& vector_1,
                                               auto const& vector_2) -> bool;

template <string_control control_mask>
constexpr auto compare_implicit_length_strings_return_index(
    auto const& vector_1, auto const& vector_2) -> cat::int4;

}  // namespace x64
