# This file is flagrantly "vibe-coded". It may not be up to the standards of most libCat code.

# LLVM archiver / ranlib selection
#
# `libcat.a` is a bitcode archive whenever LTO is on (see `-flto=auto` /
# `-flto=thin` in the top-level CMakeLists), so the archiver and ranlib
# must both understand LLVM bitcode. The same archive is what
# `clang-repl` loads to pull libCat's out-of-line implementations into
# its JIT session, and that path likewise needs a valid bitcode symbol
# table. GNU `ar` without the gold plugin silently drops those indices,
# leaving an archive that neither the LTO linker nor `clang-repl` can
# consume.
#
# Clang publishes matching `llvm-ar` / `llvm-ranlib` binaries via
# `CMAKE_CXX_COMPILER_AR` / `_RANLIB`. Prefer those, then fall back to a
# version-matched lookup derived from `CMAKE_CXX_COMPILER_VERSION` (e.g.
# `llvm-ar-23` alongside `clang++-23`), then to the unversioned names on
# PATH. No specific major is pinned, so the project tracks whichever
# Clang CMake selected.
#
# Side-effecting `include()` script. After include, `CMAKE_AR` /
# `CMAKE_RANLIB` point at a bitcode-aware pair when one is found.

string(REGEX MATCH "^[0-9]+" _cat_clang_major "${CMAKE_CXX_COMPILER_VERSION}")
if (CMAKE_CXX_COMPILER_AR)
  set(CMAKE_AR "${CMAKE_CXX_COMPILER_AR}")
else()
  find_program(CAT_LLVM_AR
    NAMES "llvm-ar-${_cat_clang_major}" llvm-ar
    DOC "`llvm-ar` matching ${CMAKE_CXX_COMPILER}.")
  if (CAT_LLVM_AR)
    set(CMAKE_AR "${CAT_LLVM_AR}")
  endif()
endif()
if (CMAKE_CXX_COMPILER_RANLIB)
  set(CMAKE_RANLIB "${CMAKE_CXX_COMPILER_RANLIB}")
else()
  find_program(CAT_LLVM_RANLIB
    NAMES "llvm-ranlib-${_cat_clang_major}" llvm-ranlib
    DOC "`llvm-ranlib` matching ${CMAKE_CXX_COMPILER}.")
  if (CAT_LLVM_RANLIB)
    set(CMAKE_RANLIB "${CAT_LLVM_RANLIB}")
  endif()
endif()
unset(_cat_clang_major)
