#
# Tests for historian

message(STATUS "TESTS: historian  ============================================")

# Internal Stuff
set(historian_internal_tests_incs
  ${GEN_DIR}
  ${SRC_DIR}
  ${TEST_DIR}
)
set(historian_internal_tests_srcs
  ${TEST_DIR}/AllTests.cpp
  InternalTest.cpp
  UtilsTest.cpp
)
set(historian_internal_tests_libs
  atp_historian
)
cpp_gtest(historian_internal_tests)

# Complete Test Suite
set(historian_tests_incs
  ${GEN_DIR}
  ${SRC_DIR}
  ${TEST_DIR}
)
set(historian_tests_srcs
  ${TEST_DIR}/AllTests.cpp
  DbTest.cpp
  DbReactorTest.cpp
)
set(historian_tests_libs
  atp_historian
)
cpp_gtest(historian_tests)

add_custom_target(all_historian_tests)
add_dependencies(all_historian_tests
  historian_internal_tests
  historian_tests
)