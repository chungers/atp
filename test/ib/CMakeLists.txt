#
# Tests for IB connector

message(STATUS "Tests: IB API version ${IBAPI_VERSION} from ${IBAPI_ROOT}")

option(IB_USE_STD_STRING "Use standard c++ string (see Shared/IBString.h)." ON)
if (IB_USE_STD_STRING)
  add_definitions('-DIB_USE_STD_STRING')
endif (IB_USE_STD_STRING)

include_directories(${IBAPI_ROOT}/Shared ${IBAPI_ROOT}/PosixSocketClient)

# Complete Test Suite
set(IBTestSuite_incs
  ${IBAPI_ROOT}
  ${GEN_DIR}
  ${SRC_DIR}
  ${IBAPI_IMPL_DIR}
  ${TEST_DIR}
)
set(IBTestSuite_srcs
  AllTests.cpp
  AsioEClientSocketTest.cpp
  SocketInitiatorTest.cpp
  UtilsTest.cpp
  ${IBAPI_IMPL_DIR}/TestEWrapperFactoryImpl.cpp
)
set(IBTestSuite_libs
  IBAPIConnector
)
cpp_gtest(IBTestSuite)

# AsioEClientSocket test
set(AsioEClientSocket_test_incs
  ${IBAPI_ROOT}
  ${GEN_DIR}
  ${SRC_DIR}
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

# Message test
set(Message_test_incs
  ${GEN_DIR}
  ${SRC_DIR}
  ${TEST_DIR}
)
set(Message_test_srcs
  AllTests.cpp
  MessageTest.cpp
)
set(Message_test_libs
  IBAPIConnector
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
