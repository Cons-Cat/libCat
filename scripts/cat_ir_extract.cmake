# This file is flagrantly "vibe-coded". It may not be up to the standards of
# most libCat code.

# `cat-ir` `bc` / `ll` extraction driver. Sole consumer is `cat_ir.cmake`'s
# per-TU custom command; do not invoke directly.
#
# Run via `cmake -P scripts/cat_ir_extract.cmake -DBC=... -DOUT=... -D...`.
#
# `llvm-extract` matches by mangled name only, so we pre-walk the bitcode
# with `llvm-nm | llvm-cxxfilt`, regex-match the demangled forms (see
# `cat_ir_match.cmake`), and feed the matched mangled names back as
# `--func=` flags. A single namespace-qualified `fn=cat::pow` therefore
# collects every overload, every template specialization, and any LTO
# clones (`.cold`, `.0`) without forcing the user to spell out a regex.
#
# Required args (-D):
#   BC, NM, CXXFILT, EXTRACT, OUT, FN
# Optional args (-D):
#   OUT_TEXT=ON   pass `-S` to llvm-extract so OUT is textual `.ll`.

cmake_minimum_required(VERSION 3.25)

include("${CMAKE_CURRENT_LIST_DIR}/cat_ir_match.cmake")

foreach(_v IN ITEMS BC NM CXXFILT EXTRACT OUT FN)
  if (NOT DEFINED ${_v} OR "${${_v}}" STREQUAL "")
    message(FATAL_ERROR "cat-ir: missing required arg -D${_v}=...")
  endif()
endforeach()

_cat_ir_match_mangled("${NM}" "${CXXFILT}" "${BC}" "${FN}" _matched)
if (_matched STREQUAL "")
  message(FATAL_ERROR
    "cat-ir: no defined symbols in ${BC} match '${FN}'. The TU may not "
    "instantiate this function (templates only emit per-TU when a TU "
    "instantiates them). The symbol may have been dead-code-eliminated. "
    "The name may be in an anonymous namespace, which `fn=` can't "
    "address. The name may require additional namespace qualification
    (e.g. `fn=foo::${FN}`). `bc`/`ll` without `fn=` dumps the TU's full 
    set of definitions for inspection.")
    
endif()

set(_func_args "")
foreach(_m IN LISTS _matched)
  list(APPEND _func_args "--func=${_m}")
endforeach()

cmake_path(GET OUT PARENT_PATH _out_dir)
if (_out_dir AND NOT EXISTS "${_out_dir}")
  file(MAKE_DIRECTORY "${_out_dir}")
endif()

set(_extra "")
if (OUT_TEXT)
  list(APPEND _extra -S)
endif()

execute_process(
  COMMAND "${EXTRACT}" ${_extra} ${_func_args} "${BC}" -o "${OUT}"
  COMMAND_ERROR_IS_FATAL ANY)
