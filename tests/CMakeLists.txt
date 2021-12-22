add_library(libcat-test INTERFACE)
target_link_libraries(libcat-test INTERFACE libcat-std)
target_include_directories(
  libcat-test INTERFACE
)

list(
  APPEND
  LIBCAT_CXX_FLAGS_TEST
  ${LIBCAT_CXX_FLAGS_COMMON}
  $<$<CONFIG:Debug>:${LIBCAT_CXX_FLAGS_DEBUG}>
  $<$<CONFIG:RelWithDebInfo>:${LIBCAT_CXX_FLAGS_RELWITHDEBINFO}>
  $<$<CONFIG:Release>:${LIBCAT_CXX_FLAGS_RELFAST}>
  $<$<CONFIG:RelSmall>:${LIBCAT_CXX_FLAGS_RELSMALL}>
  # -Wno-unused-variable
)

# TODO: Get these working:
# add_compile_options(${LIBCAT_CXX_FLAGS})
link_libraries(libcat-test)
# add_link_options(${LIBCAT_LINK_FLAGS})

# This tests that a Result<void> does not panic if it holds ok.
add_executable(test_result_ok result_success.cpp)
target_compile_options(test_result_ok PRIVATE ${LIBCAT_CXX_FLAGS_TEST})
target_link_options(test_result_ok PRIVATE ${LIBCAT_LINK_FLAGS})
add_test(NAME TestResultOk COMMAND test_result_ok)

# This tests that a Result<void> will panic if it holds a Failure.
add_executable(test_result_fail result_failure.cpp)
target_compile_options(test_result_fail PRIVATE ${LIBCAT_CXX_FLAGS_TEST})
target_link_options(test_result_fail PRIVATE ${LIBCAT_LINK_FLAGS})
add_test(NAME TestResultFail COMMAND test_result_fail)
set_tests_properties(TestResultFail PROPERTIES WILL_FAIL TRUE)

# This tests if <buffer.h> works.
add_executable(test_buffers buffers.cpp)
target_compile_options(test_buffers PRIVATE ${LIBCAT_CXX_FLAGS_TEST})
target_link_options(test_buffers PRIVATE ${LIBCAT_LINK_FLAGS})
add_test(NAME TestBuffers COMMAND test_buffers)

# This tests if string_length_as<>() works.
add_executable(test_strlen string_length.cpp)
target_compile_options(test_strlen PRIVATE ${LIBCAT_CXX_FLAGS_TEST})
target_link_options(test_strlen PRIVATE ${LIBCAT_LINK_FLAGS})
add_test(NAME TestStrlen COMMAND test_strlen)

# This tests if copy_memory() works.
add_executable(test_copymem copy_memory.cpp)
target_compile_options(test_copymem PRIVATE ${LIBCAT_CXX_FLAGS_TEST})
target_link_options(test_copymem PRIVATE ${LIBCAT_LINK_FLAGS})
add_test(NAME CopyMem COMMAND test_copymem)
