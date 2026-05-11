cmake_minimum_required(VERSION 3.25)

if (NOT DEFINED CMAKE_LINKER_TYPE AND DEFINED ENV{CMAKE_LINKER_TYPE})
  set(CMAKE_LINKER_TYPE "$ENV{CMAKE_LINKER_TYPE}" CACHE STRING
    "CMake linker type selected by mise.")
endif()

project(cat
  VERSION 0.1.0
  LANGUAGES CXX)

# libCat is Clang-only and uses Clang 23 features. Bail early instead of failing
# later with cryptic flag or intrinsic errors.
if (NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  message(FATAL_ERROR
    "libCat only supports Clang. Detected `${CMAKE_CXX_COMPILER_ID}` "
    "(`${CMAKE_CXX_COMPILER}`). Run CMake through `mise x -- cmake`!")
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
block(PROPAGATE CAT_ASAN_RUNTIME_PATH)
  execute_process(
    COMMAND "${CMAKE_CXX_COMPILER}" -print-file-name=libclang_rt.asan-x86_64.so
    OUTPUT_VARIABLE _runtime
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  if (IS_ABSOLUTE "${_runtime}" AND EXISTS "${_runtime}")
    set(CAT_ASAN_RUNTIME_PATH "${_runtime}")
  else()
    set(CAT_ASAN_RUNTIME_PATH "")
  endif()
endblock()

option(CAT_USE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer." OFF)

# `CAT_THREAD_LOCAL_SIZE` controls the `thread_local` storage buffer.
# It configures or disables the `thread_local` language feature.
#
# A value of 0 disables `thread_local` entirely.
# A value greater than 0 explicitly sets the `thread_local` storage buffer size.
# If the value is unset (default), the linker solves the correct size.
set(CAT_THREAD_LOCAL_SIZE "" CACHE STRING
  "Override the `thread_local` storage buffer size.")
if (NOT CAT_THREAD_LOCAL_SIZE STREQUAL "")
  if (NOT CAT_THREAD_LOCAL_SIZE MATCHES "^[0-9]+$")
    message(FATAL_ERROR
      "CAT_THREAD_LOCAL_SIZE must be a non-negative integer or empty, not "
      "`${CAT_THREAD_LOCAL_SIZE}`.")
  endif()
  add_compile_definitions(CAT_THREAD_LOCAL_SIZE=${CAT_THREAD_LOCAL_SIZE})
endif()

if (CAT_USE_SANITIZERS)
  if (NOT CAT_ASAN_RUNTIME_PATH)
    message(FATAL_ERROR
      "CAT_USE_SANITIZERS=ON but `libclang_rt.asan-x86_64.so` could not be "
      "located for `${CMAKE_CXX_COMPILER}`. Install the matching Clang ASan "
      "runtime or reconfigure with "
      "`-DCAT_USE_SANITIZERS=OFF`.")
  endif()
  message(VERBOSE "CAT_USE_SANITIZERS: using ${CAT_ASAN_RUNTIME_PATH}")
endif()

# Optional sccache compiler-launcher. Auto-on when `sccache` is on PATH.
# `-DCAT_USE_SCCACHE=ON` makes it required. Pairs with `-fno-pch-timestamp`
# below for ~100% hit rates across build dirs.
find_program(CAT_SCCACHE_EXECUTABLE sccache DOC "Path to the sccache binary.")
block()
  if (CAT_SCCACHE_EXECUTABLE)
    set(_default ON)
  else()
    set(_default OFF)
  endif()
  option(CAT_USE_SCCACHE
    "Use sccache as the compiler launcher for C and C++." ${_default})
endblock()

if (CAT_USE_SCCACHE)
  if (NOT CAT_SCCACHE_EXECUTABLE)
    message(FATAL_ERROR
      "CAT_USE_SCCACHE=ON but `sccache` was not found on PATH. "
      "Install it to path or pass -DCAT_SCCACHE_EXECUTABLE=/path/to/sccache.")
  endif()
  message(VERBOSE "sccache: using ${CAT_SCCACHE_EXECUTABLE} as compiler launcher")
  set(CMAKE_CXX_COMPILER_LAUNCHER "${CAT_SCCACHE_EXECUTABLE}")
endif()

# Bitcode-aware AR/RANLIB so `libcat.a` stays loadable by LTO and `clang-repl`.
include(${CMAKE_SOURCE_DIR}/cmake/llvm_archivers.cmake)

# Two flag tiers: `ESSENTIAL` = usage requirements every consumer must honor (on
# `cat` `INTERFACE`). `INTERNAL` = libCat's own warnings/diagnostics/per- config
# codegen (`PRIVATE` on `cat-impl`, re-propagated to in-tree tests/ examples via
# `cat-tests`/`cat-examples`). External users only inherit `ESSENTIAL`.
list(
  APPEND CAT_CXX_FLAGS_ESSENTIAL
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
  -mfma
  -mlzcnt
  -mbmi
  -mbmi2
)

list(
  APPEND CAT_CXX_FLAGS_COMMON
  # Enable most warnings.
  -Wall -Wextra
  -Wno-unused-function
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
  # TODO: -Wlifetime-safety-permissive.
)

if(CAT_USE_SANITIZERS)
  # Whole-program. Rides with the essentials.
  list(
    APPEND CAT_CXX_FLAGS_ESSENTIAL
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
  APPEND CAT_CXX_FLAGS_DEBUG
  -ggdb3
  -gfull
)

list(
  APPEND CAT_CXX_FLAGS_RELEASE
  # Remove unused symbols.
  -ffunction-sections -fdata-sections
  # TODO: Consider why no-plt instead of relro.
  -fno-plt
)

list(
  APPEND CAT_CXX_FLAGS_RELWITHDEBINFO
  ${CAT_CXX_FLAGS_DEBUG}
  ${CAT_CXX_FLAGS_RELEASE}
)

# LTO breaks under Clang with sanitizers, so it's special-cased. Same
# conditional adds `-fvisibility=hidden` only to `Release` (`RelWithDebInfo`
# already captured the list above), keeping `RelWithDebInfo` inspectable in
# tools like `nm` or debuggers.
if (NOT CAT_USE_SANITIZERS)
  list(
    APPEND CAT_CXX_FLAGS_RELEASE
    -flto=auto
    -fvisibility=hidden -fvisibility-inlines-hidden
  )

  list(
    APPEND CAT_CXX_FLAGS_RELWITHDEBINFO
    -flto=auto
  )
endif()

list(
  APPEND CAT_CXX_FLAGS_INTERNAL
  ${CAT_CXX_FLAGS_COMMON}
  $<$<CONFIG:Debug>:${CAT_CXX_FLAGS_DEBUG}>
  $<$<CONFIG:Release>:${CAT_CXX_FLAGS_RELEASE}>
  $<$<CONFIG:RelWithDebInfo>:${CAT_CXX_FLAGS_RELWITHDEBINFO}>
)

# Kept for tooling (format target, opt-report) that wants the full set.
list(
  APPEND CAT_CXX_FLAGS
  ${CAT_CXX_FLAGS_ESSENTIAL}
  ${CAT_CXX_FLAGS_INTERNAL}
)

list(
  APPEND CAT_LINK_OPTIONS_COMMON
  # This is required to prevent duplicate symbols.
  -nostdlib
)

list(
  APPEND CAT_LINK_OPTIONS_RELEASE
  # Remove unused symbols.
  -Wl,-z,noseparate-code,--gc-sections
  # Bump LTO codegen optimization above LLD's default of `-O2`.
  -Wl,--lto-O3
  # Eager binding pairs with `--gc-sections` to fold lazy PLT stubs.
  -Wl,-z,now
  # Identical-code folding for redundant shims.
  -Wl,--icf=safe
  # Drop the 36-byte `.note.gnu.build-id` section. Useful for distros that
  # ship debug symbol packages, but libCat's tests and examples don't.
  -Wl,--build-id=none
  # This drops the 4096-byte aligned `GNU_RELRO` segment and `.relro_padding` outright.
  -Wl,-z,norelro
  # If a Release link does end up with dynamic symbols (sanitizer or
  # `CAT_USE_SHARED` builds), `sysv` hashes are smaller than the default `gnu` style for
  # the small symbol counts libCat consumers produce.
  -Wl,--hash-style=sysv
)

# `-static`, `--no-pie`, and `-Wl,-N` (omagic) can't be used under any
# dynamic linking scenario.
if (NOT CAT_USE_SHARED AND NOT CAT_USE_SANITIZERS)
  list(
    APPEND CAT_LINK_OPTIONS_RELEASE
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:-static>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:-Wl,--no-pie>"
    # Merge the read-only and read+exec LOAD segments into a single
    # read+write+exec LOAD. Drops one program header (56 bytes) and the
    # page-alignment padding. A consumer that needs W^X can override with
    # `-Wl,-z,relro -Wl,-z,now` on her own target.
    # https://stackoverflow.com/questions/60657147/linker-what-is-nmagic-section-of-linked-file-and-what-is-the-aim-of-section-a
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:-Wl,-N>"
  )
  # Some code, such as `thread_local` initialization, depends on this.
  list(
    APPEND CAT_CXX_FLAGS_ESSENTIAL
    "$<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>:-DCAT_STATIC_LINKED>"
  )
endif()

list(
  APPEND CAT_LINK_OPTIONS
  ${CAT_LINK_OPTIONS_COMMON}
  $<$<CONFIG:Release>:${CAT_LINK_OPTIONS_RELEASE}>
  $<$<CONFIG:RelWithDebInfo>:${CAT_LINK_OPTIONS_RELEASE}>
)

# Two-target setup: `cat` (`INTERFACE`) carries usage requirements. `cat-impl`
# (`STATIC`) owns the .cpp sources and produces `libcat.a`. `cat` `INTERFACE` -
# links `cat-impl` (or `cat-impl-shared`, see `CAT_USE_SHARED` below). The
# wiring happens further down once both targets exist. `_start.cpp` is the one
# source that stays on `cat` `INTERFACE` because its body switches on the per-
# consumer `NO_ARGC_ARGV` define.
add_library(cat INTERFACE)

# C++26 + GNU extensions.
set(CAT_CXX_STANDARD_FEATURE cxx_std_26)
target_compile_features(cat INTERFACE ${CAT_CXX_STANDARD_FEATURE})
set_target_properties(cat PROPERTIES CXX_EXTENSIONS ON)

if (CAT_USE_SANITIZERS)
  target_link_options(cat INTERFACE
    -fsanitize=address
    #-fsanitize=undefined.
    -shared-libasan
    # `libcat.ld`'s `INSERT AFTER .text` splits RELRO sections under ASan + lld,
    # which lld rejects unless RELRO is disabled for this link.
    -Wl,-z,norelro
  )

  # Shared ASan lives next to Clang's resource dir, which is not a default
  # loader path. RPATH lets every linked executable run from the build tree or
  # shell.
  # `CAT_ASAN_RUNTIME_PATH` was populated and verified at the top of this file.
  cmake_path(GET CAT_ASAN_RUNTIME_PATH PARENT_PATH _rpath_dir)
  target_link_options(cat INTERFACE "-Wl,-rpath,${_rpath_dir}")
endif()

target_compile_options(cat INTERFACE ${CAT_CXX_FLAGS_ESSENTIAL})
target_link_options(cat INTERFACE ${CAT_LINK_OPTIONS})

# TODO: Implement these symbols in libCat directly.
# `target_link_libraries(cat INTERFACE gcc)`.

# `cat-impl` deliberately does NOT link `cat`. That would drag `_start.cpp`
# (`cat`'s `INTERFACE_SOURCES`) into the archive with the wrong `NO_ARGC_ARGV`
# setting. Its actual usage requirements come in via generator expressions so
# there is still one source of truth on `cat`.
add_library(cat-impl STATIC)
set_target_properties(cat-impl PROPERTIES OUTPUT_NAME cat)
target_compile_features(cat-impl PRIVATE ${CAT_CXX_STANDARD_FEATURE})
set_target_properties(cat-impl PROPERTIES CXX_EXTENSIONS ON)
target_compile_options(cat-impl PRIVATE
  $<TARGET_PROPERTY:cat,INTERFACE_COMPILE_OPTIONS>
  ${CAT_CXX_FLAGS_INTERNAL}
)
target_include_directories(cat-impl PRIVATE
  $<TARGET_PROPERTY:cat,INTERFACE_INCLUDE_DIRECTORIES>
)

add_subdirectory(src/)

# PCH uses `.hpp` only (extensionless API headers go through the regular include
# path via `CAT_HEADER_FILES` in `src/CMakeLists.txt`). Glob is scoped to `src/`
# so the PCH never picks up `tests/unit_tests.hpp`.
file(GLOB_RECURSE CAT_HEADERS "src/*.hpp")

option(CAT_PCH "Enable precompiled-headers for libCat headers." ON)
if (CAT_PCH)
  # `PRIVATE` on `cat-impl` so external consumers of `cat::cat` don't inherit
  # PCH machinery. In-tree tests/examples opt-in via `REUSE_FROM cat-impl`.
  target_precompile_headers(cat-impl PRIVATE ${CAT_HEADERS})
  # Strip the PCH's embedded mtime so sccache / ccache can reuse PCH-consuming
  # TUs across rebuilds. `SHELL:` keeps `-Xclang` glued to its argument.
  target_compile_options(cat-impl PRIVATE
    "SHELL:-Xclang -fno-pch-timestamp"
  )
endif()
set_target_properties(cat-impl PROPERTIES PCH_WARN_INVALID ON)
# Tripwire (deferred): fail configure if `INTERFACE_PRECOMPILE_HEADERS` is ever
# set on `cat`, `cat-impl`, or `cat-impl-shared`. That would export PCH usage
# requirements through `cat::cat[-impl[-shared]]` and bake `cat-impl`'s default
# `gnu::naked` `_start` declaration into every consumer. Fatal for any TU that
# defines `NO_ARGC_ARGV` (`unit_tests`, `hello`, external no-args programs),
# which would then hit "non-asm statement in naked function" on the call to
# `call_main_noargs()`. `REUSE_FROM cat-impl` is unaffected: it writes on the
# consumer target, not on `cat-impl`'s `INTERFACE`.
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
  [=[
Make `cat` link libcat.so instead of libcat.a (implies CAT_BUILD_SHARED).
This primarily exists for testing compatibility with `clang-repl` in CI.
]=] OFF)
if (CAT_USE_SHARED AND NOT CAT_BUILD_SHARED)
  set(CAT_BUILD_SHARED ON CACHE BOOL
    "Build libcat.so (cat-impl-shared) and the clang-repl-libcat wrapper." FORCE)
  message(VERBOSE "CAT_USE_SHARED=ON: forcing CAT_BUILD_SHARED on.")
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

# Source-formatting targets (`cat-format`, `cat-format-check`,
# `cat-restyle-comments`, `cat-restyle-comments-check`).
include(${CMAKE_SOURCE_DIR}/cmake/format.cmake)

# Source-tidying targets (`cat-tidy`, `cat-tidy-check`).
include(${CMAKE_SOURCE_DIR}/cmake/tidy.cmake)

# Compiler optimization report target (`cat-opt-report`).
include(${CMAKE_SOURCE_DIR}/cmake/opt_report.cmake)

# Compiler-IR target (`cat-ir`).
include(${CMAKE_SOURCE_DIR}/cmake/cat_ir.cmake)

# Syntax-only check target (`cat-syntax`).
include(${CMAKE_SOURCE_DIR}/cmake/cat_syntax.cmake)

# Optional configure-time `try_compile` negative checks for
# `cat::basic_int`/`cat::basic_float` / `cat::basic_idx` /
# `cat::basic_intptr` (see
# `cmake/cat_arithmetic_negative.cmake`). Off by default. Included after
# `add_subdirectory(src/)` so include paths are available.
include(${CMAKE_SOURCE_DIR}/cmake/cat_arithmetic_negative.cmake)
