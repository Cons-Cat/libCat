# Multi-config-only `.gdbinit` symlink (single-config slot is taken by
# `cat-gdb-examples`). See `cmake/gdb_symlink.cmake`.
cat_gdb_symlink(cat-gdb-tests)

# `cat-tests-base` holds the flags/links needed to match `cat-impl`'s compile
# commands (so `REUSE_FROM cat-impl` passes PCH validation) but no test sources.
# A focused single-test binary would link `cat-tests-base` and add only its
# own source. The bulk `unit_tests` binary pulls everything via `cat-tests`
# below.
add_library(cat-tests-base INTERFACE) # Do not produce a `.so` or `.a` artifact.
target_link_libraries(cat-tests-base INTERFACE cat)

# In-tree tests need `cat-impl`'s `INTERNAL` flags so `REUSE_FROM cat-impl`
# below passes the Clang PCH validator.
target_compile_options(cat-tests-base INTERFACE
  ${CAT_COMPILE_OPTIONS_INTERNAL}
  # Tests should fail if they produce sanitizer warnings.
  # TODO: Re-enable this when threads consistently work.
  # -fno-sanitize-recover=all
  # Tests should be allowed to have unused variables.
  -Wno-unused-variable
  -Wno-self-assign-overloaded
)

# Umbrella that adds every test source on top of `cat-tests-base`.
add_library(cat-tests INTERFACE)
target_link_libraries(cat-tests INTERFACE cat-tests-base)

# Toggle to disable compiling unit tests.
option(CAT_BUILD_UNIT_TESTS "Compile all unit tests." ON)

if(CAT_BUILD_UNIT_TESTS)
  # Include path for `unit_tests.hpp`, on the shared base since multiple
  # binaries include it.
  target_include_directories(cat-tests-base INTERFACE ${CMAKE_SOURCE_DIR}/tests/)

  # Add unit test implementation files.
  # Specifying each source file individually makes disabling tests convenient.
  target_sources(
    cat-tests INTERFACE
	  ${CMAKE_SOURCE_DIR}/tests/src/test_meta.cpp
    ${CMAKE_SOURCE_DIR}/tests/src/test_iterator.cpp
    ${CMAKE_SOURCE_DIR}/tests/src/test_arithmetic.cpp
    ${CMAKE_SOURCE_DIR}/tests/src/test_alloc.cpp
    ${CMAKE_SOURCE_DIR}/tests/src/test_arrays.cpp
    ${CMAKE_SOURCE_DIR}/tests/src/test_simd.cpp
    ${CMAKE_SOURCE_DIR}/tests/src/test_simd_sse_avx.cpp
    ${CMAKE_SOURCE_DIR}/tests/src/test_compare_strings.cpp
    ${CMAKE_SOURCE_DIR}/tests/src/test_string_length.cpp
    ${CMAKE_SOURCE_DIR}/tests/src/test_format_strings.cpp
    ${CMAKE_SOURCE_DIR}/tests/src/test_linear_allocator.cpp
    ${CMAKE_SOURCE_DIR}/tests/src/test_pool_allocator.cpp
    ${CMAKE_SOURCE_DIR}/tests/src/test_reallocation_disjoint.cpp
    ${CMAKE_SOURCE_DIR}/tests/src/test_list.cpp
    ${CMAKE_SOURCE_DIR}/tests/src/test_math.cpp
    ${CMAKE_SOURCE_DIR}/tests/src/test_maybe.cpp
    ${CMAKE_SOURCE_DIR}/tests/src/test_paging_memory.cpp
    # TODO: I might remove this feature.
    # ${CMAKE_SOURCE_DIR}/tests/src/test_raii.cpp
    ${CMAKE_SOURCE_DIR}/tests/src/test_typelist.cpp
    ${CMAKE_SOURCE_DIR}/tests/src/test_scaredy.cpp
    ${CMAKE_SOURCE_DIR}/tests/src/test_set_memory.cpp
    ${CMAKE_SOURCE_DIR}/tests/src/test_tuple.cpp
    ${CMAKE_SOURCE_DIR}/tests/src/test_variant.cpp
    ${CMAKE_SOURCE_DIR}/tests/src/test_vec.cpp
    # TODO: This container needs a lot of work.
    # ${CMAKE_SOURCE_DIR}/tests/src/test_ring.cpp
    ${CMAKE_SOURCE_DIR}/tests/src/test_invoke.cpp
    ${CMAKE_SOURCE_DIR}/tests/src/test_cast.cpp
    ${CMAKE_SOURCE_DIR}/tests/src/test_copy_memory.cpp
    ${CMAKE_SOURCE_DIR}/tests/src/test_bit.cpp
    ${CMAKE_SOURCE_DIR}/tests/src/test_bitset.cpp
    ${CMAKE_SOURCE_DIR}/tests/src/test_stringify.cpp
    # `test_cpuid.cpp` asserts a literal microarch (`__builtin_cpu_is("skylake")`),
    # so it is built standalone below and only registered with `ctest` under
    # Intel SDE pinned to `-skl`.
    ${CMAKE_SOURCE_DIR}/tests/src/test_futex.cpp
    ${CMAKE_SOURCE_DIR}/tests/src/test_thread.cpp
  )

  add_executable(unit_tests unit_tests.cpp)

  # Pin `NO_ARGC_ARGV` to `_start.cpp` only (keeps the rest of `unit_tests`'s
  # compile commands byte-equal to `cat-impl`'s so `REUSE_FROM cat-impl` below
  # passes PCH validation). `SKIP_PRECOMPILE_HEADERS` on the same TU because
  # `cat-impl`'s PCH bakes `_start` as `gnu::naked` and reusing it here would
  # leave the no-args body all-asm ("non-ASM statement in naked function").
  set_source_files_properties(
    ${CMAKE_SOURCE_DIR}/src/libraries/runtime/implementations/_start.cpp
    TARGET_DIRECTORY unit_tests
    PROPERTIES
      COMPILE_DEFINITIONS "NO_ARGC_ARGV"
      SKIP_PRECOMPILE_HEADERS ON)
  target_link_libraries(unit_tests PRIVATE cat-tests)
  # `CAT_LINK_OPTIONS` arrives transitively from `cat`'s `INTERFACE_LINK_OPTIONS`.
  if(CAT_PCH)
    target_precompile_headers(unit_tests REUSE_FROM cat-impl)
  endif()

  # Enable running `unit_tests` through `ctest`.
  add_test(NAME UnitTests COMMAND unit_tests)

  # # Parked: standalone `test_cpuid` runner. Linking `cat-tests-base` skips the
  # # bulk source list so only `test_cpuid` registers. Re-enable alongside the
  # # SDE step in `ci.yml`.
  # add_executable(
  #   unit_tests_cpuid
  #   unit_tests.cpp
  #   ${CMAKE_SOURCE_DIR}/tests/src/test_cpuid.cpp)
  # set_source_files_properties(
  #   ${CMAKE_SOURCE_DIR}/src/libraries/runtime/implementations/_start.cpp
  #   TARGET_DIRECTORY unit_tests_cpuid
  #   PROPERTIES COMPILE_DEFINITIONS "NO_ARGC_ARGV")
  # target_link_libraries(unit_tests_cpuid PRIVATE cat-tests-base)
  # if(CAT_PCH)
  #   target_precompile_headers(unit_tests_cpuid REUSE_FROM cat-impl)
  # endif()
  #
  # find_program(CAT_SDE_EXECUTABLE
  #   NAMES sde64 sde
  #   DOC "Path to the Intel Software Development Emulator binary.")
  # if(CAT_SDE_EXECUTABLE)
  #   add_test(
  #     NAME UnitTestsCpuid
  #     COMMAND ${CAT_SDE_EXECUTABLE} -skl -- $<TARGET_FILE:unit_tests_cpuid>)
  # endif()
else()
  # `.gdbinit` symlink needs at least one target. Link `cat-tests` so this
  # placeholder also exercises `REUSE_FROM cat-impl`.
  add_executable(test_dummy ${CMAKE_SOURCE_DIR}/tests/src/test_dummy.cpp)
  target_link_libraries(test_dummy PRIVATE cat-tests)
  if(CAT_PCH)
    target_precompile_headers(test_dummy REUSE_FROM cat-impl)
  endif()
endif()
