# This file is flagrantly "vibe-coded". It may not be up to the standards of
# most libCat code.

# Source tidying (`cat-tidy`, `cat-tidy-check`).
#
# Dual-mode file (`CMAKE_SCRIPT_MODE_FILE` is the discriminator):
#
# Module mode (`include()`): locates a `clang-tidy` whose major matches the
# configured Clang, locates the companion `run-clang-tidy` parallel driver,
# gathers libCat's translation units from `cat`/`cat-impl`, and registers the
# `cat-tidy` (apply fix-its) and `cat-tidy-check` (read-only,
# warnings-as-errors) custom targets.
# Both re-invoke this same file in script mode at build time.
#
# Script mode (`cmake -P`): drives `run-clang-tidy` over the listed files
# against the build tree's `compile_commands.json`. `APPLY` adds `-fix -format`
# so fix-its land in place and `clang-format` cleans up afterwards. `CHECK` adds
# `-warnings-as-errors=*`, which makes `run-clang-tidy` exit non-zero on any
# diagnostic so CI fails.
#
# `clang-tidy` reads the compile database, so this file forces
# `CMAKE_EXPORT_COMPILE_COMMANDS=ON`.
#
# Script-mode args:
#   CMAKE_ARGV3      mode, `APPLY` or `CHECK`
#   CMAKE_ARGV4      `run-clang-tidy` driver
#   CMAKE_ARGV5      `clang-tidy` binary (passed via -clang-tidy-binary)
#   CMAKE_ARGV6      build dir (holds `compile_commands.json`)
#   CMAKE_ARGV7+     source paths to tidy (also used for regex filter)

if (NOT CMAKE_SCRIPT_MODE_FILE)
  # Module mode:
  # `cat-tidy` requires a `clang-tidy` whose major matches the C++ compiler
  # CMake selected, so the linter and the compiler stay in lockstep with
  # libCat's `.clang-tidy` rules. Prefer the explicitly versioned binary, and
  # fall back to an unversioned `clang-tidy` only if it actually reports the
  # same major.
  string(REGEX MATCH "^[0-9]+" _cat_ct_major "${CMAKE_CXX_COMPILER_VERSION}")
  find_program(CAT_CLANG_TIDY_PATH
    NAMES "clang-tidy-${_cat_ct_major}" clang-tidy
    DOC "`clang-tidy` binary to use (must match Clang ${_cat_ct_major}).")
  find_program(CAT_RUN_CLANG_TIDY_PATH
    NAMES "run-clang-tidy-${_cat_ct_major}" run-clang-tidy
    DOC "`run-clang-tidy` driver for parallel `cat-tidy` runs.")

  if (CAT_CLANG_TIDY_PATH)
    execute_process(
      COMMAND "${CAT_CLANG_TIDY_PATH}" --version
      OUTPUT_VARIABLE  _cat_ct_version
      RESULT_VARIABLE  _cat_ct_result
      OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_QUIET)
    string(REGEX MATCH "version ([0-9]+)" _cat_ct_drop "${_cat_ct_version}")
    if (NOT _cat_ct_result EQUAL 0 OR NOT CMAKE_MATCH_1 STREQUAL _cat_ct_major)
      message(WARNING
        "Found `${CAT_CLANG_TIDY_PATH}` but `--version` reports "
        "`${_cat_ct_version}`. libCat requires clang-tidy "
        "${_cat_ct_major} to match `${CMAKE_CXX_COMPILER}`. Install "
        "`clang-tidy-${_cat_ct_major}` or configure with "
        "`-DCAT_CLANG_TIDY_PATH=/path/to/clang-tidy-${_cat_ct_major}`. "
        "`cat-tidy` and `cat-tidy-check` will exit non-zero until this "
        "is resolved.")
      unset(CAT_CLANG_TIDY_PATH CACHE)
    else()
      message(VERBOSE
        "clang-tidy: ${CAT_CLANG_TIDY_PATH} (version ${_cat_ct_major})")
    endif()
    unset(_cat_ct_version)
    unset(_cat_ct_result)
    unset(_cat_ct_drop)
  endif()

  # `run-clang-tidy` parallelizes per-TU `clang-tidy` runs and merges fix-its
  # via `clang-apply-replacements`. Both scripts ship with the same clang-tools
  # installation as the matching `clang-tidy`.
  if (CAT_CLANG_TIDY_PATH AND NOT CAT_RUN_CLANG_TIDY_PATH)
    message(WARNING
      "`clang-tidy-${_cat_ct_major}` was found but "
      "`run-clang-tidy-${_cat_ct_major}` was not. Install "
      "the matching clang-tools package or configure with "
      "`-DCAT_RUN_CLANG_TIDY_PATH=/path/to/run-clang-tidy-${_cat_ct_major}`. "
      "`cat-tidy` and `cat-tidy-check` will exit non-zero until this "
      "is resolved.")
  endif()

  # `clang-tidy` reads `compile_commands.json` from the build dir, so the tidy
  # targets are only useful if CMake emits that file. Force it on for this
  # configure -- cheap and idempotent, and downstream tooling like clangd wants
  # it anyway.
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

  # `cat-impl` owns libCat's `.cpp`s; `_start.cpp` rides on `cat`'s
  # `INTERFACE_SOURCES` for its per-consumer `NO_ARGC_ARGV` branch.
  # Both lists feed `compile_commands.json`, but `INTERFACE_SOURCES` still
  # carries `$<BUILD_INTERFACE:...>`/`$<INSTALL_INTERFACE:...>` generator
  # expressions -- the empty `INSTALL_INTERFACE` result is skipped in script
  # mode. Drop non-TUs (`.hpp`/`.tpp`) since `clang-tidy` only runs against
  # compile-database entries.
  get_property(_cat_impl_sources  TARGET cat-impl PROPERTY SOURCES)
  get_property(_cat_iface_sources TARGET cat      PROPERTY INTERFACE_SOURCES)
  set(_cat_tidy_files
    ${_cat_impl_sources}
    ${_cat_iface_sources})
  list(FILTER _cat_tidy_files INCLUDE REGEX "\\.(cpp|cxx|cc|c)(>|$)")

  # Capture this file's path so the custom target can re-invoke it via
  # `cmake -P` (script mode) at build time.
  set(_cat_tidy_script "${CMAKE_CURRENT_LIST_FILE}")

  # `cat-tidy` applies fix-its in place (and reformats). `cat-tidy-check` is the
  # read-only CI variant that upgrades every diagnostic to an error and exits
  # non-zero on drift. Both share this script, distinguished by the leading mode
  # argument.
  #
  # Depending on `cat-impl` ensures its PCH (`cmake_pch.hxx.pch`) has been built
  # before tidy runs. The compile-database entries for every `cat-impl` source
  # (and for `_start.cpp` under `cat-syntax-lib`/`cat-opt-report`, which
  # `REUSE_FROM cat-impl`) name that PCH explicitly, so without it `clang-tidy`
  # fails with "PCH file not found" before it can analyse anything. sccache /
  # ninja keep the rebuild nearly free on incremental runs.
  function(_cat_add_tidy_target target mode)
    if (CAT_CLANG_TIDY_PATH AND CAT_RUN_CLANG_TIDY_PATH)
      add_custom_target(
        ${target}
        COMMAND
          ${CMAKE_COMMAND}
          -P
          ${_cat_tidy_script}
          ${mode}
          ${CAT_RUN_CLANG_TIDY_PATH}
          ${CAT_CLANG_TIDY_PATH}
          ${CMAKE_BINARY_DIR}
          ${_cat_tidy_files}
        DEPENDS ${_cat_tidy_files}
        USES_TERMINAL)
      add_dependencies(${target} cat-impl)
    elseif (NOT CAT_CLANG_TIDY_PATH)
      add_custom_target(
        ${target}
        COMMAND
          ${CMAKE_COMMAND}
          -E
          echo
          "${target}: cmake did not find clang-tidy ${_cat_ct_major}. Install clang-tidy-${_cat_ct_major} or configure with -DCAT_CLANG_TIDY_PATH=/path/to/clang-tidy-${_cat_ct_major}"
        COMMAND ${CMAKE_COMMAND} -E false)
    else()
      add_custom_target(
        ${target}
        COMMAND
          ${CMAKE_COMMAND}
          -E
          echo
          "${target}: cmake did not find run-clang-tidy ${_cat_ct_major}. Install clang-tools-${_cat_ct_major} or configure with -DCAT_RUN_CLANG_TIDY_PATH=/path/to/run-clang-tidy-${_cat_ct_major}"
        COMMAND ${CMAKE_COMMAND} -E false)
    endif()
  endfunction()

  _cat_add_tidy_target(cat-tidy       APPLY)
  _cat_add_tidy_target(cat-tidy-check CHECK)
  unset(_cat_ct_major)
  return()
endif()

# Script mode:
if (CMAKE_ARGC LESS 8)
  message(FATAL_ERROR
    "cat-tidy: expected mode, run-clang-tidy, clang-tidy, build dir, and at "
    "least one file.")
endif()

set(_mode            "${CMAKE_ARGV3}")
set(_run_clang_tidy  "${CMAKE_ARGV4}")
set(_clang_tidy      "${CMAKE_ARGV5}")
set(_build_dir       "${CMAKE_ARGV6}")

if (NOT (_mode STREQUAL "APPLY" OR _mode STREQUAL "CHECK"))
  message(FATAL_ERROR "cat-tidy: unknown mode `${_mode}`, expected APPLY or CHECK.")
endif()

if (NOT EXISTS "${_build_dir}/compile_commands.json")
  message(FATAL_ERROR
    "cat-tidy: `${_build_dir}/compile_commands.json` not found. The tidy "
    "target forces `CMAKE_EXPORT_COMPILE_COMMANDS=ON` at configure time, "
    "so re-run `cmake -B ${_build_dir}` to regenerate it.")
endif()

# Collect file arguments, dropping empties (from `INSTALL_INTERFACE` generator
# expressions that evaluate to nothing at build time).
set(_files "")
set(_i 7)
while (_i LESS CMAKE_ARGC)
  set(_file "${CMAKE_ARGV${_i}}")
  math(EXPR _i "${_i} + 1")
  if (NOT _file STREQUAL "")
    list(APPEND _files "${_file}")
  endif()
endwhile()

if (NOT _files)
  message(FATAL_ERROR "cat-tidy: no input files after filtering.")
endif()

# `run-clang-tidy` treats trailing positional args as a regex-or'd filter over
# `compile_commands.json`. Escape regex metacharacters in each path and join
# with | so a single match term holds every file.
set(_escaped_files "")
foreach (_f IN LISTS _files)
  string(REGEX REPLACE "([][(){}.*+?^$|\\\\])" "\\\\\\1" _escaped "${_f}")
  list(APPEND _escaped_files "^${_escaped}$")
endforeach()
list(JOIN _escaped_files "|" _file_regex)

set(_tidy_args
  "-p" "${_build_dir}"
  "-clang-tidy-binary" "${_clang_tidy}"
  "-quiet")

if (_mode STREQUAL "APPLY")
  list(APPEND _tidy_args "-fix" "-format")
else()
  list(APPEND _tidy_args "-warnings-as-errors=*")
endif()

execute_process(
  COMMAND "${_run_clang_tidy}" ${_tidy_args} "${_file_regex}"
  RESULT_VARIABLE _tidy_result)

if (NOT _tidy_result EQUAL 0)
  if (_mode STREQUAL "CHECK")
    message(FATAL_ERROR
      "cat-tidy-check: diagnostics reported (exit ${_tidy_result}). "
      "Run `just tidy` to apply fix-its!")
  else()
    message(FATAL_ERROR
      "cat-tidy: `run-clang-tidy` exited with ${_tidy_result}.")
  endif()
endif()
