# This file is flagrantly "vibe-coded". It may not be up to the standards of
# most libCat code.

# Negative type-safety compile checks for `cat::basic_int`, `cat::basic_float`,
# `cat::basic_idx`, and `cat::basic_intptr`. Every probe must be ill-formed.
#
# Bodies and naming follow the **inverse** of the `test_arithmetic` unit tests
# (compound assignment, brace OOR, `idx`, `basic_intptr`, mixed-wide bitwise,
# `__attribute__((enable_if(...)))` on some `operator+=`/`operator=` paths in
# `overflow_reference`/`basic_idx`, etc.) Only **constructors** and **`operator*`* /
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
# Each probe is also registered as an `EXCLUDE_FROM_ALL` object target. The
# `ArithmeticTypeCheck` CTest uses launcher-wrapped check targets that invert
# compiler results and build in parallel. The target path reuses `cat-impl`'s PCH
# when `CAT_PCH=ON`.
#
# When `CAT_BUILD_ARITHMETIC_NEGATIVE_TESTS=ON`, the same probes additionally run
# at configure time. Full compiler text for each configure-time probe is written
# to `CMakeFiles/cat_arithmetic_neg/compile_diagnostics.log` in the build tree.
# A final verbose message reports counts and points at that file.
#
# Local:
#   cmake -S . -B build-neg -G Ninja -DCMAKE_CXX_COMPILER=clang++ \
#     -DCAT_BUILD_ARITHMETIC_NEGATIVE_TESTS=ON
# or:
#   just test arithmetic

option(CAT_BUILD_ARITHMETIC_NEGATIVE_TESTS
  "Also run -fsyntax-only checks for cat arithmetic wrapper ill-formed probes at configure time."
  OFF)

find_program(CAT_PYTHON3
  NAMES python3
  DOC "Python 3 for libCat helper scripts.")
if (NOT CAT_PYTHON3)
  message(FATAL_ERROR
    "Arithmetic negative tests require `python3` on PATH.")
endif()

if (NOT DEFINED CAT_INCLUDE_SUBDIRS)
  message(FATAL_ERROR
    "CAT_BUILD_ARITHMETIC_NEGATIVE_TESTS: include `cat_arithmetic_negative.cmake` "
    "after `add_subdirectory(src/)` so `CAT_INCLUDE_SUBDIRS` is set.")
endif()

# Match `cat` include layout (one `-I` per list entry) plus this directory for
# CMake-generated probe includes. `block(PROPAGATE)` keeps the loop scratch
# (`_d`, `_includes`) from leaking into the rest of the file.
block(PROPAGATE _cat_neg_compile_args)
  set(_includes "${CMAKE_SOURCE_DIR}/src")
  foreach(_d IN LISTS CAT_INCLUDE_SUBDIRS)
    list(APPEND _includes "${CMAKE_SOURCE_DIR}/src/libraries/${_d}")
  endforeach()
  list(APPEND _includes "${CMAKE_SOURCE_DIR}/cmake")

  set(_cat_neg_compile_args
    -std=gnu++26
    -Wno-everything # This test is not about warnings.
    -fsyntax-only)
  foreach(_d IN LISTS _includes)
    list(APPEND _cat_neg_compile_args "-I${_d}")
  endforeach()

  # Match normal libCat TUs: `global_includes.hpp` forward-declares
  # `default_compact_trait`/`compact` before `<cat/maybe>` pulls
  # `<cat/arithmetic>`, so arithmetic's `default_compact_trait<basic_idx<...>>`
  # specialization is valid.
  list(APPEND _cat_neg_compile_args
    -nostdlib
    -nostdlib++
    -fno-exceptions
    -fno-rtti
    "-include" "${CMAKE_SOURCE_DIR}/src/global_includes.hpp")
endblock()

# Macro (not `function`) so per-probe counters in this file stay visible.
macro(_cat_neg_expect_illformed _name _src)
  set(_cat_n_path
    "${CMAKE_BINARY_DIR}/CMakeFiles/cat_arithmetic_neg/${_name}.cpp")
  file(WRITE "${_cat_n_path}" "${_src}")

  set(_cat_n_target "cat-arithmetic-neg-${_name}")
  add_library("${_cat_n_target}" OBJECT EXCLUDE_FROM_ALL "${_cat_n_path}")
  target_link_libraries("${_cat_n_target}" PRIVATE cat)
  target_compile_options("${_cat_n_target}" PRIVATE
    ${CAT_CXX_FLAGS_INTERNAL}
    -Wno-everything)
  if (CAT_PCH)
    target_precompile_headers("${_cat_n_target}" REUSE_FROM cat-impl)
  endif()
  list(APPEND _cat_neg_targets "${_cat_n_target}")

  set(_cat_n_check_target "cat-arithmetic-neg-check-${_name}")
  add_library("${_cat_n_check_target}" OBJECT EXCLUDE_FROM_ALL "${_cat_n_path}")
  target_compile_features("${_cat_n_check_target}" PRIVATE
    ${CAT_CXX_STANDARD_FEATURE})
  set_target_properties("${_cat_n_check_target}" PROPERTIES CXX_EXTENSIONS ON)
  target_include_directories("${_cat_n_check_target}" PRIVATE
    $<TARGET_PROPERTY:cat,INTERFACE_INCLUDE_DIRECTORIES>)
  target_compile_options("${_cat_n_check_target}" PRIVATE
    $<TARGET_PROPERTY:cat,INTERFACE_COMPILE_OPTIONS>
    ${CAT_CXX_FLAGS_INTERNAL}
    -Wno-everything)
  if (CAT_PCH)
    target_precompile_headers("${_cat_n_check_target}" REUSE_FROM cat-impl)
  endif()
  set_property(
    TARGET "${_cat_n_check_target}"
    PROPERTY CAT_NEG_REPORT_TARGET "${_cat_n_target}")
  list(APPEND _cat_neg_check_targets "${_cat_n_check_target}")
  math(EXPR _cat_neg_n_cases "${_cat_neg_n_cases} + 1")

  if (CAT_BUILD_ARITHMETIC_NEGATIVE_TESTS)
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
        "CAT_BUILD_ARITHMETIC_NEGATIVE_TESTS: case '${_name}' compiled successfully. Expected a type error! Output:\n"
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
      math(EXPR _cat_neg_n_illformed_ok
        "${_cat_neg_n_illformed_ok} + 1")
    endif()
  endif()
endmacro()

# Everything below operates on file-internal accumulators (`_cat_neg_targets`,
# `_cat_neg_check_targets`, `_cat_neg_n_cases`, `_cat_neg_n_illformed_ok`)
# that aren't read outside this file. Wrap the whole bottom in a single
# `block()` so loop scratch (`n`, `_il`, plus everything dropped by
# `cat_arithmetic_neg_matrix.cmake`) and the macro's leaked locals
# (`_cat_n_path`, `_cat_n_target`, `_cat_n_check_target`) all stay scoped --
# no per-loop `unset()` cleanup needed.
block()
  set(_cat_neg_diag_path
    "${CMAKE_BINARY_DIR}/CMakeFiles/cat_arithmetic_neg/compile_diagnostics.log")
  file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/CMakeFiles/cat_arithmetic_neg")
  file(WRITE
    "${_cat_neg_diag_path}"
    "cat arithmetic wrapper negative -fsyntax-only (must-reject stderr/stdout)\n"
    "Compiler: ${CMAKE_CXX_COMPILER}\n"
    "-----\n")

  set(_cat_neg_n_illformed_ok 0)
  set(_cat_neg_n_cases 0)
  set(_cat_neg_targets)
  set(_cat_neg_check_targets)

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
  endforeach()

  include(${CMAKE_SOURCE_DIR}/cmake/cat_arithmetic_neg_cases.cmake)
  include(${CMAKE_SOURCE_DIR}/cmake/cat_arithmetic_neg_matrix.cmake)

set(_cat_neg_launcher
  "${CMAKE_SOURCE_DIR}/cmake/cat_arithmetic_negative_launcher.py")
set(_cat_neg_case_diag_dir
  "${CMAKE_BINARY_DIR}/CMakeFiles/cat_arithmetic_neg/build_cases")
set(_cat_neg_marker_dir
  "${CMAKE_BINARY_DIR}/CMakeFiles/cat_arithmetic_neg/unexpected")
set(_cat_neg_progress_path
  "${CMAKE_BINARY_DIR}/CMakeFiles/cat_arithmetic_neg/progress.txt")
foreach(_cat_neg_check_target IN LISTS _cat_neg_check_targets)
  get_property(
    _cat_neg_report_target
    TARGET "${_cat_neg_check_target}"
    PROPERTY CAT_NEG_REPORT_TARGET)
  set_property(
    TARGET "${_cat_neg_check_target}"
    PROPERTY CXX_COMPILER_LAUNCHER
      "${CAT_PYTHON3};${_cat_neg_launcher};--target;${_cat_neg_report_target};--marker;${_cat_neg_marker_dir}/${_cat_neg_report_target}.txt;--diagnostic;${_cat_neg_case_diag_dir}/${_cat_neg_report_target}.log;--progress;${_cat_neg_progress_path};--total;${_cat_neg_n_cases}")
endforeach()
set(_cat_neg_check_target cat-arithmetic-negative-checks)
add_custom_target("${_cat_neg_check_target}")
add_dependencies("${_cat_neg_check_target}" ${_cat_neg_check_targets})

set(
  _cat_neg_build_script
  "${CMAKE_BINARY_DIR}/CMakeFiles/cat_arithmetic_neg/run_build_tests.cmake"
)
file(WRITE "${_cat_neg_build_script}" "set(_cat_neg_targets\n")
foreach(_cat_neg_target IN LISTS _cat_neg_targets)
  file(APPEND "${_cat_neg_build_script}" "  [=[${_cat_neg_target}]=]\n")
endforeach()
file(APPEND "${_cat_neg_build_script}" ")\n")
file(APPEND "${_cat_neg_build_script}" [=[
file(WRITE
  "${cat_neg_diag_path}"
  "cat arithmetic wrapper negative CTest build targets\n"
  "Build dir: ${cat_neg_build_dir}\n"
  "-----\n")

string(ASCII 27 _cat_neg_escape)
set(_cat_neg_bold "${_cat_neg_escape}[1m")
set(_cat_neg_bold_red "${_cat_neg_escape}[1;31m")
set(_cat_neg_reset "${_cat_neg_escape}[0m")

set(_cat_neg_prepare_command
  "${cat_neg_cmake_command}" --build "${cat_neg_build_dir}" --target cat-impl)
if (cat_neg_config)
  list(APPEND _cat_neg_prepare_command --config "${cat_neg_config}")
endif()
execute_process(
  COMMAND ${_cat_neg_prepare_command}
  RESULT_VARIABLE _cat_neg_prepare_rc
  OUTPUT_VARIABLE _cat_neg_prepare_out
  ERROR_VARIABLE _cat_neg_prepare_err)
if (NOT _cat_neg_prepare_rc EQUAL 0)
  file(APPEND
    "${cat_neg_diag_path}"
    "==== cat-impl prepare failed, exit code ${_cat_neg_prepare_rc} ====\n"
    "stderr:\n${_cat_neg_prepare_err}\n"
    "stdout:\n${_cat_neg_prepare_out}\n")
  message(FATAL_ERROR
    "CAT_ARITHMETIC_NEGATIVE_CTEST: cat-impl failed before probes ran. See ${cat_neg_diag_path}")
endif()

list(LENGTH _cat_neg_targets _cat_neg_total)
file(REMOVE_RECURSE
  "${cat_neg_marker_dir}"
  "${cat_neg_case_diag_dir}")
file(REMOVE
  "${cat_neg_progress_path}"
  "${cat_neg_progress_path}.lock")
file(MAKE_DIRECTORY
  "${cat_neg_marker_dir}"
  "${cat_neg_case_diag_dir}")
file(GLOB _cat_neg_check_object_dirs
  "${cat_neg_build_dir}/CMakeFiles/cat-arithmetic-neg-check-*.dir")
if (_cat_neg_check_object_dirs)
  file(REMOVE_RECURSE ${_cat_neg_check_object_dirs})
endif()
message(STATUS
  "CAT_ARITHMETIC_NEGATIVE_CTEST: type-checking ${_cat_neg_total} ill-formed cases in parallel.")

set(_cat_neg_command
  "${cat_neg_cmake_command}" --build "${cat_neg_build_dir}" --target "${cat_neg_check_target}" --parallel)
if (cat_neg_config)
  list(APPEND _cat_neg_command --config "${cat_neg_config}")
endif()
execute_process(
  COMMAND ${_cat_neg_command}
  RESULT_VARIABLE _cat_neg_build_rc)
if (NOT _cat_neg_build_rc EQUAL 0)
  message(FATAL_ERROR
    "CAT_ARITHMETIC_NEGATIVE_CTEST: parallel check target failed before all probe results could be collected. See ${cat_neg_diag_path}")
endif()

set(_cat_neg_ok 0)
set(_cat_neg_unexpected_success)
foreach(_target IN LISTS _cat_neg_targets)
  set(_cat_neg_case_diag "${cat_neg_case_diag_dir}/${_target}.log")
  if (EXISTS "${_cat_neg_case_diag}")
    file(READ "${_cat_neg_case_diag}" _cat_neg_case_diag_text)
    file(APPEND "${cat_neg_diag_path}" "${_cat_neg_case_diag_text}\n")
  else()
    file(APPEND
      "${cat_neg_diag_path}"
      "==== ${_target} (missing launcher diagnostic) ====\n\n")
  endif()

  if (EXISTS "${cat_neg_marker_dir}/${_target}.txt")
    list(APPEND _cat_neg_unexpected_success "${_target}")
  else()
    math(EXPR _cat_neg_ok "${_cat_neg_ok} + 1")
  endif()
endforeach()

list(LENGTH _cat_neg_unexpected_success _cat_neg_unexpected_count)
if (_cat_neg_unexpected_count GREATER 0)
  set(_cat_neg_unexpected_targets)
  foreach(_cat_neg_unexpected_target IN LISTS _cat_neg_unexpected_success)
    string(APPEND
      _cat_neg_unexpected_targets
      "  ${_cat_neg_bold}${_cat_neg_unexpected_target}${_cat_neg_reset}\n")
  endforeach()
  message(FATAL_ERROR
    "CAT_ARITHMETIC_NEGATIVE_CTEST: ${_cat_neg_unexpected_count} probe target(s) compiled successfully. ${_cat_neg_bold_red}Expected type errors:${_cat_neg_reset}\n${_cat_neg_unexpected_targets}See ${cat_neg_diag_path}")
endif()
message(VERBOSE
  "CAT_ARITHMETIC_NEGATIVE_CTEST: ${_cat_neg_ok} build target(s) failed as expected")
]=])

add_test(
  NAME ArithmeticTypeCheck
  COMMAND
    "${CMAKE_COMMAND}"
    "-Dcat_neg_cmake_command=${CMAKE_COMMAND}"
    "-Dcat_neg_build_dir=${CMAKE_BINARY_DIR}"
    "-Dcat_neg_config=$<CONFIG>"
    "-Dcat_neg_diag_path=${CMAKE_BINARY_DIR}/CMakeFiles/cat_arithmetic_neg/build_diagnostics.log"
    "-Dcat_neg_marker_dir=${_cat_neg_marker_dir}"
    "-Dcat_neg_case_diag_dir=${_cat_neg_case_diag_dir}"
    "-Dcat_neg_progress_path=${_cat_neg_progress_path}"
    "-Dcat_neg_check_target=${_cat_neg_check_target}"
    -P "${_cat_neg_build_script}")
set_tests_properties(
  ArithmeticTypeCheck
  PROPERTIES
    LABELS "arithmetic")

  file(GLOB _cat_neg_glob
    "${CMAKE_BINARY_DIR}/CMakeFiles/cat_arithmetic_neg/*.cpp")
  list(LENGTH _cat_neg_glob _cat_neg_n_glob)
  if (NOT _cat_neg_n_glob EQUAL _cat_neg_n_cases)
    message(WARNING
      "CAT_BUILD_ARITHMETIC_NEGATIVE_TESTS: probe .cpp on disk count "
      "(${_cat_neg_n_glob}) != ill+must total (${_cat_neg_n_cases}) (glob check)")
  endif()
  message(VERBOSE
    "CAT_ARITHMETIC_NEGATIVE_CTEST: registered ${_cat_neg_n_cases} "
    "must-reject build target(s) as `ArithmeticTypeCheck`")
  if (CAT_BUILD_ARITHMETIC_NEGATIVE_TESTS)
    message(VERBOSE
      "CAT_BUILD_ARITHMETIC_NEGATIVE_TESTS: OK - compiler `${CMAKE_CXX_COMPILER}` "
      "(${_cat_neg_n_illformed_ok} must-reject) "
      "matches ${_cat_neg_n_glob} -fsyntax-only .cpp. "
      "Per-probe compiler output: ${CMAKE_BINARY_DIR}/CMakeFiles/cat_arithmetic_neg/compile_diagnostics.log")
  endif()
endblock()
