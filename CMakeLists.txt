cmake_minimum_required(VERSION 3.20)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
project(cat LANGUAGES CXX ASM)

set(CMAKE_CONFIGURATION_TYPES Debug;Release;RelWithDebInfo;RelSmall)

list(
  APPEND
  CAT_CXX_FLAGS_COMMON
  -fno-exceptions -fno-rtti -fno-unwind-tables -fno-asynchronous-unwind-tables
  -fomit-frame-pointer # This is required for the stack frame to work.
  -fno-stack-protector # This is required for threads to work.
  -msse4.2 # -march=native does not compile on some computers, apparently.
  -mavx2
  -mfsgsbase
  -fdiagnostics-color
  -include global_includes.h
  # This is enabled by default, but it's counter-productive to libCat's vision.
  -Wno-builtin-declaration-mismatch
)

list(
  APPEND
  CAT_CXX_FLAGS_DEBUG
  -ggdb3 -pipe
  # -fsanitize=undefined
  # -fsanitize=address
  # -fsanitize=leak
  -Wall -Wextra -Wno-unused-function
  # The following warnings are counter-productive for a basic-level library:
  -Wno-unused-function
)

# Set aggressive space optimizations. -Os is not favorable, however.
list(
  APPEND
  CAT_CXX_FLAGS_RELEASE
  -no-pie
  -ffunction-sections -fdata-sections
  -fvisibility=hidden -fvisibility-inlines-hidden
  -flto -fwhole-file -fno-plt
  -Wl,-z,norelro,-z,noseparate-code,--gc-sections
)

list(
  APPEND
  CAT_CXX_FLAGS_RELFAST
  ${CAT_CXX_FLAGS_RELEASE}
  -O3
)

list(
  APPEND
  CAT_CXX_FLAGS_RELWITHDEBINFO
  ${CAT_CXX_FLAGS_DEBUG}
  ${CAT_CXX_FLAGS_RELFAST}
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
  -nostdlib
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
  -Wl,-z,norelro,-z,noseparate-code,--gc-sections
)

list(
  APPEND CAT_LINK_FLAGS
  $<$<CONFIG:Debug>:${CAT_COMMON_LINK_FLAGS}>
  $<$<CONFIG:RelWithDebInfo>:${CAT_COMMON_LINK_FLAGS}>
  $<$<CONFIG:Release>:${CAT_RELEASE_LINK_FLAGS}>
  $<$<CONFIG:RelSmall>:${CAT_RELEASE_LINK_FLAGS}>
)

add_library(
  cat-std STATIC
  src/libraries/thread/clone.s
)

# src/CMakeLists.txt holds the recipes for `cat-std-cpp`
# and `cat-std-include`.
add_subdirectory(src/)

target_sources(cat-std PUBLIC ${cat-std-cpp})
target_include_directories(cat-std PUBLIC ${cat-std-include})

target_link_libraries(cat-std PUBLIC gcc)
# target_link_libraries(
#   cat-std INTERFACE
#   # $<$<CONFIG:RelWithDebInfo>:asan>
#   # $<$<CONFIG:RelWithDebInfo>:ubsan>
#   # $<$<CONFIG:Debug>:asan>
#   # $<$<CONFIG:Debug>:ubsan>
# )

target_compile_options(cat-std PUBLIC ${CAT_CXX_FLAGS})
target_link_options(cat-std PUBLIC ${CAT_LINK_FLAGS})

# Link .gdbinit to build directories.
add_custom_target(
  cat-debug ALL
  COMMAND ${CMAKE_COMMAND} -E create_symlink
  ${PROJECT_SOURCE_DIR}/.gdbinit $<OUTPUT_CONFIG:$<CONFIG>>/.gdbinit
  # DEPENDS ${CMAKE_CURRENT_BINARY_DIR}
)
add_dependencies(cat-std cat-debug)

# Build the tests.
enable_testing()
add_subdirectory(tests/)

# Build the examples.
add_executable(hello examples/hello.cpp)
target_link_libraries(hello PRIVATE cat-std)

add_executable(echo examples/echo.cpp)
target_compile_options(echo PRIVATE ${CAT_CXX_FLAGS})
target_link_libraries(echo PRIVATE cat-std)
target_link_options(echo PRIVATE ${CAT_LINK_FLAGS})

add_executable(memcpy examples/memory_copy.cpp)
target_compile_options(memcpy PRIVATE ${CAT_CXX_FLAGS})
target_link_libraries(memcpy PRIVATE cat-std)
target_link_options(memcpy PRIVATE ${CAT_LINK_FLAGS})

# libC executables are only configured for Release
list(
  APPEND
  LIBC_RELEASE_FLAGS
  -fno-exceptions -fno-rtti -fno-unwind-tables
  -fno-asynchronous-unwind-tables
  -no-pie
  -ffunction-sections -fdata-sections
  -fvisibility=hidden -fvisibility-inlines-hidden
  -flto -fwhole-file -fno-plt
  -Wl,-z,norelro,-z,noseparate-code,--gc-sections
  $<$<CONFIG:Release>:-O3>
  $<$<CONFIG:RelSmall>:-Os>
)

# Do not build libC examples by default.
option(BUILD_LIBC_EXAMPLES "Enable libC examples to compare against libCat." OFF)

if(BUILD_LIBC_EXAMPLES)
  add_executable(hello_libc examples/hello_libc.cpp)
  target_compile_options(
    hello_libc PRIVATE
    ${LIBC_RELEASE_FLAGS}
  )
  target_link_options(
    hello_libc PRIVATE
    ${LIBC_RELEASE_FLAGS}
  )

  add_executable(memcpy_libc examples/memory_copy_libc.cpp)
  target_compile_options(
    memcpy_libc PRIVATE
    ${LIBC_RELEASE_FLAGS}
  )
  target_link_options(
    memcpy_libc PRIVATE
    ${LIBC_RELEASE_FLAGS}
  )
endif()
