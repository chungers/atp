#
# Tests for platform code

message(STATUS "TESTS: Platform ==============================================")

# MessageProcessorTests
set(MessageProcessorTest_incs
  ${GEN_DIR}
  ${SRC_DIR}
  ${TEST_DIR}
)
set(MessageProcessorTest_srcs
  ${TEST_DIR}/AllTests.cpp
  MessageProcessorTest.cpp
)
set(MessageProcessorTest_libs
  proto_components
  message_processor
  gflags
  glog
  zmq
  )
cpp_gtest(MessageProcessorTest)


add_custom_target(all_platform_tests)
add_dependencies(all_platform_tests
  MessageProcessorTest
)
