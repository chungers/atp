#
# Tests for IB connector

message(STATUS "Tests: IB API version ${IBAPI_VERSION} from ${IBAPI_ROOT}")
include_directories(${IBAPI_ROOT}/Shared ${IBAPI_ROOT}/PosixSocketClient)

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
  #SocketInitiatorTest.cpp
  ${IBAPI_IMPL_DIR}/TestEWrapperFactoryImpl.cpp
)
set(SocketInitiator_test_libs
  IBAPIConnector
)
cpp_gtest(SocketInitiator_test)

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
