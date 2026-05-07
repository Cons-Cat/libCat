# This file is flagrantly "vibe-coded". It may not be up to the standards of
# most libCat code.

# `cat-ir` `.ll` normaliser. Sole consumer is `cat_ir_ll.cmake`'s per-TU
# custom command; do not invoke directly.
#
# Strips the host-specific noise that `llvm-dis` / `llvm-extract -S` /
# `opt -S` emit by default so the `.ll` output is reproducible across
# machines and toolchain rebuilds:
#
#   1. `; ModuleID = '<abs build path>'` -- comment with no semantic value.
#   2. `source_filename = "<abs source path>"` -- collapsed to project-
#      relative when possible, dropped to the basename otherwise.
#   3. `!llvm.ident = !{!N}` plus its referenced `!N = !{!"clang version
#      ..."}` definition -- toolchain version string.
#
# Run via `cmake -P scripts/cat_ir_normalize.cmake -DIN=... -DOUT=...
#                                                  -DPROJECT_ROOT=...`.

cmake_minimum_required(VERSION 3.25)

include("${CMAKE_CURRENT_LIST_DIR}/cat_ir_match.cmake")

foreach(_v IN ITEMS IN OUT PROJECT_ROOT)
  if (NOT DEFINED ${_v} OR "${${_v}}" STREQUAL "")
    message(FATAL_ERROR "cat-ir-normalize: missing required arg -D${_v}=...")
  endif()
endforeach()

file(READ "${IN}" _text)

# 1. ModuleID is a build-tree path; just drop it.
string(REGEX REPLACE "; ModuleID = '[^'\n]*'\n" "" _text "${_text}")

# 2. source_filename: trim the project-root prefix when present so what
# remains is `src/libraries/.../foo.cpp`. If the prefix doesn't match
# (out-of-tree TU, INTERFACE_SOURCES quirk, ...) the line stays untouched.
_cat_ir_regex_escape("${PROJECT_ROOT}/" _root_re)
string(REGEX REPLACE "(source_filename = \")${_root_re}" "\\1" _text "${_text}")

# 3. Drop the `!llvm.ident` named-metadata line and any anonymous metadata
# whose value is a clang version string. The metadata-id pruning is
# deliberately content-based (matching the `clang version` literal) rather
# than chasing the `!N` from the `!llvm.ident` line, so we cope with the
# numbering shifting between rebuilds.
string(REGEX REPLACE "!llvm\\.ident = !\\{[^}\n]*\\}\n" "" _text "${_text}")
string(REGEX REPLACE "![0-9]+ = !\\{!\"[^\"\n]*clang version [^\"\n]*\"\\}\n"
       "" _text "${_text}")

file(WRITE "${OUT}" "${_text}")
