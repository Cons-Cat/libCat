# This file is flagrantly "vibe-coded". It may not be up to the standards of
# most libCat code.

# `cat-ir` `fn=` pre-flight check. Sole consumer is `cat_ir.cmake`'s shared
# pre-flight rules under LTO + `fn=`; do not invoke directly.
#
# Run via `cmake -P scripts/cat_ir_fn_validate.cmake -D...`.
#
# Args (-D):
#   OBJDUMP   `llvm-objdump` binary.
#   FN        literal symbol name to match.
#   BINS      semicolon-separated candidate binary paths.
#   DOMAIN    "src" / "tests" / "examples" (used in the failure message).
#   STAMP     path to write on success; the rule's `OUTPUT`.
#
# On failure (no symbol matches in any binary) errors loudly, collapsing
# what would otherwise have been N duplicate per-TU disasm-script errors
# into a single domain-level diagnostic.

cmake_minimum_required(VERSION 3.25)

include("${CMAKE_CURRENT_LIST_DIR}/cat_ir_match.cmake")

foreach(_v IN ITEMS OBJDUMP FN BINS DOMAIN STAMP)
  if (NOT DEFINED ${_v} OR "${${_v}}" STREQUAL "")
    message(FATAL_ERROR "cat-ir-fn-validate: missing required arg -D${_v}=...")
  endif()
endforeach()

set(_any_match FALSE)
foreach(_b IN LISTS BINS)
  if (NOT EXISTS "${_b}")
    continue()
  endif()
  _cat_ir_match_addrs("${OBJDUMP}" "${_b}" "${FN}" _addrs _sizes _sects)
  list(LENGTH _addrs _n)
  if (_n GREATER 0)
    set(_any_match TRUE)
    break()
  endif()
endforeach()

if (NOT _any_match)
  message(FATAL_ERROR
    # TODO: This error family should detect when namespace qualification is needed.
    "cat-ir: no defined symbols match '${FN}' in any ${DOMAIN} candidate "
    "binary. The symbol may have been dead-code-eliminated, `${FN}` may "
    "have a typo, or the name may be in an anonymous namespace (which "
    "`fn=` can't address). The name may require additional namespace qualification "
    "(e.g. `fn=foo::${FN}`).")
endif()

cmake_path(GET STAMP PARENT_PATH _stamp_dir)
if (_stamp_dir AND NOT EXISTS "${_stamp_dir}")
  file(MAKE_DIRECTORY "${_stamp_dir}")
endif()
file(WRITE "${STAMP}" "ok\n")
