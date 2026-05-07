# This file is flagrantly "vibe-coded". It may not be up to the standards of
# most libCat code.

# `cat-ir` symbol-matching helpers. Loaded via `include()` from the disasm
# and extract pure-CMake drivers; see `scripts/cat_ir_disasm.cmake` and
# `scripts/cat_ir_extract.cmake`.
#
# `fn=<name>` is a literal source-level identifier (typically namespace-
# qualified, e.g. `cat::pow`). The pattern wraps it as
#
#   ^<escaped FN>([(<.].*)?$
#
# against the *demangled* form, so a single bare name collects:
#   - all overloads (`cat::pow(int)`, `cat::pow(double)`),
#   - all template instantiations (`cat::pow<int>(int)`),
#   - and LTO clones (`cat::pow.cold`, `cat::pow.0`, ...)
# without forcing the user to write a regex. Symbols whose demangled form
# *extends* the name with an identifier character (e.g. `cat::power`,
# `cat::pow_helper`) are *not* matched.

function(_cat_ir_regex_escape input out_var)
  # Bracket strings (`[[...]]`) sidestep CMake's quoted-string backslash
  # rules: `[[\]]` is a literal one-character backslash, `[[\\]]` is two.
  string(REPLACE [[\]] [[\\]] _x "${input}")
  foreach(_c IN ITEMS [[^]] [[$]] [[.]] [[*]] [[+]] [[?]] [[|]] [[(]] [[)]] [=[[]=] [=[]]=])
    string(REPLACE "${_c}" "\\${_c}" _x "${_x}")
  endforeach()
  set(${out_var} "${_x}" PARENT_SCOPE)
endfunction()

function(_cat_ir_function_pattern name out_var)
  # Matches the demangled form against either:
  #   <fn>[<...>][(...)][.<clone>]              -- plain function or LTO clone.
  #   <return-type> <fn>[<...>](...)[ <cv-ref>] -- function-template
  #     instantiation. Itanium C++ ABI mangles a return type into the symbol
  #     for templates only, and llvm-cxxfilt prefixes it on the demangled
  #     form, so a bare `fn=cat::pow` would otherwise fail to find any
  #     `int cat::pow<...>(...)`.
  #
  # `[^()]*` for the optional return type chunk: function-pointer return
  # types contain `(` and would confuse the prefix split, but those are rare
  # enough that the simpler shape wins. The trailing ` ` (space before the
  # function name) is what disambiguates a return-type prefix from a name
  # that happens to start with non-`fn` text.
  _cat_ir_regex_escape("${name}" _esc)
  set(${out_var} "^([^()]* )?${_esc}([(<.].*)?$" PARENT_SCOPE)
endfunction()

# `_cat_ir_match_mangled(NM CXXFILT BIN FN OUT_VAR)` -- list every defined
# symbol in `BIN` (objects, archives, *bitcode* -- llvm-nm reads them all)
# whose *demangled* form matches the pattern, and return the matching
# *mangled* names. Used by the bc/ll extract path which feeds the result
# straight to `llvm-extract --func=`.
function(_cat_ir_match_mangled nm_path cxxfilt_path bin fn out_var)
  if (NOT EXISTS "${bin}")
    set(${out_var} "" PARENT_SCOPE)
    return()
  endif()

  # Two passes over the same nm output keep the mangled and demangled
  # lists in lockstep. `--defined-only` drops undefined externs; `-j`
  # strips everything but the symbol name.
  execute_process(
    COMMAND "${nm_path}" --defined-only -j "${bin}"
    OUTPUT_VARIABLE _mangled_text
    OUTPUT_STRIP_TRAILING_WHITESPACE
    COMMAND_ERROR_IS_FATAL ANY)
  if (_mangled_text STREQUAL "")
    set(${out_var} "" PARENT_SCOPE)
    return()
  endif()

  execute_process(
    COMMAND "${nm_path}" --defined-only -j "${bin}"
    COMMAND "${cxxfilt_path}" -n
    OUTPUT_VARIABLE _demangled_text
    OUTPUT_STRIP_TRAILING_WHITESPACE
    COMMAND_ERROR_IS_FATAL ANY)

  string(REPLACE "\n" ";" _mangled_list   "${_mangled_text}")
  string(REPLACE "\n" ";" _demangled_list "${_demangled_text}")

  list(LENGTH _mangled_list   _n_man)
  list(LENGTH _demangled_list _n_dem)
  if (NOT _n_man EQUAL _n_dem)
    message(FATAL_ERROR
      "cat-ir: nm/cxxfilt line-count mismatch on ${bin} "
      "(${_n_man} mangled vs ${_n_dem} demangled).")
  endif()

  _cat_ir_function_pattern("${fn}" _pattern)

  set(_matched "")
  foreach(_man _dem IN ZIP_LISTS _mangled_list _demangled_list)
    if (_dem MATCHES "${_pattern}")
      list(APPEND _matched "${_man}")
    endif()
  endforeach()
  list(REMOVE_DUPLICATES _matched)
  set(${out_var} "${_matched}" PARENT_SCOPE)
endfunction()

# `_cat_ir_match_addrs(OBJDUMP BIN FN OUT_ADDRS OUT_SIZES OUT_SECTIONS)` --
# scan `BIN`'s symbol table (`llvm-objdump --syms --demangle`) and return
# parallel lists of `addr;size;section` for every defined function whose
# demangled name matches. Used by the `.s` driver, which then issues one
# `llvm-objdump -d --section=<S> --start-address=<A> --stop-address=<A+N>`
# per match -- the only reliable way to disassemble weak / function-
# section symbols (llvm-objdump's `--disassemble-symbols=` silently drops
# them, see https://github.com/llvm/llvm-project/issues/...).
function(_cat_ir_match_addrs objdump_path bin fn
                             out_addrs out_sizes out_sections)
  if (NOT EXISTS "${bin}")
    set(${out_addrs}    "" PARENT_SCOPE)
    set(${out_sizes}    "" PARENT_SCOPE)
    set(${out_sections} "" PARENT_SCOPE)
    return()
  endif()

  execute_process(
    COMMAND "${objdump_path}" --syms --demangle "${bin}"
    OUTPUT_VARIABLE _syms
    OUTPUT_STRIP_TRAILING_WHITESPACE
    COMMAND_ERROR_IS_FATAL ANY)

  _cat_ir_function_pattern("${fn}" _pattern)

  # llvm-objdump --syms format (one symbol per line):
  #   <addr> <flag-block> <section>\t<size> <name>
  # `<flag-block>` always contains ` F ` for function symbols. `<section>`
  # is left mangled even with --demangle so it remains a stable token.
  string(REPLACE "\n" ";" _lines "${_syms}")
  set(_addrs "")
  set(_sizes "")
  set(_sects "")
  foreach(_line IN LISTS _lines)
    string(FIND "${_line}" "\t" _tab)
    if (_tab LESS 0)
      continue()
    endif()
    string(SUBSTRING "${_line}" 0 ${_tab} _head)
    math(EXPR _after "${_tab} + 1")
    string(SUBSTRING "${_line}" ${_after} -1 _tail)
    if (NOT _head MATCHES " F ")
      continue()
    endif()
    string(REGEX MATCHALL "[^ ]+" _head_fields "${_head}")
    list(LENGTH _head_fields _nh)
    if (_nh LESS 4)
      continue()
    endif()
    list(GET _head_fields 0  _addr)
    list(GET _head_fields -1 _sect)
    if (NOT _tail MATCHES "^([0-9a-fA-F]+) +(.*)$")
      continue()
    endif()
    set(_size "${CMAKE_MATCH_1}")
    set(_name "${CMAKE_MATCH_2}")
    if (NOT _name MATCHES "${_pattern}")
      continue()
    endif()
    list(APPEND _addrs "${_addr}")
    list(APPEND _sizes "${_size}")
    list(APPEND _sects "${_sect}")
  endforeach()

  set(${out_addrs}    "${_addrs}" PARENT_SCOPE)
  set(${out_sizes}    "${_sizes}" PARENT_SCOPE)
  set(${out_sections} "${_sects}" PARENT_SCOPE)
endfunction()
