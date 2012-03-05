#
# Tests for historian

message(STATUS "TESTS: historian  ============================================")

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
  UtilsTest.cpp
)
set(historian_tests_libs
  historian_components
)
cpp_gtest(historian_tests)

