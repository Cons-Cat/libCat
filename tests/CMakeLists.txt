add_library(libcat-test INTERFACE)
target_link_libraries(libcat-test INTERFACE libcat-std)
target_include_directories(
  libcat-test INTERFACE
  minunit/
)

# TODO: Get these working:
# add_compile_options(${LIBCAT_CXX_FLAGS})
link_libraries(libcat-test)
# add_link_options(${LIBCAT_LINK_FLAGS})

# This tests that a Result<void> does not panic if it holds ok.
add_executable(test_result_ok result_success.cpp)
target_compile_options(test_result_ok PRIVATE ${LIBCAT_CXX_FLAGS})
target_link_options(test_result_ok PRIVATE ${LIBCAT_LINK_FLAGS})
add_test(NAME TestResultOk COMMAND test_result_ok)

# This tests that a Result<void> will panic if it holds a Failure.
add_executable(test_result_fail result_failure.cpp)
target_compile_options(test_result_fail PRIVATE ${LIBCAT_CXX_FLAGS})
target_link_options(test_result_fail PRIVATE ${LIBCAT_LINK_FLAGS})
add_test(NAME TestResultFail COMMAND test_result_fail)
set_tests_properties(TestResultFail PROPERTIES WILL_FAIL TRUE)
