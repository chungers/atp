
#
# Tests for top level services and managers

message(STATUS "TESTS: Core Services =========================================")
message(STATUS "-- IBAPI_IMPL_DIR = ${SRC_DIR}/ib/${IBAPI_IMPL_DIR}")

option(IB_USE_STD_STRING "Use standard c++ string (see Shared/IBString.h)." ON)
if (IB_USE_STD_STRING)
  add_definitions('-DIB_USE_STD_STRING')
endif (IB_USE_STD_STRING)

##########################################
# customize and use mocks instead.
set(api_protocol_handler_incs
  ${IBAPI_ROOT}
  ${IBAPI_ROOT}/Shared
  ${SRC_DIR}/ib/{IBAPI_IMPL_DIR}
)
set(api_protocol_handler_srcs
  ApiProtocolHandler.cpp
)
set(api_protocol_handler_libs
  ib_dist
  gflags
  glog
)
cpp_library(api_protocol_handler)



# OrderManager
set(OrderManagerTest_incs
  ${IBAPI_ROOT}
  ${IBAPI_ROOT}/Shared
  ${GEN_DIR}
  ${SRC_DIR}
  ${TEST_DIR}
)
set(OrderManagerTest_srcs
  ${TEST_DIR}/AllTests.cpp
  OrderManagerTest.cpp
)
set(OrderManagerTest_libs
  order_manager
  IBAPIConnector
  ib_api_versioned_base
#  api_protocol_handler
)
cpp_gtest(OrderManagerTest)


add_custom_target(all_core_tests)
add_dependencies(all_core_tests
  OrderManagerTest
)


################# SUBDIRECTORIES ###################

add_subdirectory(ib)
add_subdirectory(varz)
add_subdirectory(zmq)
add_subdirectory(historian)

add_custom_target(all_tests)
add_dependencies(all_tests
  all_zmq_tests
  all_varz_tests
  all_historian_tests
  all_ib_tests
  all_core_tests
)
