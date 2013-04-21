#
# Tests for Zmq

set(API_IMPL ${SRC_DIR}/ib/${IBAPI_IMPL_DIR})

message(STATUS "TESTS: ZeroMQ ================================================")

# Complete Test Suite
set(test_zmq_all_incs
  ${IBAPI_ROOT}
  ${GEN_DIR}
  ${SRC_DIR}
  ${TEST_DIR}
  ${API_IMPL}
)
set(test_zmq_all_srcs
  ${TEST_DIR}/AllTests.cpp
  ReactorTest.cpp
  PubSubTest.cpp
)
set(test_zmq_all_libs
  IBAPIConnector
  atp_zmq
)
cpp_gtest(test_zmq_all)

add_custom_target(all_zmq_tests)
add_dependencies(all_zmq_tests
  test_zmq_all
)
