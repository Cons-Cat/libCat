# Collect all of the files needed to build the `cat` library in
# `../CMakeLists.txt`.

set(CATLIB ${CMAKE_SOURCE_DIR}/src/libraries)
include(GNUInstallDirs)

# Per-library subdirs on the include path. Hoisted (and re-exposed to parent
# scope) so `cat_install.cmake`'s `install(DIRECTORY)` uses the same list.
set(CAT_INCLUDE_SUBDIRS
  meta compare limits runtime cpuid sanitizer arithmetic initializer_list
  simd algorithm debug variant scaredy maybe match notype unique collection
  array span list string tui format memory allocator utility functional file
  linux thread atomic socket x11 bit bitset iterator math type_list tuple
  vec ring enum)
set(CAT_INCLUDE_SUBDIRS ${CAT_INCLUDE_SUBDIRS} PARENT_SCOPE)

# Each entry has `BUILD_INTERFACE` (in-tree path) and `INSTALL_INTERFACE`
# (installed path) so `cat::cat` works for both `add_subdirectory` and
# `find_package(cat)` consumers.
target_include_directories(cat INTERFACE
  "$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/src/>"
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/libcat>")
foreach(_cat_subdir IN LISTS CAT_INCLUDE_SUBDIRS)
  target_include_directories(cat INTERFACE
    "$<BUILD_INTERFACE:${CATLIB}/${_cat_subdir}/>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/libcat/libraries/${_cat_subdir}>")
endforeach()
unset(_cat_subdir)

# `libcat.ld` defines `__cat_tls_*` symbols against the linkee's own TLS, so
# `.a` consumers need `-Wl,-T,libcat.ld` at link time. `.so` bakes the same
# definitions in during its own link, so its consumers don't. Hanging this on
# `cat-impl` (rather than `cat`) means `CAT_USE_SHARED` automatically swaps
# the `-T` along with the impl.
set(LIBCAT_LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/src/libcat.ld")
target_link_options(cat-impl INTERFACE
  "$<BUILD_INTERFACE:-Wl,-T,${LIBCAT_LINKER_SCRIPT}>"
  "$<INSTALL_INTERFACE:-Wl,-T,${CMAKE_INSTALL_FULL_DATADIR}/libcat/libcat.ld>")
set_property(TARGET cat-impl APPEND PROPERTY INTERFACE_LINK_DEPENDS
  "$<BUILD_INTERFACE:${LIBCAT_LINKER_SCRIPT}>")

# `_start.cpp` stays on `cat` `INTERFACE` because consumers select between its
# argc/argv vs. no-args branches via `NO_ARGC_ARGV`. All other impls are
# flag-independent and live in `cat-impl` (`libcat.a`).
target_sources(cat INTERFACE
  "$<BUILD_INTERFACE:${CATLIB}/runtime/implementations/_start.cpp>"
  "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/libcat/libraries/runtime/implementations/_start.cpp>"
)

# TODO: Group and annotate these lines by their corresponding header.
target_sources(cat-impl PRIVATE
  ${CATLIB}/runtime/implementations/exit.cpp
  ${CATLIB}/runtime/implementations/__stack_chk_fail.cpp
  ${CATLIB}/runtime/implementations/__cxa_atexit.cpp
  ${CATLIB}/runtime/implementations/__cxa_pure_virtual.cpp
  ${CATLIB}/runtime/implementations/setjmp.cpp
  ${CATLIB}/runtime/implementations/longjmp.cpp
  ${CATLIB}/runtime/implementations/load_base_stack_pointer.cpp
  ${CATLIB}/new/implementations/new.cpp
  ${CATLIB}/simd/implementations/is_avx2_supported.cpp
  ${CATLIB}/simd/implementations/is_avx512f_supported.cpp
  ${CATLIB}/simd/implementations/is_avx_supported.cpp
  ${CATLIB}/simd/implementations/is_mmx_supported.cpp
  ${CATLIB}/simd/implementations/is_sse1_supported.cpp
  ${CATLIB}/simd/implementations/is_sse2_supported.cpp
  ${CATLIB}/simd/implementations/is_sse3_supported.cpp
  ${CATLIB}/simd/implementations/is_sse4_1_supported.cpp
  ${CATLIB}/simd/implementations/is_sse4_2_supported.cpp
  ${CATLIB}/math/implementations/pow.cpp
  ${CATLIB}/math/implementations/sqrt.cpp
  ${CATLIB}/memory/implementations/copy_memory.cpp
  ${CATLIB}/memory/implementations/copy_memory_backward.cpp
  ${CATLIB}/string/implementations/memcpy.cpp
  ${CATLIB}/string/implementations/memset.cpp
  ${CATLIB}/string/implementations/strlen.cpp
  ${CATLIB}/string/implementations/compare_strings.cpp
  ${CATLIB}/string/implementations/print.cpp
  ${CATLIB}/string/implementations/println.cpp
  ${CATLIB}/string/implementations/eprint.cpp
  ${CATLIB}/string/implementations/eprintln.cpp
  ${CATLIB}/format/implementations/itoa_jeaiii.cpp
  ${CATLIB}/format/implementations/ftoa_dragonbox.cpp
  ${CATLIB}/linux/implementations/syscall0.cpp
  ${CATLIB}/linux/implementations/syscall1.cpp
  ${CATLIB}/linux/implementations/syscall2.cpp
  ${CATLIB}/linux/implementations/syscall3.cpp
  ${CATLIB}/linux/implementations/syscall4.cpp
  ${CATLIB}/linux/implementations/syscall5.cpp
  ${CATLIB}/linux/implementations/syscall6.cpp
  ${CATLIB}/linux/implementations/sys_read.cpp
  ${CATLIB}/linux/implementations/sys_write.cpp
  ${CATLIB}/linux/implementations/sys_close.cpp
  ${CATLIB}/linux/implementations/clonearguments.cpp
  ${CATLIB}/linux/implementations/sys_open.cpp
  ${CATLIB}/linux/implementations/sys_rt_sigprocmask.cpp
  ${CATLIB}/linux/implementations/sys_unlink.cpp
  ${CATLIB}/linux/implementations/sys_getrlimit.cpp
  ${CATLIB}/linux/implementations/sys_setrlimit.cpp
  ${CATLIB}/linux/implementations/sys_mmap.cpp
  ${CATLIB}/linux/implementations/sys_munmap.cpp
  ${CATLIB}/linux/implementations/sys_wait4.cpp
  ${CATLIB}/linux/implementations/wait_pid.cpp
  ${CATLIB}/linux/implementations/sys_waitid.cpp
  ${CATLIB}/linux/implementations/sys_clone.cpp
  ${CATLIB}/linux/implementations/executable_tls.cpp
  ${CATLIB}/linux/implementations/sys_socket.cpp
  ${CATLIB}/linux/implementations/create_socket_local.cpp
  ${CATLIB}/linux/implementations/is_a_tty.cpp
  ${CATLIB}/linux/implementations/tty_get_attributes.cpp
  ${CATLIB}/linux/implementations/tty_set_attributes.cpp
  ${CATLIB}/linux/implementations/sys_ioctl.cpp
  ${CATLIB}/linux/implementations/sys_getpid.cpp
  ${CATLIB}/linux/implementations/read_char.cpp
  ${CATLIB}/linux/implementations/sys_accept.cpp
  ${CATLIB}/linux/implementations/sys_arch_prctl.cpp
  ${CATLIB}/linux/implementations/sys_bind.cpp
  ${CATLIB}/linux/implementations/sys_connect.cpp
  ${CATLIB}/linux/implementations/sys_listen.cpp
  ${CATLIB}/linux/implementations/sys_recv.cpp
  ${CATLIB}/linux/implementations/sys_sendto.cpp
  ${CATLIB}/linux/implementations/sys_writev.cpp
  ${CATLIB}/linux/implementations/sys_readv.cpp
  ${CATLIB}/linux/implementations/sys_stat.cpp
  ${CATLIB}/linux/implementations/sys_fstat.cpp
  ${CATLIB}/linux/implementations/sys_tkill.cpp
  ${CATLIB}/linux/implementations/sys_sched_yield.cpp
  ${CATLIB}/linux/implementations/sys_tgkill.cpp
  ${CATLIB}/linux/implementations/sys_futex.cpp
  ${CATLIB}/linux/implementations/sys_set_robust_list.cpp
  ${CATLIB}/linux/implementations/sys_get_robust_list.cpp
  ${CATLIB}/linux/implementations/sys_futex_waitv.cpp
  ${CATLIB}/linux/implementations/block_all_signals.cpp
  ${CATLIB}/linux/implementations/raise.cpp
  ${CATLIB}/linux/implementations/raise_here.cpp
  ${CATLIB}/linux/implementations/process.cpp
  ${CATLIB}/debug/implementations/breakpoint.cpp
  ${CATLIB}/debug/implementations/default_assert_handler.cpp
  ${CATLIB}/debug/implementations/verify.cpp
  ${CATLIB}/debug/implementations/assert.cpp
)

set(CAT_HEADER_FILES
  ${CATLIB}/algorithm/cat/algorithm
  ${CATLIB}/allocator/cat/allocator
  ${CATLIB}/allocator/implementations/meta_alloc.tpp
  ${CATLIB}/allocator/implementations/get.tpp
  ${CATLIB}/allocator/implementations/free.tpp
  ${CATLIB}/allocator/cat/linear_allocator
  ${CATLIB}/allocator/cat/null_allocator
  ${CATLIB}/allocator/cat/page_allocator
  ${CATLIB}/allocator/cat/pool_allocator
  ${CATLIB}/arithmetic/cat/arithmetic
  ${CATLIB}/arithmetic/cat/overflow_reference.hpp
  ${CATLIB}/arithmetic/cat/arithmetic_interface
  ${CATLIB}/array/cat/array
  ${CATLIB}/atomic/cat/atomic
  ${CATLIB}/bit/cat/bit
  ${CATLIB}/bitset/cat/bitset
  ${CATLIB}/collection/cat/collection
  ${CATLIB}/compare/cat/compare
  ${CATLIB}/debug/cat/debug
  ${CATLIB}/file/cat/file
  ${CATLIB}/format/cat/format
  ${CATLIB}/format/cat/detail/ftoa_dragonbox.hpp
  ${CATLIB}/format/cat/detail/itoa_jeaiii.hpp
  ${CATLIB}/functional/cat/functional
  ${CATLIB}/iterator/cat/iterator
  ${CATLIB}/iterator/cat/insert_iterators
  ${CATLIB}/limits/cat/limits
  ${CATLIB}/linux/cat/linux
  ${CATLIB}/list/cat/list
  ${CATLIB}/match/cat/match
  ${CATLIB}/math/cat/math
  ${CATLIB}/maybe/cat/maybe
  ${CATLIB}/maybe/implementations/narrow_cast.tpp
  ${CATLIB}/memory/cat/memory
  ${CATLIB}/memory/implementations/copy_memory.tpp
  ${CATLIB}/memory/implementations/copy_memory_backward.tpp
  ${CATLIB}/memory/implementations/is_memory_overlapping.tpp
  ${CATLIB}/meta/cat/meta
  ${CATLIB}/notype/cat/notype
  ${CATLIB}/ring/cat/ring
  ${CATLIB}/runtime/cat/runtime
  ${CATLIB}/sanitizer/cat/sanitizer
  ${CATLIB}/scaredy/cat/scaredy
  ${CATLIB}/simd/cat/simd
  ${CATLIB}/simd/cat/detail/simd_overflow_reference.hpp
  ${CATLIB}/simd/cat/simd_ops
  ${CATLIB}/simd/cat/simd_iterator
  ${CATLIB}/simd/implementations/simd_mask_bitset.tpp
  ${CATLIB}/simd/cat/detail/simd_broadcast_traits.hpp
  ${CATLIB}/simd/cat/detail/simd_range_iterator.hpp
  ${CATLIB}/simd/cat/detail/simd_avx2.hpp
  ${CATLIB}/simd/cat/detail/simd_avx2_mask_ops.hpp
  ${CATLIB}/simd/cat/detail/simd_width_aliases.hpp
  ${CATLIB}/simd/cat/detail/simd_unaligned_abi.hpp
  ${CATLIB}/simd/cat/detail/simd_mask_lane_sseavx.hpp
  ${CATLIB}/simd/cat/detail/simd_sse42.hpp
  ${CATLIB}/simd/cat/detail/simd_sse2.hpp
  ${CATLIB}/simd/cat/detail/simd_sse2_movmsk.hpp
  ${CATLIB}/simd/cat/detail/simd_sse2_unary_full.hpp
  ${CATLIB}/simd/cat/detail/simd_avx2_unary_full.hpp
  ${CATLIB}/socket/cat/socket
  ${CATLIB}/span/cat/span
  ${CATLIB}/string/cat/string
  ${CATLIB}/thread/cat/thread
  ${CATLIB}/tui/cat/tui
  ${CATLIB}/tuple/cat/tuple
  ${CATLIB}/type_list/cat/type_list
  ${CATLIB}/unique/cat/unique
  ${CATLIB}/utility/cat/utility
  ${CATLIB}/variant/cat/variant
  ${CATLIB}/vec/cat/vec
  ${CATLIB}/x11/cat/x11
  ${CATLIB}/linux/implementations/syscall.tpp
  ${CATLIB}/linux/implementations/process.tpp
  ${CATLIB}/linux/implementations/futex.tpp
  ${CATLIB}/meta/implementations/constant_evaluate.tpp
  ${CATLIB}/meta/implementations/common_reference.tpp
  ${CATLIB}/simd/implementations/stream_in.tpp
  ${CATLIB}/string/implementations/string_length.tpp
  ${CATLIB}/bit/implementations/is_aligned.tpp
  ${CATLIB}/bit/implementations/align_up.tpp
  ${CATLIB}/bit/implementations/align_down.tpp
  ${CATLIB}/tuple/implementations/tuple_cat.tpp
  ${CATLIB}/enum/cat/enum
  PARENT_SCOPE
)
