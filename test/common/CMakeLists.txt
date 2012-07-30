#
# Tests for common code

message(STATUS "TESTS: Common ================================================")

# CommonTests
set(CommonLibTest_incs
  ${GEN_DIR}
  ${SRC_DIR}
  ${TEST_DIR}
)
set(CommonLibTest_srcs
  ${TEST_DIR}/AllTests.cpp
  FactoryTest.cpp
)
set(CommonLibTest_libs
  zmq
  )
cpp_gtest(CommonLibTest)


add_custom_target(all_common_tests)
add_dependencies(all_common_tests
  CommonLibTest
)
