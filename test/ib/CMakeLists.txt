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
set(test_ib_asio_eclient_driver_incs
  ${IBAPI_ROOT}
  ${GEN_DIR}
  ${SRC_DIR}
  ${SRC_DIR}/ib/${IBAPI_IMPL_DIR}
  ${IBAPI_IMPL_DIR}
  ${TEST_DIR}
)
set(test_ib_asio_eclient_driver_srcs
  ${TEST_DIR}/AllTests.cpp
  AsioEClientDriverTest.cpp
)
set(test_ib_asio_eclient_driver_libs
  IBAPIConnector
  ib_api_versioned
  QuantLib
)
cpp_gtest(test_ib_asio_eclient_driver)

# SocketInitiator test
set(test_ib_socket_initiator_incs
  ${IBAPI_ROOT}
  ${GEN_DIR}
  ${SRC_DIR}
  ${IBAPI_IMPL_DIR}
  ${TEST_DIR}
)
set(test_ib_socket_initiator_srcs
  ${TEST_DIR}/AllTests.cpp
  SocketInitiatorTest.cpp
)
set(test_ib_socket_initiator_libs
  IBAPIConnector
  ib_api_versioned
)
cpp_gtest(test_ib_socket_initiator)

# SocketConnector test
set(test_ib_socket_connector_incs
  ${IBAPI_ROOT}
  ${GEN_DIR}
  ${SRC_DIR}
  ${SRC_DIR}/ib/${IBAPI_IMPL_DIR}
  ${IBAPI_IMPL_DIR}
  ${TEST_DIR}
)
set(test_ib_socket_connector_srcs
  ${TEST_DIR}/AllTests.cpp
  SocketConnectorTest.cpp
)
set(test_ib_socket_connector_libs
  IBAPIConnector
  ib_api_versioned
)
cpp_gtest(test_ib_socket_connector)

# Utils test
set(test_ib_utils_incs
  ${GEN_DIR}
  ${SRC_DIR}
  ${TEST_DIR}
)
set(test_ib_utils_srcs
  ${TEST_DIR}/AllTests.cpp
  TickerMapTest.cpp
  UtilsTest.cpp
)
set(test_ib_utils_libs
  IBAPIConnector
  ib_api_versioned
)
cpp_gtest(test_ib_utils)


# Version specific unit tests.
add_subdirectory(${IBAPI_IMPL_DIR})

add_custom_target(all_ib_tests)
add_dependencies(all_ib_tests
  test_ib_asio_eclient_driver
  test_ib_socket_initiator
  test_ib_socket_connector
  test_ib_utils
  all_versioned_tests # target in subdirectory
)