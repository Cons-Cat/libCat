# This file is flagrantly "vibe-coded". It may not be up to the standards of most libCat code.

# Source formatting (`cat-format`, `cat-format-check`) and full-line // reflow
# (`cat-reflow-comments`, `cat-reflow-comments-check`, see
# `scripts/reflow_prefix_comment_paragraphs.py`)
#
# Module mode (`include()`): locates a `clang-format` whose major
# matches the configured Clang, gathers libCat's sources, public
# headers from the `cat` / `cat-impl` targets, and unit test sources
# on `cat-tests` plus `tests/unit_tests.cpp`, and registers the
# `cat-format` and `cat-format-check` custom targets. Both run
# `scripts/cat_format_worktree.py` (parallel) with the same file list
# (capped at 4 workers, default. Tune down with =CAT_LIBCAT_JOBS= or =-j=, see
# the script help).
# `APPLY` rewrites mismatches. `CHECK` is read-only, non-zero on drift.
#
# Requires `python3` on =PATH= for `cat-format=`, =cat-format-check=, and
# `cat-reflow-*`.

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
  # `tests/` is not on `cat` or `cat-impl`, add so `cat-format` and reflows stay
  # consistent with the rest of the tree.
  if (TARGET cat-tests)
    get_target_property(_cat_test_format_SOURCES cat-tests INTERFACE_SOURCES)
    if (NOT _cat_test_format_SOURCES STREQUAL
        "_cat_test_format_SOURCES-NOTFOUND")
      list(APPEND _cat_format_files ${_cat_test_format_SOURCES})
    endif()
  endif()
  list(APPEND _cat_format_files
    "${CMAKE_SOURCE_DIR}/tests/unit_tests.cpp")
  set(_cat_format_worktree
    "${CMAKE_SOURCE_DIR}/scripts/cat_format_worktree.py")
  set(_cat_format_file_list "${_cat_format_files}")
  list(FILTER _cat_format_file_list EXCLUDE REGEX "^$")
  find_program(CAT_PYTHON3
    NAMES python3
    DOC "Python 3 for `cat-format` and `cat-reflow-comments`")
  # `cat-format` rewrites mismatches in place. `cat-format-check` is the
  # read-only CI variant. Both use `cat_format_worktree.py` in parallel
  # (capped at 4 workers, default. Tune down with =CAT_LIBCAT_JOBS= or =-j=.)
  function(_cat_add_format_target target mode)
    if (NOT CAT_CLANG_FORMAT_PATH)
      add_custom_target(
        ${target}
        COMMAND
          ${CMAKE_COMMAND}
          -E
          echo
          "${target}: cmake did not find clang-format ${_cat_cf_major}. Install clang-format-${_cat_cf_major} or configure with -DCAT_CLANG_FORMAT_PATH=/path/to/clang-format-${_cat_cf_major}"
        COMMAND ${CMAKE_COMMAND} -E false)
    elseif (NOT CAT_PYTHON3)
      add_custom_target(
        ${target}
        COMMAND
          ${CMAKE_COMMAND}
          -E
          echo
          "${target}: `python3` not found on PATH (required for parallel format)"
        COMMAND ${CMAKE_COMMAND} -E false)
    elseif (NOT EXISTS "${_cat_format_worktree}")
      add_custom_target(
        ${target}
        COMMAND
          ${CMAKE_COMMAND}
          -E
          echo
          "cat-format: missing `${_cat_format_worktree}`"
        COMMAND ${CMAKE_COMMAND} -E false)
    else()
      add_custom_target(
        ${target}
        COMMAND
          ${CAT_PYTHON3}
          "${_cat_format_worktree}"
          ${mode}
          ${CAT_CLANG_FORMAT_PATH}
          ${_cat_format_file_list}
        WORKING_DIRECTORY
          "${CMAKE_SOURCE_DIR}"
        DEPENDS
          "${_cat_format_worktree}"
          ${_cat_format_file_list}
        VERBATIM)
    endif()
  endfunction()

  _cat_add_format_target(cat-format       APPLY)
  _cat_add_format_target(cat-format-check CHECK)

  # `cat-reflow-comments` / `cat-reflow-comments-check` run the same file set
  # through
  # `scripts/reflow_prefix_comment_paragraphs.py` (word-preserving // wrap to
  # 80 columns, URL-safe). `CHECK` is read-only, like `cat-format-check`.
  # Reflow is not a substitute for `clang-format`; run `cat-format` afterward
  # so the tree matches `.clang-format`.
  set(_cat_reflow_script "${CMAKE_SOURCE_DIR}/scripts/reflow_prefix_comment_paragraphs.py")
  if (CAT_PYTHON3)
    if (EXISTS "${_cat_reflow_script}")
      set(_cat_reflow_files "${_cat_format_files}")
      list(FILTER _cat_reflow_files EXCLUDE REGEX "^$")
      add_custom_target(
        cat-reflow-comments
        COMMAND
          ${CAT_PYTHON3}
          "${_cat_reflow_script}"
          --max-col
          80
          ${_cat_reflow_files}
        DEPENDS
          ${_cat_reflow_files}
        WORKING_DIRECTORY
          "${CMAKE_SOURCE_DIR}"
        COMMENT
          "Reflow full-line // comments in place (run `cat-format` after)"
        VERBATIM)
      add_custom_target(
        cat-reflow-comments-check
        COMMAND
          ${CAT_PYTHON3}
          "${_cat_reflow_script}"
          --check
          --max-col
          80
          ${_cat_reflow_files}
        DEPENDS
          ${_cat_reflow_files}
        WORKING_DIRECTORY
          "${CMAKE_SOURCE_DIR}"
        COMMENT
          "Check // comment reflow (read-only; `cat-reflow-comments` to fix)"
        VERBATIM)
    else()
      add_custom_target(
        cat-reflow-comments
        COMMAND
          ${CMAKE_COMMAND}
          -E
          echo
          "cat-reflow-comments: missing `${_cat_reflow_script}`"
        COMMAND ${CMAKE_COMMAND} -E false
        VERBATIM)
      add_custom_target(
        cat-reflow-comments-check
        COMMAND
          ${CMAKE_COMMAND}
          -E
          echo
          "cat-reflow-comments-check: missing `${_cat_reflow_script}`"
        COMMAND ${CMAKE_COMMAND} -E false
        VERBATIM)
    endif()
  else()
    add_custom_target(
      cat-reflow-comments
      COMMAND
        ${CMAKE_COMMAND}
        -E
        echo
        "cat-reflow-comments: `python3` not found on PATH"
      COMMAND ${CMAKE_COMMAND} -E false
      VERBATIM)
    add_custom_target(
      cat-reflow-comments-check
      COMMAND
        ${CMAKE_COMMAND}
        -E
        echo
        "cat-reflow-comments-check: `python3` not found on PATH"
      COMMAND ${CMAKE_COMMAND} -E false
      VERBATIM)
  endif()

  unset(_cat_cf_major)
endif()
