# `cat-opt-report` -- Clang `-fsave-optimization-record` sweep
#
# Dual-mode file (`CMAKE_SCRIPT_MODE_FILE` is the discriminator):
#
# Module mode (`include()`): defines `cat-opt-report-lib` (an OBJECT
# shadow of `cat-impl`) plus the `cat-opt-report` custom target.
# Building the target recompiles `cat-impl`'s sources with
# `-fsave-optimization-record`, so Clang writes a `*.opt.yaml` next to
# every object file. `EXCLUDE_FROM_ALL` keeps the shadow off the default
# build.
#
# Script mode (`cmake -P`): locates an `opt-viewer.py` matching the
# configured Clang and prints the invocation needed to render the YAML
# records.
#
# An OBJECT library (rather than a custom target) keeps CMake in charge
# of compile rules -- include dirs, definitions, `REUSE_FROM` PCH, the
# sccache launcher -- so the flag drops in with one line. The shadow
# PRIVATE-links `cat` for the essentials and re-applies
# `CAT_COMPILE_OPTIONS_INTERNAL` because those are PRIVATE on `cat-impl`
# and do not propagate.
#
# Script-mode args:
#   CMAKE_ARGV3   `CMAKE_CXX_COMPILER_VERSION` (for fallback search)
#   CMAKE_ARGV4   build directory passed to `opt-viewer.py`
#
# Reference:
#   https://johnnysswlab.com/loop-optimizations-interpreting-the-compiler-optimization-report/

if (NOT CMAKE_SCRIPT_MODE_FILE)
  # ===== Module mode =====================================================
  set(_cat_opt_report_script "${CMAKE_CURRENT_LIST_FILE}")
  get_property(_cat_impl_sources TARGET cat-impl PROPERTY SOURCES)

  add_library(cat-opt-report-lib OBJECT EXCLUDE_FROM_ALL ${_cat_impl_sources})
  target_link_libraries(cat-opt-report-lib PRIVATE cat)
  target_compile_options(cat-opt-report-lib PRIVATE
    ${CAT_COMPILE_OPTIONS_INTERNAL}
    -fsave-optimization-record
  )

  # `-fsave-optimization-record` is a codegen flag and does not affect the
  # PCH's frontend state, so reusing `cat-impl`'s PCH via `REUSE_FROM` is
  # safe and cuts the PCH compile out of the report build.
  if (CAT_PCH)
    target_precompile_headers(cat-opt-report-lib REUSE_FROM cat-impl)
  endif()

  add_custom_target(
    cat-opt-report
    COMMAND
      ${CMAKE_COMMAND}
      -P
      ${_cat_opt_report_script}
      ${CMAKE_CXX_COMPILER_VERSION}
      ${CMAKE_BINARY_DIR}
    DEPENDS cat-opt-report-lib
    COMMENT "Built cat-opt-report-lib with -fsave-optimization-record yaml.")
  return()
endif()

# ===== Script mode =========================================================
if (CMAKE_ARGC LESS 5)
  message(FATAL_ERROR "cat-opt-report: expected compiler version and build dir.")
endif()

set(_cxx_version "${CMAKE_ARGV3}")
set(_build_dir   "${CMAKE_ARGV4}")

# Prefer any `opt-viewer.py` already on PATH so an upgraded script can be
# substituted without touching CMake. Otherwise fall back to the script shipped
# under `/usr/lib/llvm-<major>/share/opt-viewer/` that matches the configured
# compiler version.
find_program(_opt_viewer NAMES opt-viewer.py opt-viewer)

if (NOT _opt_viewer)
  string(REGEX MATCH "^[0-9]+" _clang_major "${_cxx_version}")
  if (_clang_major)
    find_program(_opt_viewer
      NAMES opt-viewer.py
      HINTS
        "/usr/lib/llvm-${_clang_major}/share/opt-viewer"
        "/usr/share/llvm-${_clang_major}/opt-viewer"
        "/usr/local/lib/llvm-${_clang_major}/share/opt-viewer"
      NO_DEFAULT_PATH)
  endif()
endif()

if (NOT _opt_viewer)
  message(FATAL_ERROR
    "cat-opt-report: could not find `opt-viewer.py`. Install the "
    "`llvm-<version>-tools` package matching `${_cxx_version}` or put "
    "`opt-viewer.py` on PATH.")
endif()

# The Debian and Ubuntu `llvm-<N>-tools` packages install the script without
# the executable bit, so always prefix a `.py` copy with `python3`.
if (_opt_viewer MATCHES "\\.py$")
  set(_command "python3 ${_opt_viewer}")
else()
  set(_command "${_opt_viewer}")
endif()

message(NOTICE "Open the report with: ${_command} ${_build_dir}")
