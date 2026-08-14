// -*- mode: c++ -*-
// vim: set ft=cpp:
#pragma once

#include <cat/format>

// `monostate_type` lives in `global_includes.hpp`, which is parsed before
// `formatter_base` exists. The specialization is declared under the class and
// defined here after `<cat/format>` completes `formatter_base`.

namespace cat {

template <typename CharT>
struct formatter<monostate_type, CharT> : formatter_base<CharT> {
   auto
   format(monostate_type /*value*/, format_context& context) const
      -> scaredy_format<void> {
      return context.append("monostate");
   }
};

}  // namespace cat
