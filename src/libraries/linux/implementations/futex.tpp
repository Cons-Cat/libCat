// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/linux>

namespace nix {

namespace detail {

inline constexpr cat::uword futex_op_command_mask{0x7fu};

inline constexpr cat::uword futex_op_options_mask = cat::to_underlying(
   futex_options::private_process | futex_options::clock_realtime
);

[[nodiscard]]
constexpr auto
futex_command_bits(futex_command const command) -> cat::uword {
   return cat::to_underlying(command) & futex_op_command_mask;
}

[[nodiscard]]
constexpr auto
futex_options_bits(futex_options const options) -> cat::uword {
   return cat::to_underlying(options) & futex_op_options_mask;
}

}  // namespace detail

// Encoded `op` word for futex syscalls. Command bits plus `futex_options`
// flags.
struct futex_op {
   cat::uword encoded;

   [[nodiscard]]
   constexpr auto
   command() const -> futex_command {
      return static_cast<futex_command>(
         encoded & detail::futex_op_command_mask
      );
   }

   [[nodiscard]]
   constexpr auto
   options() const -> futex_options {
      return static_cast<futex_options>(
         encoded & detail::futex_op_options_mask
      );
   }

   constexpr void
   set_command(futex_command const cmd) {
      encoded = (encoded & ~detail::futex_op_command_mask)
                | detail::futex_command_bits(cmd);
   }

   constexpr void
   set_options(futex_options const opts) {
      encoded = (encoded & ~detail::futex_op_options_mask)
                | detail::futex_options_bits(opts);
   }
};

static_assert(sizeof(futex_op) == sizeof(cat::uword));

[[nodiscard]]
constexpr auto
make_futex_op(
   futex_command const command,
   futex_options const options = futex_options::none
) -> futex_op {
   return futex_op{
      .encoded = detail::futex_command_bits(command)
                 | detail::futex_options_bits(options),
   };
}

template <class Kind>
[[nodiscard]]
inline auto
basic_futex<Kind>::wait(
   cat::uint4 const expected, futex_timespec const* _Nullable p_timeout
) -> scaredy_nix<void> {
   scaredy_nix<cat::idx> const result = sys_futex(
      *this, make_futex_op(futex_command::wait, futex_options::private_process),
      expected, p_timeout, nullptr, cat::uint4{}
   );
   if (!result.has_value()) {
      return scaredy_nix<void>(result.error());
   }
   return scaredy_nix<void>(cat::monostate);
}

template <class Kind>
[[nodiscard]]
inline auto
basic_futex<Kind>::wake() -> scaredy_nix<cat::idx> {
   return sys_futex(
      *this, make_futex_op(futex_command::wake, futex_options::private_process),
      1u, nullptr, nullptr, 0u
   );
}

template <class Kind>
[[nodiscard]]
inline auto
basic_futex<Kind>::wake_all() -> scaredy_nix<cat::idx> {
   return sys_futex(
      *this, make_futex_op(futex_command::wake, futex_options::private_process),
      futex_bitset_match_any, nullptr, nullptr, 0u
   );
}

template <class Kind>
[[nodiscard]]
inline auto
basic_futex<Kind>::compare_requeue(
   cat::uint4 const wake_limit, cat::uint4 const requeue_limit,
   futex_word& target, cat::uint4 const expected_source_value
) -> scaredy_nix<cat::idx> {
   return sys_futex(
      *this,
      make_futex_op(
         futex_command::compare_requeue, futex_options::private_process
      ),
      wake_limit, requeue_limit, &target, expected_source_value
   );
}

}  // namespace nix
