# This file is flagrantly "vibe-coded". It may not be up to the standards of
# most libCat code.

# `cat-ir` post-build path summary. Sole consumer is the `ir` recipe in the
# justfile; do not invoke directly.
#
# Run via `cmake -P scripts/cat_ir_paths.cmake -DBUILD_ROOT=... -D...`.
#
# Args (-D):
#   BUILD_ROOT   build directory, relative or absolute.
#   CONFIGS      semicolon-separated configs that were built (e.g.
#                "Release", "Debug;Release;RelWithDebInfo").
#   SELECTORS    semicolon-separated selectors as the user typed them
#                (basenames or domain meta names src/tests/examples/full).
#   KINDS        semicolon-separated IR kinds (ii;bc;ll;s).
#   FN           optional literal symbol name (e.g. `cat::pow`); the
#                sanitised form is matched against per-TU output filenames.
#                Re-uses the same regex as `cmake/cat_ir.cmake` so the
#                emitted filenames stay in lockstep with what was actually
#                written.
#
# Per-domain selectors print the directory; per-TU selectors print each
# generated file's path. Existence-checked so we never report a path that
# isn't on disk.

cmake_minimum_required(VERSION 3.25)

foreach(_v IN ITEMS BUILD_ROOT CONFIGS SELECTORS KINDS)
  if (NOT DEFINED ${_v} OR "${${_v}}" STREQUAL "")
    message(FATAL_ERROR "cat-ir-paths: missing required arg -D${_v}=...")
  endif()
endforeach()

# Resolve BUILD_ROOT to absolute so subsequent globs and IS_DIRECTORY
# probes are independent of cwd.
cmake_path(ABSOLUTE_PATH BUILD_ROOT NORMALIZE OUTPUT_VARIABLE _root)

# Mirror cat_ir.cmake's func-suffix sanitisation: collapse runs of
# non-`[A-Za-z0-9._-]` to a single `_`, strip leading/trailing `_`,
# prepend `-`. Touching `cat_ir.cmake`'s recipe means touching this
# regex as well.
set(_sfx "")
if (DEFINED FN AND NOT FN STREQUAL "")
  string(REGEX REPLACE "[^A-Za-z0-9._-]+" "_" _sfx "${FN}")
  string(REGEX REPLACE "^_+|_+$" "" _sfx "${_sfx}")
  if (_sfx)
    set(_sfx "-${_sfx}")
  endif()
endif()

# Walk every `<root>/<lib>/<config>/` dir and collect those that hold at
# least one `*.<ext>` file. Tests / examples / CMakeFiles have their own
# subtrees and own selectors, so they're skipped here.
function(_list_src_dirs cfg ext out_var)
  set(_result "")
  file(GLOB _candidates LIST_DIRECTORIES true "${_root}/*/${cfg}")
  foreach(_d IN LISTS _candidates)
    if (NOT IS_DIRECTORY "${_d}")
      continue()
    endif()
    cmake_path(GET _d PARENT_PATH _parent)
    cmake_path(GET _parent FILENAME _libname)
    if (_libname MATCHES "^(tests|examples|CMakeFiles|cat-ir-fn)$")
      continue()
    endif()
    file(GLOB _files "${_d}/*.${ext}")
    if (_files)
      list(APPEND _result "${_d}/")
    endif()
  endforeach()
  set(${out_var} "${_result}" PARENT_SCOPE)
endfunction()

set(_paths "")
foreach(_cfg IN LISTS CONFIGS)
  foreach(_sel IN LISTS SELECTORS)
    foreach(_kind IN LISTS KINDS)
      if (_sel STREQUAL "tests" OR _sel STREQUAL "examples")
        if (IS_DIRECTORY "${_root}/${_sel}/${_cfg}")
          list(APPEND _paths "${_root}/${_sel}/${_cfg}/")
        endif()
      elseif (_sel STREQUAL "src")
        _list_src_dirs("${_cfg}" "${_kind}" _src_dirs)
        list(APPEND _paths ${_src_dirs})
      elseif (_sel STREQUAL "full")
        if (IS_DIRECTORY "${_root}/tests/${_cfg}")
          list(APPEND _paths "${_root}/tests/${_cfg}/")
        endif()
        if (IS_DIRECTORY "${_root}/examples/${_cfg}")
          list(APPEND _paths "${_root}/examples/${_cfg}/")
        endif()
        _list_src_dirs("${_cfg}" "${_kind}" _src_dirs)
        list(APPEND _paths ${_src_dirs})
      else()
        # Per-TU basename: glob across all lib subdirs (impl basenames are
        # unique across libCat, so this hits one file).
        file(GLOB _matches
          "${_root}/*/${_cfg}/${_sel}${_sfx}.${_kind}")
        list(APPEND _paths ${_matches})
      endif()
    endforeach()
  endforeach()
endforeach()

list(REMOVE_DUPLICATES _paths)

# Re-relativise to the just recipe's cwd (project root) so output matches
# what the user typed at the command line. Plain prefix-strip preserves the
# trailing slash on directory entries that `cmake_path(RELATIVE_PATH)` would
# otherwise normalise away.
# `cmake -E echo` writes to stdout, which `cmake -P` inherits from the parent
# `just` process; `message(NOTICE ...)` would land on stderr instead.
file(REAL_PATH "." _pwd)
set(_prefix "${_pwd}/")
string(LENGTH "${_prefix}" _prefix_len)
foreach(_p IN LISTS _paths)
  string(LENGTH "${_p}" _p_len)
  set(_rel "${_p}")
  if (_p_len GREATER_EQUAL _prefix_len)
    string(SUBSTRING "${_p}" 0 ${_prefix_len} _head)
    if (_head STREQUAL "${_prefix}")
      string(SUBSTRING "${_p}" ${_prefix_len} -1 _rel)
    endif()
  endif()
  execute_process(COMMAND "${CMAKE_COMMAND}" -E echo "${_rel}")
endforeach()
