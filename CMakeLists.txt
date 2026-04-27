cmake_minimum_required(VERSION 3.24)

project(cat
  VERSION 0.1.0
  LANGUAGES CXX)

# libCat is Clang-only and uses Clang 23 features. Bail early instead of failing later
# with cryptic flag or intrinsic errors.
if (NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  message(FATAL_ERROR
    "libCat only supports Clang. Detected `${CMAKE_CXX_COMPILER_ID}` "
    "(`${CMAKE_CXX_COMPILER}`). Pass `-DCMAKE_CXX_COMPILER=clang++-23` "
    "when configuring.")
endif()
if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 23)
  message(FATAL_ERROR
    "libCat requires Clang 23+. Detected Clang ${CMAKE_CXX_COMPILER_VERSION} "
    "(`${CMAKE_CXX_COMPILER}`).")
endif()

# Unconditional ASan-runtime probe. One source of truth for the opt-in check
# below, the RPATH block further down, and `cat_shared.cmake`'s REPL preload.
# `-print-file-name` echoes the bare filename on miss, so `IS_ABSOLUTE` is the
# "found" test.
execute_process(
  COMMAND "${CMAKE_CXX_COMPILER}" -print-file-name=libclang_rt.asan-x86_64.so
  OUTPUT_VARIABLE _cat_asan_runtime
  OUTPUT_STRIP_TRAILING_WHITESPACE)
if (IS_ABSOLUTE "${_cat_asan_runtime}" AND EXISTS "${_cat_asan_runtime}")
  set(CAT_ASAN_RUNTIME_PATH "${_cat_asan_runtime}")
else()
  set(CAT_ASAN_RUNTIME_PATH "")
endif()
unset(_cat_asan_runtime)

option(CAT_USE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer." OFF)

if (CAT_USE_SANITIZERS)
  if (NOT CAT_ASAN_RUNTIME_PATH)
    message(FATAL_ERROR
      "CAT_USE_SANITIZERS=ON but `libclang_rt.asan-x86_64.so` could not be "
      "located for `${CMAKE_CXX_COMPILER}`. Install the Clang ASan runtime "
      "(apt: `libclang-rt-<MAJOR>-dev`) or reconfigure with "
      "`-DCAT_USE_SANITIZERS=OFF`.")
  endif()
  message(STATUS "CAT_USE_SANITIZERS: using ${CAT_ASAN_RUNTIME_PATH}")
endif()

# Optional sccache compiler-launcher. Auto-on when `sccache` is on PATH.
# `-DCAT_USE_SCCACHE=ON` makes it required. Pairs with `-fno-pch-timestamp`
# below for ~100% hit rates across build dirs.
find_program(CAT_SCCACHE_EXECUTABLE sccache DOC "Path to the sccache binary.")
if (CAT_SCCACHE_EXECUTABLE)
  set(_cat_sccache_default ON)
else()
  set(_cat_sccache_default OFF)
endif()
option(CAT_USE_SCCACHE "Use sccache as the compiler launcher for C and C++." ${_cat_sccache_default})
unset(_cat_sccache_default)

if (CAT_USE_SCCACHE)
  if (NOT CAT_SCCACHE_EXECUTABLE)
    message(FATAL_ERROR
      "CAT_USE_SCCACHE=ON but `sccache` was not found on PATH. "
      "Install it to path or pass -DCAT_SCCACHE_EXECUTABLE=/path/to/sccache.")
  endif()
  message(STATUS "sccache: using ${CAT_SCCACHE_EXECUTABLE} as compiler launcher")
  set(CMAKE_CXX_COMPILER_LAUNCHER "${CAT_SCCACHE_EXECUTABLE}")
endif()

# Bitcode-aware AR/RANLIB so `libcat.a` stays loadable by LTO and clang-repl.
include(${CMAKE_SOURCE_DIR}/cmake/llvm_archivers.cmake)

# Two flag tiers: `ESSENTIAL` = usage requirements every consumer must honor
# (on `cat` `INTERFACE`). `INTERNAL` = libCat's own warnings/diagnostics/per-
# config codegen (`PRIVATE` on `cat-impl`, re-propagated to in-tree tests/
# examples via `cat-tests` / `cat-examples`). External users only inherit
# `ESSENTIAL`.
list(
  APPEND CAT_COMPILE_OPTIONS_ESSENTIAL
  # Disable linking libC symbols.
  -nostdlib
  -nostdlib++
  # These features are pessimizations to libCat:
  # TODO: Exceptions and rtti should have optional support once it's possible to
  # implement them.
  -fno-exceptions -fno-rtti -fno-unwind-tables -fno-asynchronous-unwind-tables
  # `global_includes.hpp` must be available everywhere.
  -include global_includes.hpp

  # Enable CPU intrinsics required by the SIMD headers:
  # TODO: These should be optional.
  -msse4.2
  -mavx2
  -mfma
  -mlzcnt
  -mbmi
  -mbmi2
  -mfsgsbase
)

list(
  APPEND CAT_COMPILE_OPTIONS_INTERNAL_COMMON
  # Enable most warnings.
  -Wall -Wextra
  -Wno-unused-function
  -Wno-unknown-pragmas
  -Wno-missing-braces # This breaks `cat::tuple`.

  # libCat needs to violate these.
  -Wno-unqualified-std-cast-call
  -Wno-main
  -Wno-redundant-consteval-if # TODO: Do we need this? Remove if not.
  # Use Clang extensions.
  -Wno-gnu

  # Lifetime analysis and bounds-safety.
  -Wdangling
  -Wdangling-gsl
  "SHELL:-Xclang -fexperimental-bounds-safety"
  # TODO: -Wlifetime-safety-permissive
)

if(CAT_USE_SANITIZERS)
  # Whole-program. Rides with the essentials.
  list(
    APPEND CAT_COMPILE_OPTIONS_ESSENTIAL
    -fomit-frame-pointer # Required for UBsan.
    -fsanitize=undefined
    -fsanitize=address
    # `pthread_create` is intercepted so ASan learns each pthread stack. `clone`
    # with a user stack does not go through that path, so stack instrumentation
    # still assumes the main thread stack and faults on the mmap stack (e.g.
    # `nix::process::spawn_impl`). Disable stack ASan while keeping heap ASan.
    "SHELL:-mllvm -asan-stack=0"
  )
endif()

list(
  APPEND CAT_COMPILE_OPTIONS_DEBUG
  -ggdb3
  -gfull
)

list(
  APPEND CAT_COMPILE_OPTIONS_RELEASE
  # Remove unused symbols.
  -ffunction-sections -fdata-sections
  # TODO: Consider why no-plt instead of relro.
  -fno-plt
)

list(
  APPEND CAT_COMPILE_OPTIONS_RELWITHDEBINFO
  ${CAT_COMPILE_OPTIONS_DEBUG}
  ${CAT_COMPILE_OPTIONS_RELEASE}
)

# LTO breaks under clang with sanitizers, so it's special-cased. Same conditional
# adds `-fvisibility=hidden` only to `Release` (`RelWithDebInfo` already captured
# the list above), keeping `RelWithDebInfo` inspectable in tools like `nm` or
# debuggers.
if (NOT CAT_USE_SANITIZERS)
  list(
    APPEND CAT_COMPILE_OPTIONS_RELEASE
    -flto=auto
    -fvisibility=hidden -fvisibility-inlines-hidden
  )

  list(
    APPEND CAT_COMPILE_OPTIONS_RELWITHDEBINFO
    # Use thin LTO for optimized non-release builds.
    -flto=thin
  )
endif()

list(
  APPEND CAT_COMPILE_OPTIONS_INTERNAL
  ${CAT_COMPILE_OPTIONS_INTERNAL_COMMON}
  $<$<CONFIG:Debug>:${CAT_COMPILE_OPTIONS_DEBUG}>
  $<$<CONFIG:Release>:${CAT_COMPILE_OPTIONS_RELEASE}>
  $<$<CONFIG:RelWithDebInfo>:${CAT_COMPILE_OPTIONS_RELWITHDEBINFO}>
)

# Kept for tooling (format target, opt-report) that wants the full set.
list(
  APPEND CAT_COMPILE_OPTIONS
  ${CAT_COMPILE_OPTIONS_ESSENTIAL}
  ${CAT_COMPILE_OPTIONS_INTERNAL}
)

list(
  APPEND CAT_LINK_OPTIONS_COMMON
  # This is required to prevent duplicate symbols.
  -nostdlib
  # `lld` is required for LLVM LTO. Bfd and `mold` do not work.
  -fuse-ld=lld
)

list(
  APPEND CAT_LINK_OPTIONS_RELEASE
  # Remove unused symbols.
  -Wl,-z,noseparate-code,--gc-sections
)

list(
  APPEND CAT_LINK_OPTIONS
  ${CAT_LINK_OPTIONS_COMMON}
  $<$<CONFIG:Release>:${CAT_LINK_OPTIONS_RELEASE}>
  $<$<CONFIG:RelWithDebInfo>:${CAT_LINK_OPTIONS_RELEASE}>
)

# Two-target setup: `cat` (`INTERFACE`) carries usage requirements. `cat-impl`
# (`STATIC`) owns the .cpp sources and produces `libcat.a`. `cat` `INTERFACE`-
# links `cat-impl` (or `cat-impl-shared`, see `CAT_USE_SHARED` below). The
# wiring happens further down once both targets exist. `_start.cpp` is the one
# source that stays on `cat` `INTERFACE` because its body switches on the per-
# consumer `NO_ARGC_ARGV` define.
add_library(cat INTERFACE)

# C++26 + GNU extensions. Raw flag instead of `cxx_std_26` because that's
# broken for Clang 19 until CMake 3.29.2.
target_compile_options(cat INTERFACE "-std=gnu++26")

if (CAT_USE_SANITIZERS)
  target_link_options(cat INTERFACE
    -fsanitize=address
    #-fsanitize=undefined
    -shared-libasan
    # `libcat.ld`'s `INSERT AFTER .text` splits RELRO sections under ASan + lld,
    # which lld rejects unless RELRO is disabled for this link.
    -Wl,-z,norelro
  )

  # Shared ASan lives next to Clang's resource dir, which is not a default loader
  # path. RPATH lets every linked executable run from the build tree or shell.
  # `CAT_ASAN_RUNTIME_PATH` was populated and verified at the top of this file.
  get_filename_component(_cat_asan_rpath_dir "${CAT_ASAN_RUNTIME_PATH}" DIRECTORY)
  target_link_options(cat INTERFACE "-Wl,-rpath,${_cat_asan_rpath_dir}")
  unset(_cat_asan_rpath_dir)
endif()

target_compile_options(cat INTERFACE ${CAT_COMPILE_OPTIONS_ESSENTIAL})
target_link_options(cat INTERFACE ${CAT_LINK_OPTIONS})

# TODO: Implement these symbols in libCat directly.
# target_link_libraries(cat INTERFACE gcc)

# `cat-impl` deliberately does NOT link `cat`. That would drag `_start.cpp`
# (`cat`'s `INTERFACE_SOURCES`) into the archive with the wrong `NO_ARGC_ARGV`
# setting. Its actual usage requirements come in via generator expressions so
# there is still one source of truth on `cat`.
add_library(cat-impl STATIC)
set_target_properties(cat-impl PROPERTIES OUTPUT_NAME cat)
target_compile_options(cat-impl PRIVATE
  $<TARGET_PROPERTY:cat,INTERFACE_COMPILE_OPTIONS>
  ${CAT_COMPILE_OPTIONS_INTERNAL}
)
target_include_directories(cat-impl PRIVATE
  $<TARGET_PROPERTY:cat,INTERFACE_INCLUDE_DIRECTORIES>
)

add_subdirectory(src/)

# PCH uses `.hpp` only (extensionless API headers go through the regular
# include path via `CAT_HEADER_FILES` in `src/CMakeLists.txt`). Glob is scoped
# to `src/` so the PCH never picks up `tests/unit_tests.hpp`.
file(GLOB_RECURSE CAT_HEADERS "src/*.hpp")

option(CAT_PCH "Enable precompiled-headers for libCat headers." ON)
if (CAT_PCH)
  # `PRIVATE` on `cat-impl` so external consumers of `cat::cat` don't inherit
  # PCH machinery. In-tree tests/examples opt in via `REUSE_FROM cat-impl`.
  target_precompile_headers(cat-impl PRIVATE ${CAT_HEADERS})
  # Strip the PCH's embedded mtime so sccache / ccache can reuse PCH-consuming
  # TUs across rebuilds. `SHELL:` keeps `-Xclang` glued to its argument.
  target_compile_options(cat-impl PRIVATE
    "SHELL:-Xclang -fno-pch-timestamp"
  )
endif()
set_target_properties(cat-impl PROPERTIES PCH_WARN_INVALID ON)
# Tripwire (deferred): fail configure if `INTERFACE_PRECOMPILE_HEADERS` is
# ever set on `cat`, `cat-impl`, or `cat-impl-shared`. That would export PCH
# usage requirements through `cat::cat[-impl[-shared]]` and bake `cat-impl`'s
# default `gnu::naked` `_start` declaration into every consumer. Fatal for
# any TU that defines `NO_ARGC_ARGV` (`unit_tests`, `hello`, external no-args
# programs), which would then hit "non-ASM statement in naked function" on the
# call to `call_main_noargs()`. `REUSE_FROM cat-impl` is unaffected: it writes
# on the consumer target, not on `cat-impl`'s `INTERFACE`.
function(_cat_assert_no_interface_pch)
  set(_targets cat cat-impl)
  if (TARGET cat-impl-shared)
    list(APPEND _targets cat-impl-shared)
  endif()
  foreach(_t IN LISTS _targets)
    get_target_property(_pch ${_t} INTERFACE_PRECOMPILE_HEADERS)
    if (_pch)
      message(FATAL_ERROR
        "libCat PCH isolation broken: target `${_t}` has "
        "INTERFACE_PRECOMPILE_HEADERS set (value: ${_pch}). Keep PCH on "
        "cat-impl PRIVATE and use `target_precompile_headers(<consumer> "
        "REUSE_FROM cat-impl)` for in-tree opt-in. See the comment block "
        "above this assertion in the top-level CMakeLists.txt.")
    endif()
  endforeach()
endfunction()
cmake_language(DEFER DIRECTORY "${CMAKE_SOURCE_DIR}"
  CALL _cat_assert_no_interface_pch)

# Optional `libcat.so` (`cat-impl-shared`), linkable as `cat::cat-impl-shared`
# from `find_package(cat)` and consumed by `clang-repl-libcat`. Off by default.
option(CAT_BUILD_SHARED
  "Build libcat.so (cat-impl-shared) and the clang-repl-libcat wrapper." OFF)

# Switches `cat`'s `INTERFACE`-linked impl to `cat-impl-shared`, so every
# `cat`-linker transparently picks up `libcat.so`. Implies `CAT_BUILD_SHARED`.
option(CAT_USE_SHARED
  "Make `cat` link libcat.so instead of libcat.a (implies CAT_BUILD_SHARED)." OFF)
if (CAT_USE_SHARED AND NOT CAT_BUILD_SHARED)
  set(CAT_BUILD_SHARED ON CACHE BOOL
    "Build libcat.so (cat-impl-shared) and the clang-repl-libcat wrapper." FORCE)
  message(STATUS "CAT_USE_SHARED=ON: forcing CAT_BUILD_SHARED on.")
endif()

if (CAT_BUILD_SHARED)
  include(${CMAKE_SOURCE_DIR}/cmake/cat_shared.cmake)
else()
  # Give the user an actionable error message when they try to run `cat-repl`
  # without `CAT_BUILD_SHARED=ON`.
  add_custom_target(cat-repl
    COMMAND ${CMAKE_COMMAND} -E echo
      "cat-repl: requires -DCAT_BUILD_SHARED=ON. Reconfigure with:"
    COMMAND ${CMAKE_COMMAND} -E echo
      "  cmake -B ${CMAKE_BINARY_DIR} -DCAT_BUILD_SHARED=ON"
    COMMAND ${CMAKE_COMMAND} -E false
    COMMENT "cat-repl unavailable: CAT_BUILD_SHARED is OFF.")
endif()

# Pick which impl `cat` `INTERFACE`-links. Default is `cat-impl` (`libcat.a`).
if (CAT_USE_SHARED)
  target_link_libraries(cat INTERFACE cat-impl-shared)
else()
  target_link_libraries(cat INTERFACE cat-impl)
endif()

# `.gdbinit` symlink helper, shared by `tests/` and `examples/`.
include(${CMAKE_SOURCE_DIR}/cmake/gdb_symlink.cmake)

enable_testing()
add_subdirectory(tests/)
add_subdirectory(examples/)

# # `ctor.ld` is a minimal linker script necessary for thread local storage.
# target_link_options(cat INTERFACE -T ${CMAKE_SOURCE_DIR}/src/ctor.ld)
# set_target_properties(
#   cat PROPERTIES
#   LINK_DEPENDS ${CMAKE_SOURCE_DIR}/src/ctor.ld
# )

# Source-formatting targets (`cat-format`, `cat-format-check`, `cat-reflow-comments`,
# `cat-reflow-comments-check`).
include(${CMAKE_SOURCE_DIR}/cmake/format.cmake)

# Source-tidying targets (`cat-tidy`, `cat-tidy-check`).
include(${CMAKE_SOURCE_DIR}/cmake/tidy.cmake)

# Compiler optimization report target (`cat-opt-report`).
include(${CMAKE_SOURCE_DIR}/cmake/opt_report.cmake)

# Compiler-intermediaries target (`cat-intermediaries`).
include(${CMAKE_SOURCE_DIR}/cmake/cat_intermediaries.cmake)

# Syntax-only check target (`cat-syntax`).
include(${CMAKE_SOURCE_DIR}/cmake/cat_syntax.cmake)

# Install / export rules (`cmake --install build/` = `find_package(cat)` sysroot).
include(${CMAKE_SOURCE_DIR}/cmake/cat_install.cmake)

# Optional configure-time `try_compile` negative checks for
# `cat::arithmetic` / `cat::index` / `cat::arithmetic_ptr` (see
# `cmake/cat_arithmetic_negative.cmake`). Off by default; the
# `arithmetic-negative.yml` workflow opts in via
# `CAT_BUILD_ARITHMETIC_NEGATIVE_TESTS=ON`. Included after
# `add_subdirectory(src/)` so include paths are available.
include(${CMAKE_SOURCE_DIR}/cmake/cat_arithmetic_negative.cmake)
