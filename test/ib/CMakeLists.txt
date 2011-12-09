#
# Tests for IB connector

message(STATUS "TESTS: IB Connetor ===========================================")
message(STATUS "-- IB API version ${IBAPI_VERSION} from ${IBAPI_ROOT}")
message(STATUS "-- Implementation from ${IBAPI_IMPL_DIR}")

option(IB_USE_STD_STRING "Use standard c++ string (see Shared/IBString.h)." ON)
if (IB_USE_STD_STRING)
  add_definitions('-DIB_USE_STD_STRING')
endif (IB_USE_STD_STRING)

# AsioEClientSocket test
set(AsioEClientSocket_test_incs
  ${IBAPI_ROOT}
  ${GEN_DIR}
  ${SRC_DIR}
  ${SRC_DIR}/ib/${IBAPI_IMPL_DIR}
  ${IBAPI_IMPL_DIR}
  ${TEST_DIR}
)
set(AsioEClientSocket_test_srcs
  AllTests.cpp
  AsioEClientSocketTest.cpp
  ${IBAPI_IMPL_DIR}/TestEWrapperFactoryImpl.cpp
)
set(AsioEClientSocket_test_libs
  IBAPIConnector
  QuantLib
)
cpp_gtest(AsioEClientSocket_test)

# SocketInitiator test
set(SocketInitiator_test_incs
  ${IBAPI_ROOT}
  ${GEN_DIR}
  ${SRC_DIR}
  ${IBAPI_IMPL_DIR}
  ${TEST_DIR}
)
set(SocketInitiator_test_srcs
  AllTests.cpp
  SocketInitiatorTest.cpp
)
set(SocketInitiator_test_libs
  IBAPIConnector
)
cpp_gtest(SocketInitiator_test)

# SocketConnector test
set(SocketConnector_test_incs
  ${IBAPI_ROOT}
  ${GEN_DIR}
  ${SRC_DIR}
  ${SRC_DIR}/ib/${IBAPI_IMPL_DIR}
  ${IBAPI_IMPL_DIR}
  ${TEST_DIR}
)
set(SocketConnector_test_srcs
  AllTests.cpp
  SocketConnectorTest.cpp
  ${IBAPI_IMPL_DIR}/TestEWrapperFactoryImpl.cpp
)
set(SocketConnector_test_libs
  IBAPIConnector
)
cpp_gtest(SocketConnector_test)

# MarketData test
set(MarketData_test_incs
  ${IBAPI_ROOT}
  ${GEN_DIR}
  ${SRC_DIR}
  ${SRC_DIR}/ib/${IBAPI_IMPL_DIR}
  ${IBAPI_IMPL_DIR}
  ${TEST_DIR}
)
set(MarketData_test_srcs
  AllTests.cpp
  MarketDataTest.cpp
  ${IBAPI_IMPL_DIR}/VersionSpecific.cpp
)
set(MarketData_test_libs
  IBAPIConnector
)
cpp_gtest(MarketData_test)

# Message test
set(Message_test_incs
  ${GEN_DIR}
  ${SRC_DIR}
  ${TEST_DIR}
)
set(Message_test_srcs
  AllTests.cpp
  MessageTest.cpp
  ApiMessageBaseTest.cpp
)
set(Message_test_libs
  IBAPIConnector
  zmq
)
cpp_gtest(Message_test)

# Utils test
set(utils_test_incs
  ${GEN_DIR}
  ${SRC_DIR}
  ${TEST_DIR}
)
set(utils_test_srcs
  AllTests.cpp
  UtilsTest.cpp
)
set(utils_test_libs
  IBAPIConnector
)
cpp_gtest(utils_test)


# Version specific unit tests.
add_subdirectory(api964)
