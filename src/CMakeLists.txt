# Create collect all of the files needed to build the `cat-std`
# library in `../CMakeLists.txt`.
set(CAT_SRC ${CMAKE_SOURCE_DIR}/src)

set(
  cat-std-include
  ${CAT_SRC}/
  ${CAT_SRC}/runtime/
  ${CAT_SRC}/compatibility/
  ${CAT_SRC}/libraries/numerals/
  ${CAT_SRC}/libraries/simd/
  ${CAT_SRC}/libraries/result/
  ${CAT_SRC}/libraries/maybe/
  ${CAT_SRC}/libraries/any/
  ${CAT_SRC}/libraries/buffer/
  ${CAT_SRC}/libraries/string/
  ${CAT_SRC}/libraries/memory/
  ${CAT_SRC}/libraries/allocators/
  ${CAT_SRC}/libraries/type_traits/
  ${CAT_SRC}/libraries/concepts/
  ${CAT_SRC}/libraries/utility/
  ${CAT_SRC}/libraries/stdlib/
  ${CAT_SRC}/libraries/unistd/
  PARENT_SCOPE
)

# target_sources(
set(
  cat-std-cpp
  ${CAT_SRC}/runtime/cpp/_start.cpp
  ${CAT_SRC}/runtime/cpp/exit.cpp
  ${CAT_SRC}/runtime/cpp/__stack_chk_fail.cpp
  ${CAT_SRC}/runtime/cpp/align_stack_pointer_16.cpp
  ${CAT_SRC}/runtime/cpp/align_stack_pointer_32.cpp
  ${CAT_SRC}/runtime/cpp/dont_optimize_out.cpp
  ${CAT_SRC}/runtime/cpp/load_argc.cpp
  ${CAT_SRC}/runtime/cpp/load_argv.cpp
  ${CAT_SRC}/runtime/cpp/load_base_stack_pointer.cpp
  ${CAT_SRC}/runtime/cpp/mmap.cpp
  ${CAT_SRC}/runtime/cpp/munmap.cpp
  ${CAT_SRC}/libraries/simd/cpp/is_avx2_supported.cpp
  ${CAT_SRC}/libraries/simd/cpp/is_avx512f_supported.cpp
  ${CAT_SRC}/libraries/simd/cpp/is_avx_supported.cpp
  ${CAT_SRC}/libraries/simd/cpp/is_mmx_supported.cpp
  ${CAT_SRC}/libraries/simd/cpp/is_sse1_supported.cpp
  ${CAT_SRC}/libraries/simd/cpp/is_sse2_supported.cpp
  ${CAT_SRC}/libraries/simd/cpp/is_sse3_supported.cpp
  ${CAT_SRC}/libraries/simd/cpp/is_sse4_1_supported.cpp
  ${CAT_SRC}/libraries/simd/cpp/is_sse4_2_supported.cpp
  ${CAT_SRC}/libraries/simd/cpp/is_sse3_supported.cpp
  ${CAT_SRC}/libraries/simd/cpp/sfence.cpp
  ${CAT_SRC}/libraries/simd/cpp/zero_avx_registers.cpp
  ${CAT_SRC}/libraries/simd/cpp/zero_upper_avx_registers.cpp
  ${CAT_SRC}/libraries/string/cpp/copy_memory.cpp
  ${CAT_SRC}/libraries/string/cpp/is_aligned.cpp
  ${CAT_SRC}/libraries/string/cpp/memcpy.cpp
  ${CAT_SRC}/libraries/string/cpp/strlen.cpp
  ${CAT_SRC}/libraries/unistd/cpp/syscall.cpp
  ${CAT_SRC}/libraries/unistd/cpp/write.cpp
  PARENT_SCOPE
)
