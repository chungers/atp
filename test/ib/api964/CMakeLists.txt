#
# Tests for IB VERSION 9.64 connector

message(STATUS "TESTS: IB V964 Connetor ======================================")
message(STATUS "-- IB API version ${IBAPI_VERSION} from ${IBAPI_ROOT}")

option(IB_USE_STD_STRING "Use standard c++ string (see Shared/IBString.h)." ON)
if (IB_USE_STD_STRING)
  add_definitions('-DIB_USE_STD_STRING')
endif (IB_USE_STD_STRING)

include_directories(${IBAPI_ROOT}/Shared ${IBAPI_ROOT}/PosixSocketClient)


# MarketData test
set(V964_MarketData_test_incs
  ${IBAPI_ROOT}
  ${GEN_DIR}
  ${SRC_DIR}
  ${SRC_DIR}/ib/${IBAPI_IMPL_DIR}
  ${IBAPI_IMPL_DIR}
  ${TEST_DIR}
)
set(V964_MarketData_test_srcs
  ${TEST_DIR}/AllTests.cpp
  MarketDataTest.cpp
  VersionSpecific.cpp
)
set(V964_MarketData_test_libs
  IBAPIConnector
)
cpp_gtest(V964_MarketData_test)


# V964Message test
set(V964_Message_test_incs
  ${GEN_DIR}
  ${SRC_DIR}
  ${TEST_DIR}
)
set(V964_Message_test_srcs
  ${TEST_DIR}/AllTests.cpp
  V964_MessageTest.cpp
  TestEWrapperFactoryImpl.cpp
)
set(V964_Message_test_libs
  IBAPIConnector
  QuantLib
  zmq
)
cpp_gtest(V964_Message_test)
