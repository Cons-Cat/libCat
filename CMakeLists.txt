cmake_minimum_required(VERSION 3.21)
project(kitten LANGUAGES CXX ASM)

set(CMAKE_CXX_FLAGS
  "-nostdlib -fno-exceptions -fno-rtti -fno-unwind-tables \
   -fno-asynchronous-unwind-tables -std=gnu++20")

# Set aggressive space optimizations. -Os is not favorable, however.
set(CMAKE_CXX_FLAGS_RELEASE
  "-flto -fwhole-file -Wl,-z,norelro,-z,noseparate-code,--gc-sections \
   -ffunction-sections -fdata-sections -static")

add_library(kitten-std-static STATIC
  src/crt/crt1.S
  )

add_library(kitten-std-headers INTERFACE
  )
target_include_directories(kitten-std-headers INTERFACE
  src/unistd/
  src/cstdlib/
  src/cstdint/
  )

# Combine static objects and C++ headers for convenience.
add_library(kitten-std INTERFACE)
target_link_libraries(kitten-std INTERFACE kitten-std-static)
target_link_libraries(kitten-std INTERFACE kitten-std-headers)

add_executable(hello examples/hello.cpp)
target_link_libraries(hello PRIVATE kitten-std)
