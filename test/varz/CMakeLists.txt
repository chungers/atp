#
# Tests for Varz


message(STATUS "TESTS: Varz ================================================")

# Complete Test Suite
set(varz_test_incs
  ${GEN_DIR}
  ${SRC_DIR}
  ${TEST_DIR}
)
set(varz_test_srcs
  ${TEST_DIR}/AllTests.cpp
  VarzTest.cpp
)
set(varz_test_libs
  varz_components
)
cpp_gtest(varz_test)

