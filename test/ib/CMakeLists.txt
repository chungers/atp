#
# Tests for IB connector

message(STATUS "TESTS: IB Connetor ===========================================")
message(STATUS "-- IB API version ${IBAPI_VERSION} from ${IBAPI_ROOT}")
message(STATUS "-- Implementation from ${IBAPI_IMPL_DIR}")

option(IB_USE_STD_STRING "Use standard c++ string (see Shared/IBString.h)." ON)
if (IB_USE_STD_STRING)
  add_definitions('-DIB_USE_STD_STRING')
endif (IB_USE_STD_STRING)

# AsioEClientDriver test
set(AsioEClientDriver_test_incs
  ${IBAPI_ROOT}
  ${GEN_DIR}
  ${SRC_DIR}
  ${SRC_DIR}/ib/${IBAPI_IMPL_DIR}
  ${IBAPI_IMPL_DIR}
  ${TEST_DIR}
)
set(AsioEClientDriver_test_srcs
  ${TEST_DIR}/AllTests.cpp
  AsioEClientDriverTest.cpp
)
set(AsioEClientDriver_test_libs
  IBAPIConnector
  QuantLib
)
cpp_gtest(AsioEClientDriver_test)

# SocketInitiator test
set(SocketInitiator_test_incs
  ${IBAPI_ROOT}
  ${GEN_DIR}
  ${SRC_DIR}
  ${IBAPI_IMPL_DIR}
  ${TEST_DIR}
)
set(SocketInitiator_test_srcs
  ${TEST_DIR}/AllTests.cpp
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
  ${TEST_DIR}/AllTests.cpp
  SocketConnectorTest.cpp
)
set(SocketConnector_test_libs
  IBAPIConnector
)
cpp_gtest(SocketConnector_test)

# Utils test
set(utils_test_incs
  ${GEN_DIR}
  ${SRC_DIR}
  ${TEST_DIR}
)
set(utils_test_srcs
  ${TEST_DIR}/AllTests.cpp
  TickerMapTest.cpp
  UtilsTest.cpp
)
set(utils_test_libs
  IBAPIConnector
)
cpp_gtest(utils_test)


# Version specific unit tests.
add_subdirectory(${IBAPI_IMPL_DIR})

add_custom_target(all_ib_tests)
add_dependencies(all_ib_tests
  AsioEClientDriver_test
  SocketInitiator_test
  SocketConnector_test
  utils_test
  all_versioned_tests
)