# This file is flagrantly "vibe-coded". It may not be up to the standards of
# most libCat code.

# `cat-ir` `.s` driver. Sole consumer is `cat_ir.cmake`'s per-TU custom
# command; do not invoke directly.
#
# Run via `cmake -P scripts/cat_ir_disasm.cmake -DSHADOW_OBJ=... -D...`.
#
# Branches on the magic bytes of the shadow `.cpp.o`:
#
#   - ELF (non-LTO build): disassemble the shadow `.cpp.o` directly. With
#     `FN`, scope to symbols matching the pattern via
#     `--disassemble-symbols=`.
#
#   - LLVM bitcode (LTO build): the shadow has no native code; codegen
#     happened at link time. Disassemble the listed `CANDIDATE_BINS`
#     instead.
#       - `DOMAIN=tests` / `DOMAIN=examples` + no `FN`: dump the whole
#         owner binary (the user's `tests/foo.cpp` or example exec is the
#         natural per-TU scope already).
#       - `DOMAIN=src` + no `FN`: hard-fail with an actionable message.
#         Without DWARF (we don't compile with `-g`) we can't recover
#         per-TU origin.
#       - With `FN`: scope each candidate to symbols matching `FN` and
#         concatenate. See `cat_ir_match.cmake` for the matching rules.
#
# Boilerplate (`<file>: file format`, `Disassembly of section ...`) and
# trailing `int3` trap-fill runs are stripped post-objdump so the `.s`
# opens directly on the first `<addr> <symbol>:` block.

cmake_minimum_required(VERSION 3.25)

include("${CMAKE_CURRENT_LIST_DIR}/cat_ir_match.cmake")

foreach(_v IN ITEMS SHADOW_OBJ SOURCE_NAME DOMAIN SYNTAX OBJDUMP OUT)
  if (NOT DEFINED ${_v} OR "${${_v}}" STREQUAL "")
    message(FATAL_ERROR "cat-ir: missing required arg -D${_v}=...")
  endif()
endforeach()

# ELF magic; everything else (raw `BC\xC0\xDE`, wrapped `\xDE\xC0\x17\x0B`)
# is treated as bitcode.
file(READ "${SHADOW_OBJ}" _magic LIMIT 4 HEX)
set(_is_elf FALSE)
if (_magic STREQUAL "7f454c46")
  set(_is_elf TRUE)
endif()

set(_bins "")
if (_is_elf)
  list(APPEND _bins "${SHADOW_OBJ}")
else()
  if (DOMAIN STREQUAL "src" AND NOT FN)
    message(FATAL_ERROR
      "cat-ir: ${SOURCE_NAME} is part of an LTO build, so per-TU "
      "assembly cannot be recovered without `fn=<name>`. Either:\n"
      "  - re-run with `fn=<namespace::name>` to scope to specific symbols, or\n"
      "  - inspect `bc` / `ll` for the pre-LTO bitcode of this TU, or\n"
      "  - inspect a `tests/` or `examples/` selector instead, where the\n"
      "    owner binary defines the scope implicitly.")
  endif()
  foreach(_c IN LISTS CANDIDATE_BINS)
    if (EXISTS "${_c}")
      list(APPEND _bins "${_c}")
    endif()
  endforeach()
  if (_bins STREQUAL "")
    message(FATAL_ERROR
      "cat-ir: ${SOURCE_NAME} compiled to LTO bitcode but no candidate "
      "binary is available. Build the project first or rerun with a "
      "build mode whose binaries are linked.")
  endif()
endif()

set(_static_args -d --no-show-raw-insn --demangle "--x86-asm-syntax=${SYNTAX}")
if (DEFINED TRAILER AND NOT TRAILER STREQUAL "")
  list(APPEND _static_args ${TRAILER})
endif()

# Strip llvm-objdump's leading boilerplate so the `.s` opens on the first
# `<addr> <symbol>:` block.
function(_strip_headers in_var)
  set(_x "${${in_var}}")
  string(REGEX REPLACE "(^|\n)[^\n]+:[ \t]+file format[ \t]+[^\n]+" "\\1" _x "${_x}")
  string(REGEX REPLACE "(^|\n)Disassembly of section [^\n]+:[^\n]*" "\\1" _x "${_x}")
  string(REGEX REPLACE "^\n+" "" _x "${_x}")
  string(REGEX REPLACE "\n\n\n+" "\n\n" _x "${_x}")
  set(${in_var} "${_x}" PARENT_SCOPE)
endfunction()

# `int3` runs that immediately precede a function header (or end-of-dump)
# are the x86 backend's trap-fill (see `X86AsmBackend::writeNopData`) --
# unreachable code by construction. Drop them. Runs *inside* a function
# (e.g. a deliberate `__builtin_trap()`) survive because we only flush the
# pending buffer when a non-int3 / non-blank / non-header line appears.
function(_drop_trailing_int3 in_var)
  string(REPLACE "\r" "" _text "${${in_var}}")
  string(REPLACE "\n" ";" _lines "${_text}")
  set(_out "")
  set(_pending_int3 "")
  set(_pending_blank "")
  foreach(_line IN LISTS _lines)
    if (_line MATCHES "^[ \t]*[0-9a-fA-F]+:[ \t]+([0-9a-fA-F]+[ \t]+)?int3[ \t]*$")
      list(APPEND _pending_int3 ${_pending_blank})
      set(_pending_blank "")
      list(APPEND _pending_int3 "${_line}")
    elseif (_line MATCHES "^[ \t]*$")
      list(APPEND _pending_blank "${_line}")
    elseif (_line MATCHES "^[0-9a-fA-F]+ <.+>:[ \t]*$")
      set(_pending_int3 "")
      list(APPEND _out ${_pending_blank})
      set(_pending_blank "")
      list(APPEND _out "${_line}")
    else()
      list(APPEND _out ${_pending_int3})
      set(_pending_int3 "")
      list(APPEND _out ${_pending_blank})
      set(_pending_blank "")
      list(APPEND _out "${_line}")
    endif()
  endforeach()
  string(JOIN "\n" _result ${_out})
  set(${in_var} "${_result}" PARENT_SCOPE)
endfunction()

# For each candidate binary: dump everything (no fn=) or scope each
# matched symbol to its `(section, addr, addr+size)` triple. Per-symbol
# objdump invocations are necessary because `--start-address` /
# `--stop-address` only accept one range each and `--disassemble-symbols=`
# silently drops weak / function-section symbols (which is what most of
# libCat's templated / inline functions become).
set(_chunks "")
foreach(_bin IN LISTS _bins)
  set(_pieces "")
  if (FN)
    _cat_ir_match_addrs("${OBJDUMP}" "${_bin}" "${FN}" _addrs _sizes _sects)
    list(LENGTH _addrs _n)
    if (_n EQUAL 0)
      continue()
    endif()
    math(EXPR _last "${_n} - 1")
    foreach(_i RANGE 0 ${_last})
      list(GET _addrs ${_i} _addr)
      list(GET _sizes ${_i} _size)
      list(GET _sects ${_i} _sect)
      math(EXPR _stop "0x${_addr} + 0x${_size}" OUTPUT_FORMAT HEXADECIMAL)
      execute_process(
        COMMAND "${OBJDUMP}" ${_static_args}
                "--section=${_sect}"
                "--start-address=0x${_addr}"
                "--stop-address=${_stop}"
                "${_bin}"
        OUTPUT_VARIABLE _piece
        COMMAND_ERROR_IS_FATAL ANY)
      _strip_headers(_piece)
      string(STRIP "${_piece}" _piece)
      if (NOT _piece STREQUAL "")
        list(APPEND _pieces "${_piece}")
      endif()
    endforeach()
    if (NOT _pieces STREQUAL "")
      string(JOIN "\n\n" _disasm ${_pieces})
    else()
      set(_disasm "")
    endif()
  else()
    execute_process(
      COMMAND "${OBJDUMP}" ${_static_args} "${_bin}"
      OUTPUT_VARIABLE _disasm
      COMMAND_ERROR_IS_FATAL ANY)
    _strip_headers(_disasm)
    _drop_trailing_int3(_disasm)
    string(STRIP "${_disasm}" _disasm)
  endif()
  if (NOT _disasm STREQUAL "")
    cmake_path(GET _bin FILENAME _bin_name)
    list(APPEND _chunks "# === ${_bin_name} ===\n${_disasm}")
  endif()
endforeach()

if (_chunks STREQUAL "")
  if (FN)
    message(FATAL_ERROR
      "cat-ir: no defined symbols match '${FN}' across the candidate "
      "binaries for ${SOURCE_NAME}. The symbol may have been dead-code-"
      "eliminated. `${FN}` may have a typo. The function may be in an "
      "anonymous namespace, which `fn=` can't address. "
      "The name may require additional namespace qualification (e.g. `fn=foo::${FN}`)."
      "`bc`/`ll` without `fn=` dumps the full TU for inspection.")
  else()
    message(FATAL_ERROR
      "cat-ir: ${SOURCE_NAME} produced no disassembly. The candidate "
      "binaries appear to be empty -- this is almost certainly a build "
      "system bug.")
  endif()
endif()

cmake_path(GET OUT PARENT_PATH _out_dir)
if (_out_dir AND NOT EXISTS "${_out_dir}")
  file(MAKE_DIRECTORY "${_out_dir}")
endif()

string(JOIN "\n" _final ${_chunks})
file(WRITE "${OUT}" "${_final}\n")
