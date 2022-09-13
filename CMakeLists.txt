cmake_minimum_required(VERSION 3.20)

# TODO: Make these apply to specific targets, not globally.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

project(cat LANGUAGES CXX)

option(USE_SANITIZERS "Link ASan and UBSan" ON)

list(
  APPEND CAT_CXX_FLAGS_COMMON
  # Disable linking libC symbols.
  -nostdlib
  -Wno-builtin-declaration-mismatch
  # These are pessimizations to libCat:
  -fno-exceptions -fno-rtti -fno-unwind-tables -fno-asynchronous-unwind-tables
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
  -Wno-unused-function
)

list(
  APPEND CAT_SANITIZER_FLAGS
  -fno-omit-frame-pointer # Required for UBsan.
  -fsanitize=undefined
  -fsanitize=address
  # Asan stack overflow checking does not work in libCat.
  # TODO: Explain why.
  --param asan-stack=0
)

list(
  APPEND CAT_CXX_FLAGS_DEBUG
  -ggdb3 -pipe
)
if(USE_SANITIZERS)
  list(
    APPEND CAT_CXX_FLAGS_DEBUG
    ${CAT_SANITIZER_FLAGS}
  )
endif()

list(
  APPEND CAT_CXX_FLAGS_RELEASE
  -O3
  # Remove unused symbols.
  -ffunction-sections -fdata-sections
  -fvisibility=hidden -fvisibility-inlines-hidden
  -flto -fwhole-file -fno-plt
)

list(
  APPEND CAT_CXX_FLAGS_RELWITHDEBINFO
  ${CAT_CXX_FLAGS_DEBUG}
  ${CAT_CXX_FLAGS_RELEASE}
)

list(
  APPEND CAT_CXX_FLAGS
  ${CAT_CXX_FLAGS_COMMON}
  $<$<CONFIG:Debug>:${CAT_CXX_FLAGS_DEBUG}>
  $<$<CONFIG:RelWithDebInfo>:${CAT_CXX_FLAGS_RELWITHDEBINFO}>
  $<$<CONFIG:Release>:${CAT_CXX_FLAGS_RELEASE}>
)

list(
  APPEND CAT_COMMON_LINK_FLAGS
  # This is required to prevent linking 4 kibibytes of unnecessary symbols.
  -nostdlib
)

list(
  APPEND CAT_RELEASE_LINK_FLAGS
  ${CAT_COMMON_LINK_FLAGS}
  # Remove unused symbols.
  -Wl,-z,noseparate-code,--gc-sections
)

list(
  APPEND CAT_LINK_FLAGS
  $<$<CONFIG:Debug>:${CAT_COMMON_LINK_FLAGS}>
  $<$<CONFIG:RelWithDebInfo>:${CAT_RELEASE_LINK_FLAGS}>
  $<$<CONFIG:RelWithDebInfoNoSan>:${CAT_RELEASE_LINK_FLAGS}>
  $<$<CONFIG:Release>:${CAT_RELEASE_LINK_FLAGS}>
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

# TODO: This adds in code to Hello World.
# Linking gcc is required for SIMD intrinsics.
# target_link_libraries(cat INTERFACE gcc_s)

# Link the address and undefined behavior sanitizers in debug builds.
if (USE_SANITIZERS)
target_link_libraries(
  cat INTERFACE
  $<$<CONFIG:RelWithDebInfo>:asan>
  $<$<CONFIG:RelWithDebInfo>:ubsan>
  $<$<CONFIG:Debug>:asan>
  $<$<CONFIG:Debug>:ubsan>
)
endif()

target_compile_options(cat INTERFACE ${CAT_CXX_FLAGS})
target_link_options(cat INTERFACE ${CAT_LINK_FLAGS})

# Build the tests.
enable_testing()
add_subdirectory(tests/)

# Build the examples.
add_subdirectory(examples/)
