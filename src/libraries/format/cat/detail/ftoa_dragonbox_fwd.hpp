#pragma once

// Forward declarations for Dragonbox float-to-chars. The implementation lives
// in `ftoa_dragonbox.cpp` so `<cat/format>` does not pull the Dragonbox
// templates.

namespace cat::detail::dragonbox {

auto
to_chars(float value, char* _Nonnull p_buffer) -> char* _Nonnull;

auto
to_chars(double value, char* _Nonnull p_buffer) -> char* _Nonnull;

}  // namespace cat::detail::dragonbox
