# I have only tested on CMake 3.21, but this likely works on older versions.
cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
project(cat LANGUAGES CXX ASM)

set(CMAKE_CONFIGURATION_TYPES Debug;Release;RelWithDebInfo;RelSmall)

# Link .gdbinit to build directories.
add_custom_target(
  debug_target ALL
  COMMAND ${CMAKE_COMMAND} -E create_symlink
  ${PROJECT_SOURCE_DIR}/.gdbinit $<OUTPUT_CONFIG:$<CONFIG>>/.gdbinit
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}
)

list(
  APPEND
  CAT_CXX_FLAGS_COMMON
  -fno-exceptions -fno-rtti -fno-unwind-tables
  -fomit-frame-pointer # This is required for the stack frame to work.
  -msse4.2
  -mavx2
  -fdiagnostics-color
  -include global_includes.h
)

list(
  APPEND
  CAT_CXX_FLAGS_DEBUG
  -ggdb3 -pipe
  -fstack-protector-strong
  # -fsanitize=undefined
  # -fsanitize=address
  # -fsanitize=leak
  -Wall -Wextra -Wno-unused-function
  -Wno-unused-function
)

# Set aggressive space optimizations. -Os is not favorable, however.
list(
  APPEND
  CAT_CXX_FLAGS_RELEASE
  -no-pie -fno-stack-protector
  -fno-asynchronous-unwind-tables
  -s
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
list(
  APPEND
  CAT_RELEASE_LINK_FLAGS
  ${CAT_COMMON_LINK_FLAGS}
  -no-pie -fno-stack-protector
  -fno-asynchronous-unwind-tables
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
  src/libraries/unistd/detail/syscall0.s
  src/libraries/unistd/detail/syscall1.s
  src/libraries/unistd/detail/syscall2.s
  src/libraries/unistd/detail/syscall3.s
  src/libraries/unistd/detail/syscall4.s
  src/libraries/unistd/detail/syscall5.s
)
# set_target_properties(cat-std PROPERTIES LINKER_LANGUAGE CXX)
target_sources(
  cat-std PRIVATE
  ${CMAKE_SOURCE_DIR}/src/runtime/cpp/_start.cpp
  ${CMAKE_SOURCE_DIR}/src/runtime/cpp/exit.cpp
  ${CMAKE_SOURCE_DIR}/src/runtime/cpp/__stack_chk_fail.cpp
  ${CMAKE_SOURCE_DIR}/src/runtime/cpp/align_stack_pointer_16.cpp
  ${CMAKE_SOURCE_DIR}/src/runtime/cpp/align_stack_pointer_32.cpp
  ${CMAKE_SOURCE_DIR}/src/runtime/cpp/dont_optimize_out.cpp
  ${CMAKE_SOURCE_DIR}/src/runtime/cpp/load_argc.cpp
  ${CMAKE_SOURCE_DIR}/src/runtime/cpp/load_argv.cpp
  ${CMAKE_SOURCE_DIR}/src/runtime/cpp/load_base_stack_pointer.cpp
  ${CMAKE_SOURCE_DIR}/src/runtime/cpp/mmap.cpp
  ${CMAKE_SOURCE_DIR}/src/runtime/cpp/munmap.cpp
  ${CMAKE_SOURCE_DIR}/src/libraries/simd/cpp/is_avx2_supported.cpp
  ${CMAKE_SOURCE_DIR}/src/libraries/simd/cpp/is_avx512f_supported.cpp
  ${CMAKE_SOURCE_DIR}/src/libraries/simd/cpp/is_avx_supported.cpp
  ${CMAKE_SOURCE_DIR}/src/libraries/simd/cpp/is_mmx_supported.cpp
  ${CMAKE_SOURCE_DIR}/src/libraries/simd/cpp/is_sse1_supported.cpp
  ${CMAKE_SOURCE_DIR}/src/libraries/simd/cpp/is_sse2_supported.cpp
  ${CMAKE_SOURCE_DIR}/src/libraries/simd/cpp/is_sse3_supported.cpp
  ${CMAKE_SOURCE_DIR}/src/libraries/simd/cpp/is_sse4_1_supported.cpp
  ${CMAKE_SOURCE_DIR}/src/libraries/simd/cpp/is_sse4_2_supported.cpp
  ${CMAKE_SOURCE_DIR}/src/libraries/simd/cpp/is_sse3_supported.cpp
  ${CMAKE_SOURCE_DIR}/src/libraries/simd/cpp/sfence.cpp
  ${CMAKE_SOURCE_DIR}/src/libraries/simd/cpp/zero_avx_registers.cpp
  ${CMAKE_SOURCE_DIR}/src/libraries/simd/cpp/zero_upper_avx_registers.cpp
  ${CMAKE_SOURCE_DIR}/src/libraries/string/cpp/copy_memory.cpp
  ${CMAKE_SOURCE_DIR}/src/libraries/string/cpp/is_aligned.cpp
  ${CMAKE_SOURCE_DIR}/src/libraries/string/cpp/memcpy.cpp
  ${CMAKE_SOURCE_DIR}/src/libraries/string/cpp/strlen.cpp
  ${CMAKE_SOURCE_DIR}/src/libraries/unistd/cpp/syscall.cpp
  ${CMAKE_SOURCE_DIR}/src/libraries/unistd/cpp/write.cpp
)

target_include_directories(
  cat-std PUBLIC
  src/
  src/runtime/
  src/compatibility/
  src/libraries/numerals/
  src/libraries/simd/
  src/libraries/result/
  src/libraries/maybe/
  src/libraries/any/
  src/libraries/buffer/
  src/libraries/string/
  src/libraries/memory/
  src/libraries/allocators/
  src/libraries/type_traits/
  src/libraries/concepts/
  src/libraries/utility/
  src/libraries/stdlib/
  src/libraries/unistd/
)

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
  -no-pie -fno-stack-protector
  -fno-asynchronous-unwind-tables
  -s
  -ffunction-sections -fdata-sections
  -fvisibility=hidden -fvisibility-inlines-hidden
  -flto -fwhole-file -fno-plt
  -Wl,-z,norelro,-z,noseparate-code,--gc-sections
  $<$<CONFIG:Release>:-O3>
  $<$<CONFIG:RelSmall>:-Os>
)

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
