#
# Tests for historian

message(STATUS "TESTS: historian  ============================================")

# Internal Stuff
set(test_historian_internal_incs
  ${GEN_DIR}
  ${SRC_DIR}
  ${TEST_DIR}
)
set(test_historian_internal_srcs
  ${TEST_DIR}/AllTests.cpp
  InternalTest.cpp
  UtilsTest.cpp
)
set(test_historian_internal_libs
  atp_historian
)
cpp_gtest(test_historian_internal)

# Complete Test Suite
set(test_historian_incs
  ${GEN_DIR}
  ${SRC_DIR}
  ${TEST_DIR}
)
set(test_historian_srcs
  ${TEST_DIR}/AllTests.cpp
  DbTest.cpp
  DbReactorTest.cpp
)
set(test_historian_libs
  atp_historian
)
cpp_gtest(test_historian)

add_custom_target(all_historian_tests)
add_dependencies(all_historian_tests
  test_historian_internal
  test_historian
)