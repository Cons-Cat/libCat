# This file is flagrantly "vibe-coded". It may not be up to the standards of
# most libCat code.

# `cat-opt-report` -- Clang `-fsave-optimization-record` sweep.
#
# `cat-opt-report` is an OBJECT shadow of `cat-impl`. Building it recompiles
# `cat-impl`'s sources with `-fsave-optimization-record`, so Clang writes a
# `*.opt.yaml` next to every object file.
# `EXCLUDE_FROM_ALL` keeps it off the default build.
#
# An OBJECT library (rather than a custom target driving the compiler by hand)
# keeps CMake in charge of compile rules -- include dirs, definitions,
# `REUSE_FROM` PCH, the sccache launcher -- so the flag drops in with one line.
# It PRIVATE-links `cat` for the essentials and re-applies
# `CAT_COMPILE_OPTIONS_INTERNAL` because those are PRIVATE on `cat-impl` and do
# not propagate.
#
# The companion `opt-viewer.py` invocation is located at configure time and
# echoed as a STATUS message so the user has a ready command to render the YAML
# records into an HTML report. Post-build `TARGET` custom commands on OBJECT
# libraries require CMake `3.29;` the floor here is 3.24, so configure-time
# notice it is.
#
# Reference:
#   https://johnnysswlab.com/loop-optimizations-interpreting-the-compiler-optimization-report/

get_property(_cat_impl_sources_for_opt_report TARGET cat-impl PROPERTY SOURCES)

add_library(cat-opt-report OBJECT EXCLUDE_FROM_ALL
  ${_cat_impl_sources_for_opt_report})
target_link_libraries(cat-opt-report PRIVATE cat)
target_compile_options(cat-opt-report PRIVATE
  ${CAT_COMPILE_OPTIONS_INTERNAL}
  -fsave-optimization-record
)

# `-fsave-optimization-record` is a codegen flag and does not affect the PCH's
# frontend state, so reusing `cat-impl`'s PCH via `REUSE_FROM` is safe and cuts
# the PCH compile out of the report build.
if (CAT_PCH)
  target_precompile_headers(cat-opt-report REUSE_FROM cat-impl)
endif()

unset(_cat_impl_sources_for_opt_report)

# Prefer any `opt-viewer.py` already on PATH so an upgraded script can be
# substituted without touching CMake. Otherwise fall back to the script shipped
# under `/usr/lib/llvm-<major>/share/opt-viewer/` that matches the configured
# compiler version. Debian / Ubuntu's `llvm-<N>-tools` installs the script
# without the executable bit, so prefix `python3` when the resolved path ends in
# `.py`.
string(REGEX MATCH "^[0-9]+" _cat_opt_clang_major "${CMAKE_CXX_COMPILER_VERSION}")
find_program(CAT_OPT_VIEWER_PATH
  NAMES opt-viewer.py opt-viewer
  HINTS
    "/usr/lib/llvm-${_cat_opt_clang_major}/share/opt-viewer"
    "/usr/share/llvm-${_cat_opt_clang_major}/opt-viewer"
    "/usr/local/lib/llvm-${_cat_opt_clang_major}/share/opt-viewer"
  DOC "`opt-viewer.py` for rendering cat-opt-report YAML into HTML.")

if (CAT_OPT_VIEWER_PATH)
  if (CAT_OPT_VIEWER_PATH MATCHES "\\.py$")
    set(_cat_opt_viewer_cmd "python3 ${CAT_OPT_VIEWER_PATH}")
  else()
    set(_cat_opt_viewer_cmd "${CAT_OPT_VIEWER_PATH}")
  endif()
  message(STATUS
    "cat-opt-report: render with `${_cat_opt_viewer_cmd} ${CMAKE_BINARY_DIR}`")
  unset(_cat_opt_viewer_cmd)
else()
  message(STATUS
    "cat-opt-report: `opt-viewer.py` not found; install `llvm-${_cat_opt_clang_major}-tools` "
    "or put `opt-viewer.py` on PATH to render the YAML records.")
endif()
unset(_cat_opt_clang_major)
