# This file is flagrantly "vibe-coded". It may not be up to the standards of
# most libCat code.

# `cat-ir` s rule -- assembly per TU, via `scripts/cat_ir_disasm.cmake`.
#
# The disasm script handles both branches:
#   - non-LTO ELF: disassemble the shadow `.cpp.o` directly. `fn=` optional
#     (filters via `--syms` -> per-symbol address-range invocations).
#   - LTO bitcode: scan candidate binaries' demangled symbol tables and
#     disassemble matches per address range. Candidate set:
#       tests    -> `unit_tests`.
#       examples -> the owner exec.
#       src      -> every libCat exec (LTO may park the symbol anywhere);
#                   `fn=` is required, otherwise the script emits an
#                   actionable error since per-TU origin isn't recoverable
#                   without DWARF.
#
# Reads file-scope vars set by `cat_ir.cmake`:
#   CAT_LLVM_OBJDUMP_PATH, CAT_IR_FUNCTION, CAT_IR_S_SYNTAX,
#   CAT_IR_OBJDUMP_TRAILER, _cat_ir_disasm_cmake, _cat_ir_match_cmake,
#   _cat_ir_all_execs, _cat_ir_all_exec_targets, _cat_ir_any_lto.
# Also calls the file-scope helper `_cat_ir_exec_path`.

function(_cat_ir_register_s domain basename
                             dst_dir dst
                             compile_anchor obj_o
                             owner_target)
  set(_candidates "")
  set(_candidate_targets "")
  if (domain STREQUAL "tests")
    if (TARGET unit_tests)
      _cat_ir_exec_path(unit_tests _path)
      list(APPEND _candidates "${_path}")
      list(APPEND _candidate_targets unit_tests)
    endif()
  elseif (domain STREQUAL "examples")
    if (owner_target AND TARGET ${owner_target})
      _cat_ir_exec_path(${owner_target} _path)
      list(APPEND _candidates "${_path}")
      list(APPEND _candidate_targets ${owner_target})
    endif()
  else()
    set(_candidates "${_cat_ir_all_execs}")
    set(_candidate_targets "${_cat_ir_all_exec_targets}")
  endif()

  # Pick up the matching domain's `fn=` pre-flight stamp when registered
  # (LTO + fn=). See `cat_ir.cmake` for the rationale.
  set(_extra_deps "")
  if (domain STREQUAL "src" AND _cat_ir_src_fn_stamp)
    list(APPEND _extra_deps "${_cat_ir_src_fn_stamp}")
  elseif (domain STREQUAL "tests" AND _cat_ir_tests_fn_stamp)
    list(APPEND _extra_deps "${_cat_ir_tests_fn_stamp}")
  elseif (domain STREQUAL "examples" AND _cat_ir_examples_fn_stamp)
    list(APPEND _extra_deps "${_cat_ir_examples_fn_stamp}")
  endif()

  # Decide whether the disasm path will actually consume the candidate execs:
  # any LTO config (where the shadow `.cpp.o` is bitcode and the script falls
  # through to a candidate scan), excluding the `src`-no-fn case (the script
  # bails with an error before opening any binary). When yes, plumb the
  # candidate file paths into the custom command's `DEPENDS` so ninja
  # re-runs the disasm whenever a candidate binary is relinked, not only
  # when the (unchanging) bitcode shadow object is touched. The
  # `add_dependencies(... ${_candidate_targets})` below only adds
  # build-order constraints, not file-level invalidation.
  set(_consume_candidates FALSE)
  if (_cat_ir_any_lto AND _candidate_targets)
    set(_consume_candidates TRUE)
    if (domain STREQUAL "src" AND NOT CAT_IR_FUNCTION)
      set(_consume_candidates FALSE)
    endif()
  endif()
  if (_consume_candidates)
    list(APPEND _extra_deps ${_candidates})
  endif()

  add_custom_command(OUTPUT "${dst}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${dst_dir}"
    COMMAND "${CMAKE_COMMAND}"
            "-DSHADOW_OBJ=${obj_o}"
            "-DSOURCE_NAME=${basename}.cpp"
            "-DDOMAIN=${domain}"
            "-DSYNTAX=${CAT_IR_S_SYNTAX}"
            "-DOBJDUMP=${CAT_LLVM_OBJDUMP_PATH}"
            "-DOUT=${dst}"
            "-DFN=${CAT_IR_FUNCTION}"
            "-DTRAILER=${CAT_IR_OBJDUMP_TRAILER}"
            "-DCANDIDATE_BINS=${_candidates}"
            -P "${_cat_ir_disasm_cmake}"
    DEPENDS "${compile_anchor}" "${_cat_ir_disasm_cmake}"
            "${_cat_ir_match_cmake}" ${_extra_deps}
    COMMENT "ir ${domain}/${basename}.s"
    VERBATIM)
  add_custom_target(${basename}-s DEPENDS "${dst}")
  if (owner_target)
    add_dependencies(${basename}-s ${owner_target})
  endif()

  if (_consume_candidates)
    add_dependencies(${basename}-s ${_candidate_targets})
  endif()

  set_property(GLOBAL APPEND PROPERTY
    _cat_ir_${domain}_targets_s ${basename}-s)
endfunction()
