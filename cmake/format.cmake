# Source formatting (`cat-format`, `cat-format-check`)
#
# Dual-mode file (`CMAKE_SCRIPT_MODE_FILE` is the discriminator):
#
# Module mode (`include()`): locates a `clang-format` whose major
# matches the configured Clang, gathers libCat's sources and public
# headers from the `cat` / `cat-impl` targets, and registers the
# `cat-format` (in-place rewrite) and `cat-format-check` (read-only,
# non-zero exit on drift) custom targets. Both re-invoke this same file
# in script mode at build time.
#
# Script mode (`cmake -P`): pipes each listed file through
# `clang-format`. `APPLY` rewrites mismatches in place. `CHECK` reports
# every file that would be reformatted and exits non-zero so CI fails.
#
# Script-mode args:
#   CMAKE_ARGV3      mode, `APPLY` or `CHECK`
#   CMAKE_ARGV4      `clang-format` executable (CAT_CLANG_FORMAT_PATH)
#   CMAKE_ARGV5+     source paths to format / check

if (NOT CMAKE_SCRIPT_MODE_FILE)
  # ===== Module mode =====================================================
  # `cat-format` requires a `clang-format` whose major matches the C++
  # compiler CMake selected, so the formatter and the compiler stay in
  # lockstep with libCat's `.clang-format` rules. Prefer the explicitly
  # versioned binary, and fall back to an unversioned `clang-format` only
  # if it actually reports the same major.
  string(REGEX MATCH "^[0-9]+" _cat_cf_major "${CMAKE_CXX_COMPILER_VERSION}")
  find_program(CAT_CLANG_FORMAT_PATH
    NAMES "clang-format-${_cat_cf_major}" clang-format
    DOC "`clang-format` binary to use (must match Clang ${_cat_cf_major}).")

  if (CAT_CLANG_FORMAT_PATH)
    execute_process(
      COMMAND "${CAT_CLANG_FORMAT_PATH}" --version
      OUTPUT_VARIABLE  _cat_cf_version
      RESULT_VARIABLE  _cat_cf_result
      OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_QUIET)
    string(REGEX MATCH "version ([0-9]+)" _cat_cf_drop "${_cat_cf_version}")
    if (NOT _cat_cf_result EQUAL 0 OR NOT CMAKE_MATCH_1 STREQUAL _cat_cf_major)
      message(WARNING
        "Found `${CAT_CLANG_FORMAT_PATH}` but `--version` reports "
        "`${_cat_cf_version}`. libCat requires clang-format "
        "${_cat_cf_major} to match `${CMAKE_CXX_COMPILER}`. Install "
        "`clang-format-${_cat_cf_major}` or configure with "
        "`-DCAT_CLANG_FORMAT_PATH=/path/to/clang-format-${_cat_cf_major}`. "
        "`cat-format` and `cat-format-check` will exit non-zero until this "
        "is resolved.")
      unset(CAT_CLANG_FORMAT_PATH CACHE)
    else()
      message(STATUS
        "clang-format: ${CAT_CLANG_FORMAT_PATH} (version ${_cat_cf_major})")
    endif()
    unset(_cat_cf_version)
    unset(_cat_cf_result)
    unset(_cat_cf_drop)
  endif()

  # `cat-impl` owns most of libCat's `.cpp`s (they compile into `libcat.a`)
  # while `_start.cpp` remains on `cat`'s `INTERFACE_SOURCES`. The format
  # target wants both lists plus the public header set assembled in
  # `src/CMakeLists.txt`.
  get_property(_cat_impl_sources  TARGET cat-impl PROPERTY SOURCES)
  get_property(_cat_iface_sources TARGET cat      PROPERTY INTERFACE_SOURCES)
  set(_cat_format_files
    ${_cat_impl_sources}
    ${_cat_iface_sources}
    ${CAT_HEADER_FILES})

  # Capture this file's path so the custom target can re-invoke it via
  # `cmake -P` (script mode) at build time.
  set(_cat_format_script "${CMAKE_CURRENT_LIST_FILE}")

  # `cat-format` rewrites mismatches in place. `cat-format-check` is the
  # read-only CI variant that reports every file that would be reformatted
  # and exits non-zero so the build fails on drift. Both share this
  # script, distinguished by the leading mode argument.
  function(_cat_add_format_target target mode)
    if (CAT_CLANG_FORMAT_PATH)
      add_custom_target(
        ${target}
        COMMAND
          ${CMAKE_COMMAND}
          -P
          ${_cat_format_script}
          ${mode}
          ${CAT_CLANG_FORMAT_PATH}
          ${_cat_format_files}
        DEPENDS ${_cat_format_files})
    else()
      add_custom_target(
        ${target}
        COMMAND
          ${CMAKE_COMMAND}
          -E
          echo
          "${target}: cmake did not find clang-format ${_cat_cf_major}. Install clang-format-${_cat_cf_major} or configure with -DCAT_CLANG_FORMAT_PATH=/path/to/clang-format-${_cat_cf_major}"
        COMMAND ${CMAKE_COMMAND} -E false)
    endif()
  endfunction()

  _cat_add_format_target(cat-format       APPLY)
  _cat_add_format_target(cat-format-check CHECK)
  unset(_cat_cf_major)
  return()
endif()

# ===== Script mode =========================================================
if (CMAKE_ARGC LESS 6)
  message(FATAL_ERROR
    "cat-format: expected mode, clang-format path, and at least one file.")
endif()

set(_mode         "${CMAKE_ARGV3}")
set(_clang_format "${CMAKE_ARGV4}")
set(_compare_tmp  "${CMAKE_CURRENT_LIST_DIR}/.cat-format-compare.tmp")

if (NOT (_mode STREQUAL "APPLY" OR _mode STREQUAL "CHECK"))
  message(FATAL_ERROR "cat-format: unknown mode `${_mode}`, expected APPLY or CHECK.")
endif()

set(_mismatches "")

set(_i 5)
while (_i LESS CMAKE_ARGC)
  set(_file "${CMAKE_ARGV${_i}}")
  math(EXPR _i "${_i} + 1")

  if (_file STREQUAL "")
    continue()
  endif()

  execute_process(
    COMMAND "${_clang_format}" "${_file}"
    OUTPUT_FILE "${_compare_tmp}"
    RESULT_VARIABLE _fmt_out_result
    ERROR_VARIABLE _fmt_out_stderr)
  if (NOT _fmt_out_result EQUAL 0)
    message(FATAL_ERROR "cat-format: clang-format failed for `${_file}`: ${_fmt_out_stderr}")
  endif()

  execute_process(
    COMMAND "${CMAKE_COMMAND}" -E compare_files "${_file}" "${_compare_tmp}"
    RESULT_VARIABLE _diff
    OUTPUT_QUIET
    ERROR_QUIET)

  if (_diff EQUAL 1)
    list(APPEND _mismatches "${_file}")
    if (_mode STREQUAL "APPLY")
      message(STATUS "formatted: ${_file}")
      execute_process(
        COMMAND "${_clang_format}" -i "${_file}"
        RESULT_VARIABLE _fmt_in_result
        ERROR_VARIABLE _fmt_in_stderr)
      if (NOT _fmt_in_result EQUAL 0)
        message(FATAL_ERROR "cat-format: clang-format -i failed for `${_file}`: ${_fmt_in_stderr}")
      endif()
    else()
      message(STATUS "would reformat: ${_file}")
    endif()
  elseif (_diff EQUAL 2)
    message(FATAL_ERROR "cat-format: could not compare `${_file}` (missing file or read error).")
  endif()
endwhile()

file(REMOVE "${_compare_tmp}")

# `CHECK` must surface a non-zero exit status so CI fails. Collecting all
# mismatches first (instead of aborting on the first one) reports every file
# in a single pass, which is more useful when scanning a dirty tree.
if (_mode STREQUAL "CHECK" AND _mismatches)
  list(LENGTH _mismatches _count)
  message(FATAL_ERROR
    "cat-format-check: ${_count} file(s) would be reformatted. "
    "Run `cmake --build <build> --target cat-format` to fix.")
endif()
