cmake_minimum_required(VERSION 3.20)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
project(cat LANGUAGES CXX)

set(CMAKE_CONFIGURATION_TYPES Debug;Release;RelWithDebInfo;RelSmall)

list(
  APPEND
  CAT_CXX_FLAGS_COMMON
  # Disable linking libC symbols.
  -nostdlib
  -Wno-builtin-declaration-mismatch
  # These are pessimizations to libCat:
  -fno-exceptions -fno-rtti -fno-unwind-tables -fno-asynchronous-unwind-tables
  -fno-stack-protector # This is required for threads to work.
  # `global_includes.hpp` must be available everywhere.
  -include global_includes.hpp
  # Enable CPU intrinsics:
  -msse4.2
  -mavx2
  -mfma
  -mlzcnt
  -mfsgsbase
  # These flags cause clang-tidy 12 to crash:
  -fconcepts-diagnostics-depth=2
  -fmax-errors=4
  -fdiagnostics-color
  # Enable most warnings.
  -Wall -Wextra
  # The following warnings are counter-productive for a basic-level library:
  -Wno-unused-function
  # `main()` is variadic in libCat, which triggers this warning.
  -Wno-main
)

list(
  APPEND
  CAT_SANITIZER_FLAGS
  -fno-omit-frame-pointer # Required for UBsan.
  -fsanitize=undefined
  -fsanitize=address
  # Stack overflow checking is broken in libCat.
  # TODO: Is that still true? Why?
  --param asan-stack=0
  -fsanitize-recover=all
)

list(
  APPEND
  CAT_CXX_FLAGS_DEBUG
  ${CAT_SANITIZER_FLAGS}
  -ggdb3 -pipe
)

# Set aggressive space optimizations without `-Os`.
list(
  APPEND
  CAT_CXX_FLAGS_RELEASE
  -no-pie
  -ffunction-sections -fdata-sections
  -fvisibility=hidden -fvisibility-inlines-hidden
  -flto -fwhole-file -fno-plt
  -Wl,-z,noseparate-code,--gc-sections
)

list(
  APPEND
  CAT_CXX_FLAGS_RELWITHDEBINFO
  ${CAT_SANITIZER_FLAGS}
  -O3 -ggdb3 -pipe
  -no-pie
  -ffunction-sections -fdata-sections
  -fvisibility=hidden -fvisibility-inlines-hidden
  -flto -fwhole-file -fno-plt
  -Wl,-z,noseparate-code,--gc-sections
)

list(
  APPEND
  CAT_CXX_FLAGS_RELFAST
  ${CAT_CXX_FLAGS_RELEASE}
  -O3
)

list(
  APPEND
  CAT_CXX_FLAGS_RELSMAL
  ${CAT_CXX_FLAGS_RELEASE}
  -Os
)

list(
  APPEND CAT_CXX_FLAGS
  ${CAT_CXX_FLAGS_COMMON}
  $<$<CONFIG:Debug>:${CAT_CXX_FLAGS_DEBUG}>
  $<$<CONFIG:RelWithDebInfo>:${CAT_CXX_FLAGS_RELWITHDEBINFO}>
  $<$<CONFIG:Release>:${CAT_CXX_FLAGS_RELFAST}>
  $<$<CONFIG:RelSmall>:${CAT_CXX_FLAGS_RELSMALL}>
)

list(
  APPEND
  CAT_COMMON_LINK_FLAGS
  # This is required to prevent linking 4 kibibytes of unnecessary symbols.
  -nostdlib
)

list(
  APPEND
  CAT_DEBUG_LINK_FLAGS
  # -Wl,--gdb-index
)

# I can't figure out which flags are for the linker and which
# are for the compiler, so I just send them all to both.
# TODO: Improve this.
list(
  APPEND
  CAT_RELEASE_LINK_FLAGS
  ${CAT_COMMON_LINK_FLAGS}
  -no-pie
  -ffunction-sections -fdata-sections
  -fvisibility=hidden -fvisibility-inlines-hidden
  -flto -fwhole-file -fno-plt
  -Wl,-z,noseparate-code,--gc-sections
)

# I can't figure out which flags are for the linker and which
# are for the compiler, so I just send them all to both.
# TODO: Improve this.
list(
  APPEND
  CAT_RELEASE_DEBUG_LINK_FLAGS
  ${CAT_RELEASE_LINK_FLAGS}
  ${CAT_DEBUG_LINK_FLAGS}
)

list(
  APPEND CAT_LINK_FLAGS
  $<$<CONFIG:Debug>:${CAT_COMMON_LINK_FLAGS}>
  $<$<CONFIG:RelWithDebInfo>:${CAT_RELEASE_DEBUG_LINK_FLAGS}>
  $<$<CONFIG:Release>:${CAT_RELEASE_LINK_FLAGS}>
  $<$<CONFIG:RelSmall>:${CAT_RELEASE_LINK_FLAGS}>
)

add_library(cat INTERFACE)
set_target_properties(
  cat
  PROPERTIES
  OUTPUT_NAME "cat"
)

# `src/CMakeLists.txt` holds the recipes for `cat-cpp` and `cat-include`.
add_subdirectory(src/)

target_sources(cat INTERFACE ${cat-cpp})
target_include_directories(cat INTERFACE ${cat-include})

# Linking gcc is required for SIMD intrinsics.
target_link_libraries(cat INTERFACE gcc)

# TODO: Make these be automated optional dependencies.
# Link the address and undefined behavior sanitizers in debug builds.
target_link_libraries(
  cat INTERFACE
  $<$<CONFIG:RelWithDebInfo>:asan>
  $<$<CONFIG:RelWithDebInfo>:ubsan>
  $<$<CONFIG:Debug>:asan>
  $<$<CONFIG:Debug>:ubsan>
)

target_compile_options(cat INTERFACE ${CAT_CXX_FLAGS})
target_link_options(cat INTERFACE ${CAT_LINK_FLAGS})

# Build the tests.
enable_testing()
add_subdirectory(tests/)

# Build the examples.
add_subdirectory(examples/)
