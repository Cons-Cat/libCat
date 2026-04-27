# This file is flagrantly "vibe-coded". It may not be up to the standards of most libCat code.

# `cat-impl-shared` -- `libcat.so` opt-in (`CAT_BUILD_SHARED`)
#
# Canonical libCat artifact is `libcat.a` (`cat-impl`). This file adds a
# parallel `SHARED` variant: anything that links `cat` can instead link
# `cat-impl-shared`, or via the `CAT_USE_SHARED` switch keep linking
# `cat` and have it transparently resolve to the `.so`.
#
# `clang-repl` originally motivated the target -- its `%lib` directive
# uses `dlopen`, which cannot load a static archive. The wrapper script
# generated at the bottom of this file
# (`${CMAKE_BINARY_DIR}/clang-repl-libcat`) packages the JIT flow
# end-to-end, but the `.so` is also independently useful for installed
# consumers via `find_package(cat) -> cat::cat-impl-shared`.
#
# Reuses `cat-impl`'s sources and essential flags verbatim, plus four
# specialisations:
#   * `-fPIC` everywhere -- mandatory for a shared object.
#   * `-fno-lto`, because LLD would otherwise internalise libCat's
#     entire public API (no static consumer is visible at link time).
#   * compiler-rt's PIC builtins archive, for `__cpu_model` referenced
#     by `__builtin_cpu_supports` in the SIMD shims. Under `-nostdlib`
#     `dlopen` would otherwise reject the `.so` for an undefined
#     symbol.
#   * `libcat.ld` via `-T`, defining the `__cat_tls_*` linker-script
#     symbols `executable_tls.cpp` reads (same `dlopen` failure mode
#     without it).
#
# Usage requirements (essentials, include dirs, link options,
# `_start.cpp`, linker-script `LINK_DEPENDS`) propagate via INTERFACE,
# so consumers get a complete build environment with no flag
# duplication. `CMAKE_EXPORT_COMPILE_COMMANDS` is forced on so the
# `clang-repl-libcat` wrapper has a flag database to read.
# `INSTALL_INTERFACE` link options below reference `CMAKE_INSTALL_FULL_DATADIR`,
# which `GNUInstallDirs` (re)computes from the current `CMAKE_INSTALL_PREFIX`.
# Re-include here so the value is correct regardless of which subdirectory
# already pulled the module in.

include(GNUInstallDirs)

# Use a major version derived from `CMAKE_CXX_COMPILER_VERSION` to look up
# `clang-repl-NN`, mirroring the strategy in `llvm_archivers.cmake`.
string(REGEX MATCH "^[0-9]+" _cat_clang_major "${CMAKE_CXX_COMPILER_VERSION}")

execute_process(
  COMMAND "${CMAKE_CXX_COMPILER}" --rtlib=compiler-rt -print-libgcc-file-name
  OUTPUT_VARIABLE _cat_compiler_rt_builtins
  OUTPUT_STRIP_TRAILING_WHITESPACE
  ERROR_QUIET)
if (NOT _cat_compiler_rt_builtins OR NOT EXISTS "${_cat_compiler_rt_builtins}")
  message(WARNING
    "CAT_BUILD_SHARED: could not locate compiler-rt builtins via "
    "`${CMAKE_CXX_COMPILER} --rtlib=compiler-rt -print-libgcc-file-name`. "
    "`libcat.so` will likely fail to link with undefined `__cpu_model`.")
endif()

# Reuse `cat-impl`'s source list verbatim so adding a `.cpp` to the archive
# automatically rebuilds it into the shared object too.
get_property(_cat_impl_sources_for_so TARGET cat-impl PROPERTY SOURCES)
add_library(cat-impl-shared SHARED ${_cat_impl_sources_for_so})
set_target_properties(cat-impl-shared PROPERTIES
  OUTPUT_NAME cat
  POSITION_INDEPENDENT_CODE ON)

# `PUBLIC` for the props that are usage requirements (essentials, include
# dirs, link options): the `.so`'s own TUs need them, and any consumer that
# links `cat-impl-shared` does too. `PRIVATE` for the build-only specialisations
# (internal warnings / codegen, `-fno-lto`, `-fvisibility=default`) so they
# never leak into a downstream's compile command.
target_compile_options(cat-impl-shared
  PUBLIC
    $<TARGET_PROPERTY:cat,INTERFACE_COMPILE_OPTIONS>
  PRIVATE
    ${CAT_COMPILE_OPTIONS_INTERNAL}
    # Override `-flto=auto` / `-flto=thin` from `CAT_COMPILE_OPTIONS_INTERNAL`
    # so LLD does not internalise the public API at link time.
    -fno-lto
    # Release-without-sanitizers adds `-fvisibility=hidden`
    # `-fvisibility-inlines-hidden` (in the top-level CMakeLists, alongside
    # `-flto=auto`) to keep `libcat.a`'s symbol set tight, which strips
    # libCat's entire ABI from the `.so` if not undone here. Trailing
    # `-fvisibility=default` wins for the non-inline definitions
    # `clang-repl` actually needs to look up. `-fvisibility-inlines-hidden`
    # has no inverse switch, but inline functions are inlined at the JIT
    # call site so their visibility is moot. Debug / RelWithDebInfo / any
    # `CAT_USE_SANITIZERS=ON` build never sees the hidden flag in the first
    # place, so this override is a no-op there.
    -fvisibility=default)

target_include_directories(cat-impl-shared PUBLIC
  $<TARGET_PROPERTY:cat,INTERFACE_INCLUDE_DIRECTORIES>)

target_link_options(cat-impl-shared
  PUBLIC
    $<TARGET_PROPERTY:cat,INTERFACE_LINK_OPTIONS>
    # `libcat.ld` is needed at BOTH ends:
    #   * the `.so`'s own link, so it has internal `__cat_tls_*` for its
    #     own (typically empty) TLS layout, and
    #   * a downstream executable's link, so its `__cat_tls_*` describe the
    #     executable's `.tdata` / `.tbss` (where `thread_local` data from
    #     consumer TUs lives). At runtime ELF symbol resolution prefers the
    #     executable's copies, so libCat's `executable_tls.cpp` always
    #     reports the executable's TLS size when called from the `.so`.
    # `BUILD_INTERFACE` / `INSTALL_INTERFACE` so installed consumers pick
    # up the script's installed copy under `${CMAKE_INSTALL_DATADIR}/libcat`.
    "$<BUILD_INTERFACE:-Wl,-T,${CMAKE_SOURCE_DIR}/src/libcat.ld>"
    "$<INSTALL_INTERFACE:-Wl,-T,${CMAKE_INSTALL_FULL_DATADIR}/libcat/libcat.ld>"
    # The `INSERT AFTER .text` directive in `libcat.ld` interleaves
    # `.tdata` between RELRO-backed sections, which `lld` rejects on any
    # link that also pulls in a `.so` (the `.so`'s `.got` entries widen
    # the RELRO region). The same flag is added under `CAT_USE_SANITIZERS`
    # in the top-level CMakeLists for the ASan-runtime `.so`. Repeating
    # it here keeps `CAT_USE_SHARED` builds working without sanitizers.
    -Wl,-z,norelro
  PRIVATE
    -fno-lto)

# Pull libc / libm into both `libcat.so`'s `DT_NEEDED` AND any consumer
# executable's link line. libCat shims a small set of libc / libm symbols
# (`memset`, `memcpy`, `strlen`, `sqrt`, `sqrtf`, `pow`, `powf`) in
# `src/libraries/{string,math}/implementations/`, but consumers can still
# end up referencing other libc / libm symbols (e.g. compiler-emitted
# `printf`, `sin`, `cos`, `log`); explicit `-lm -lc` makes those
# resolvable both at the `.so`'s own link and at the consumer's. PUBLIC is
# required because `lld` does not follow a shared library's `DT_NEEDED`
# entries when resolving the consumer's undefined symbols, so each consumer
# needs `-lm -lc` repeated on its own link line.
target_link_options(cat-impl-shared PUBLIC
  -lm
  -lc)

# `_start.cpp` lives on `cat`'s INTERFACE_SOURCES (each consumer compiles it
# with their own `NO_ARGC_ARGV`). Mirror the entry on `cat-impl-shared` so
# consumers that link the `.so` directly (or via `cat`-with-`CAT_USE_SHARED`)
# still get a valid program entry point.
target_sources(cat-impl-shared INTERFACE
  $<TARGET_PROPERTY:cat,INTERFACE_SOURCES>)

# In-tree consumers relink when the script changes; the installed copy
# is referenced by absolute path in the `INSTALL_INTERFACE` link option
# above and CMake does not propagate `LINK_DEPENDS` across the install
# boundary, so this is `BUILD_INTERFACE`-only.
set_property(TARGET cat-impl-shared APPEND PROPERTY INTERFACE_LINK_DEPENDS
  "$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/libcat.ld>")

if (_cat_compiler_rt_builtins AND EXISTS "${_cat_compiler_rt_builtins}")
  target_link_libraries(cat-impl-shared PRIVATE "${_cat_compiler_rt_builtins}")
endif()

# Ensure the `.so`'s own relink picks up the script.
set_target_properties(cat-impl-shared PROPERTIES
  LINK_DEPENDS "${CMAKE_SOURCE_DIR}/src/libcat.ld")

if (CAT_PCH)
  # `REUSE_FROM cat-impl` is rejected because Clang treats `-fPIC` as a
  # PCH-relevant frontend choice (`is pie differs in precompiled file`),
  # so `cat-impl-shared` gets its own PCH built with the same `-fPIC`
  # flag the rest of its TUs use. The extra PCH compile costs ~1s once
  # but every `cat-impl-shared` source then skips re-parsing libCat's
  # headers, which is a much bigger saving across the 88-TU build.
  target_precompile_headers(cat-impl-shared PRIVATE ${CAT_HEADERS})
  target_compile_options(cat-impl-shared PRIVATE
    "SHELL:-Xclang -fno-pch-timestamp")
  set_target_properties(cat-impl-shared PROPERTIES PCH_WARN_INVALID ON)
endif()

unset(_cat_compiler_rt_builtins)
unset(_cat_impl_sources_for_so)

# The wrapper derives its `--Xcc` flag set from `compile_commands.json`,
# so make sure CMake emits one. Forced because the wrapper hard-fails
# without it.
if (NOT CMAKE_EXPORT_COMPILE_COMMANDS)
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL
    "Export compile_commands.json (required by clang-repl-libcat)." FORCE)
  message(STATUS
    "CAT_BUILD_SHARED: enabling CMAKE_EXPORT_COMPILE_COMMANDS for clang-repl-libcat.")
endif()

find_program(CAT_CLANG_REPL_EXECUTABLE
  NAMES "clang-repl-${_cat_clang_major}" clang-repl
  DOC "`clang-repl` matching ${CMAKE_CXX_COMPILER}.")
if (NOT CAT_CLANG_REPL_EXECUTABLE)
  message(WARNING
    "CAT_BUILD_SHARED: `clang-repl-${_cat_clang_major}` / `clang-repl` not "
    "found on PATH. The clang-repl-libcat wrapper will be generated but "
    "won't run until clang-repl is installed.")
  set(CAT_CLANG_REPL_EXECUTABLE "clang-repl-${_cat_clang_major}")
endif()

# When sanitizers are on, `libcat.so` carries unresolved ASan symbols
# (`__asan_init`, `__asan_register_elf_globals`, ...). `clang-repl` itself
# is not asanified, so those references stay unresolved unless the ASan
# runtime is `LD_PRELOAD`'d into the REPL process. The wrapper handles
# this automatically when `CAT_ASAN_RUNTIME_PATH` is non-empty. The path
# is already probed and verified at the top of the root CMakeLists.txt;
# clear it here when sanitizers are off so the generated wrapper does
# not preload asan into a non-asanified `libcat.so`.
if (NOT CAT_USE_SANITIZERS)
  set(CAT_ASAN_RUNTIME_PATH "")
endif()

configure_file(
  "${CMAKE_SOURCE_DIR}/cmake/clang_repl_libcat.in.sh"
  "${CMAKE_BINARY_DIR}/clang-repl-libcat"
  @ONLY)
file(CHMOD "${CMAKE_BINARY_DIR}/clang-repl-libcat"
  PERMISSIONS
    OWNER_READ OWNER_WRITE OWNER_EXECUTE
    GROUP_READ GROUP_EXECUTE
    WORLD_READ WORLD_EXECUTE)

# `USES_TERMINAL` puts the command in Ninja's `console` pool so stdin /
# stdout pass through unbuffered -- required for an interactive REPL. The
# explicit `add_dependencies` ensures `libcat.so` exists before the
# wrapper tries to `%lib`-load it (the wrapper's existence is configure-
# time, but the `.so` only appears after `cat-impl-shared` builds).
add_custom_target(cat-repl
  COMMAND "${CMAKE_BINARY_DIR}/clang-repl-libcat"
  USES_TERMINAL
  COMMENT "Launching clang-repl with libCat preloaded.")
add_dependencies(cat-repl cat-impl-shared)

unset(_cat_clang_major)
