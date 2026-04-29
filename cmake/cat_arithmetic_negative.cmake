# This file is flagrantly "vibe-coded". It may not be up to the standards of
# most libCat code.

# Configure-time `-fsyntax-only` checks for `cat::arithmetic`, `cat::index`, and
# `cat::arithmetic_ptr`. Every probe must be ill-formed.
#
# Bodies and naming follow the **inverse** of the `test_arithmetic` unit tests
# (compound assignment, brace OOR, `idx`, `arithmetic_ptr`, mixed-wide bitwise,
# `__attribute__((enable_if(...)))` on some `operator+=`/`operator=` paths in
# `overflow_reference`/`index`, etc.) Only **constructors** and **`operator*`* /
# assignment / compound-assignment / brace-init expressions (no `static_assert`
# in probe sources, no concept-`requires` probes in those sources). The positive
# suite still uses `static_assert` and concepts.
#
# `cmake/cat_arithmetic_neg_cases.cmake` holds ad hoc probes,
# `cat_arithmetic_neg_matrix.cmake` the full `intN`/`uintN` = / `icvt` /
# compound grid, and a width `foreach` here matches every `int1`..
# `int8`/`uint1`..`uint8` OOR case.
# Pure type-trait negations (e.g. `!is_signed`, `!is_lvalue_reference`) are not
# expressed as TUs: they are not an ill-formed *constructor or `operator*` in
# user code.
#
# When `CAT_BUILD_ARITHMETIC_NEGATIVE_TESTS=ON`, checks run at **configure**
# time. Full compiler text for each *must-reject* probe (stderr/stdout) is
# written to `CMakeFiles/cat_arithmetic_neg/compile_diagnostics.log` in the
# build tree.
# A final `message(STATUS` reports counts and points at that file. Set
# `CAT_ARITHMETIC_NEG_ECHO_DIAGNOSTICS=ON` to `message(STATUS` every probe (very
# verbose on configure output). No CTest, no `cmake --build`.
#
# Local:
#   cmake -S . -B build-neg -G Ninja -DCMAKE_CXX_COMPILER=clang++ \
#     -DCAT_BUILD_ARITHMETIC_NEGATIVE_TESTS=ON

option(CAT_ARITHMETIC_NEG_ECHO_DIAGNOSTICS
  "With CAT_BUILD_ARITHMETIC_NEGATIVE_TESTS, print each must-reject probe to configure output (hundreds of lines)."
  OFF)
option(CAT_BUILD_ARITHMETIC_NEGATIVE_TESTS
  "Enable -fsyntax-only checks for cat::arithmetic / index / ptr (ill-formed probes only) at configure time."
  OFF)

if (NOT CAT_BUILD_ARITHMETIC_NEGATIVE_TESTS)
  return()
endif()

if (NOT DEFINED CAT_INCLUDE_SUBDIRS)
  message(FATAL_ERROR
    "CAT_BUILD_ARITHMETIC_NEGATIVE_TESTS: include `cat_arithmetic_negative.cmake` "
    "after `add_subdirectory(src/)` so `CAT_INCLUDE_SUBDIRS` is set.")
endif()

# Match `cat` include layout (one `-I` per list entry) plus this directory for
# `arithmetic_neg_deconst.hpp` (`deconst_number` see `test_arithmetic`).
set(_cat_neg_includes "${CMAKE_SOURCE_DIR}/src")
foreach(_d IN LISTS CAT_INCLUDE_SUBDIRS)
  list(APPEND _cat_neg_includes "${CMAKE_SOURCE_DIR}/src/libraries/${_d}")
endforeach()
list(APPEND _cat_neg_includes "${CMAKE_SOURCE_DIR}/cmake")
unset(_d)

set(_cat_neg_compile_args
  -std=gnu++26
  -fsyntax-only)
foreach(_d IN LISTS _cat_neg_includes)
  list(APPEND _cat_neg_compile_args "-I${_d}")
endforeach()
unset(_d)
unset(_cat_neg_includes)

# Match normal libCat TUs: `global_includes.hpp` forward-declares
# `default_compact_trait`/`compact` before `<cat/maybe>` pulls
# `<cat/arithmetic>`, so arithmetic's `default_compact_trait<index<...>>`
# specialization is valid.
list(APPEND _cat_neg_compile_args
  -nostdlib
  -nostdlib++
  -fno-exceptions
  -fno-rtti
  "-include" "${CMAKE_SOURCE_DIR}/src/global_includes.hpp")

# Macro (not `function`) so per-probe counters in this file stay visible. No
# `ninja`/`cmake --build` is involved: the compiler is invoked at configure by
# `execute_process` only. The build tree is not required for linking.
macro(_cat_neg_expect_illformed _name _src)
  set(_cat_n_path
    "${CMAKE_BINARY_DIR}/CMakeFiles/cat_arithmetic_neg/${_name}.cpp")
  file(WRITE "${_cat_n_path}" "${_src}")
  execute_process(
    COMMAND
      "${CMAKE_CXX_COMPILER}"
      ${_cat_neg_compile_args}
      "${_cat_n_path}"
    RESULT_VARIABLE _neg_rc
    OUTPUT_VARIABLE _neg_out
    ERROR_VARIABLE _neg_err
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_STRIP_TRAILING_WHITESPACE
  )
  if (_neg_rc EQUAL 0)
    set(_neg_log "${_neg_err}")
    if (NOT _neg_log)
      set(_neg_log "${_neg_out}")
    endif()
    if (NOT _neg_log)
      set(_neg_log "(no output)")
    endif()
    message(SEND_ERROR
      "CAT_BUILD_ARITHMETIC_NEGATIVE_TESTS: case '${_name}' built clean with -fsyntax-only; expected a hard error. Output:\n"
      "${_neg_log}")
  else()
    file(
      APPEND
      "${_cat_neg_diag_path}"
      "==== ${_name} (expected failure, exit code ${_neg_rc}) ====\n"
    )
    if (_neg_err)
      file(APPEND
        "${_cat_neg_diag_path}" "stderr:\n${_neg_err}\n"
      )
    endif()
    if (_neg_out)
      file(APPEND
        "${_cat_neg_diag_path}" "stdout:\n${_neg_out}\n"
      )
    endif()
    if (NOT _neg_err AND NOT _neg_out)
      file(
        APPEND
        "${_cat_neg_diag_path}" "(no stderr/stdout from compiler)\n"
      )
    endif()
    file(
      APPEND
      "${_cat_neg_diag_path}" "\n"
    )
    if (CAT_ARITHMETIC_NEG_ECHO_DIAGNOSTICS)
      set(
        _cat_neg_merged_err_out
        "${_neg_err}\n${_neg_out}"
      )
      string(
        SUBSTRING
        "${_cat_neg_merged_err_out}" 0 800 _cat_neg_echo
      )
      if (NOT _cat_neg_echo)
        set(_cat_neg_echo "${_name} (empty)")
      endif()
      message(
        STATUS
        "CAT_ARITHMETIC_NEG echo ${_name}:\n${_cat_neg_echo}…\n"
      )
      unset(_cat_neg_merged_err_out)
    endif()
    math(EXPR _cat_neg_n_illformed_ok
      "${_cat_neg_n_illformed_ok} + 1")
  endif()
endmacro()

set(
  _cat_neg_diag_path
  "${CMAKE_BINARY_DIR}/CMakeFiles/cat_arithmetic_neg/compile_diagnostics.log"
)
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/CMakeFiles/cat_arithmetic_neg")
file(WRITE
  "${_cat_neg_diag_path}"
  "cat::arithmetic negative -fsyntax-only (must-reject stderr/stdout)\n"
  "Compiler: ${CMAKE_CXX_COMPILER}\n"
  "-----\n"
)

set(_cat_neg_n_illformed_ok 0)

# Out-of-range brace constants: every `uint1`..`uint8`/`int1`..`int8` width
# (mirrors the per-width story in the positive `can_brace_init` block).
foreach(n IN ITEMS 1 2 4 8)
  _cat_neg_expect_illformed("uint${n}-brace-neg1" "#include <cat/arithmetic>
void t() { (void)cat::uint${n}{-1}; }
")
  if (n EQUAL 1)
    set(_il "1000")
  elseif (n EQUAL 2)
    set(_il "100000")
  elseif (n EQUAL 4)
    set(_il "5000000000")
  else()
    # 2^64: too large for any 64-bit integer type (fails the lexer / constant
    # path).
    set(_il "18446744073709551616u")
  endif()
  _cat_neg_expect_illformed("int${n}-const-too-large" "#include <cat/arithmetic>
void t() { (void)cat::int${n}{${_il}}; }
")
  unset(_il)
endforeach()
unset(n)

include(${CMAKE_SOURCE_DIR}/cmake/cat_arithmetic_neg_cases.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/cat_arithmetic_neg_matrix.cmake)

unset(_cat_neg_compile_args)

file(
  GLOB
  _cat_neg_glob
  "${CMAKE_BINARY_DIR}/CMakeFiles/cat_arithmetic_neg/*.cpp"
)
list(LENGTH _cat_neg_glob _cat_neg_n_glob)
set(_cat_neg_n_calc "${_cat_neg_n_illformed_ok}")
if (NOT _cat_neg_n_glob EQUAL _cat_neg_n_calc)
  message(
    WARNING
    "CAT_BUILD_ARITHMETIC_NEGATIVE_TESTS: probe .cpp on disk count "
    "(${_cat_neg_n_glob}) != ill+must total (${_cat_neg_n_calc}) (glob check)"
  )
endif()
message(
  STATUS
  "CAT_BUILD_ARITHMETIC_NEGATIVE_TESTS: OK - compiler `${CMAKE_CXX_COMPILER}` "
  "(${_cat_neg_n_illformed_ok} must-reject) "
  "matches ${_cat_neg_n_glob} -fsyntax-only .cpp. "
  "Per-probe compiler output: ${CMAKE_BINARY_DIR}/CMakeFiles/cat_arithmetic_neg/compile_diagnostics.log "
  "(or configure with -DCAT_ARITHMETIC_NEG_ECHO_DIAGNOSTICS=ON to print excerpts here)"
  ". No `cmake --build`"
)
unset(_cat_neg_n_glob)
unset(_cat_neg_n_calc)
unset(_cat_neg_diag_path)
