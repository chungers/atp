#
# Tests for IB VERSION 9.64 connector

message(STATUS "TESTS: IB V964 Connetor ======================================")
message(STATUS "-- IB API version ${IBAPI_VERSION} from ${IBAPI_ROOT}")

option(IB_USE_STD_STRING "Use standard c++ string (see Shared/IBString.h)." ON)
if (IB_USE_STD_STRING)
  add_definitions('-DIB_USE_STD_STRING')
endif (IB_USE_STD_STRING)

include_directories(${IBAPI_ROOT}/Shared ${IBAPI_ROOT}/PosixSocketClient)


# V964Message test
set(V964Message_test_incs
  ${GEN_DIR}
  ${SRC_DIR}
  ${TEST_DIR}
)
set(V964Message_test_srcs
  ../AllTests.cpp
  V964MessageTest.cpp
  TestEWrapperFactoryImpl.cpp
)
set(V964Message_test_libs
  IBAPIConnector
  QuantLib
  zmq
)
cpp_gtest(V964Message_test)
