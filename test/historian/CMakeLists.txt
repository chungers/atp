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
  historian_components
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
  historian_components
)
cpp_gtest(historian_tests)

