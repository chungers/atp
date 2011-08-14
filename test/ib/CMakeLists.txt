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
  ${IBAPI_IMPL_DIR}/EWrapperFactoryImpl.cpp
)
set(AsioEClientSocket_test_libs
  boost_system
  boost_thread
  ib_adapter
  gflags
  glog
)
cpp_gtest(AsioEClientSocket_test)


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
  ib_adapter
  gflags
  glog
)
cpp_gtest(utils_test)
