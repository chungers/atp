#
# Tests for IB VERSION 9.66 connector

message(STATUS "TESTS: IB V966 Connetor ======================================")
message(STATUS "-- IB API version ${IBAPI_VERSION} from ${IBAPI_ROOT}")

option(IB_USE_STD_STRING "Use standard c++ string (see Shared/IBString.h)." ON)
if (IB_USE_STD_STRING)
  add_definitions('-DIB_USE_STD_STRING')
endif (IB_USE_STD_STRING)

include_directories(${IBAPI_ROOT}/Shared ${IBAPI_ROOT}/PosixSocketClient)


FILE(GLOB IMPL_FILES "${SRC_DIR}/ib/${IBAPI_IMPL_DIR}/*.cpp")

# MarketData test
set(V966_Marshall_test_incs
  ${IBAPI_ROOT}
  ${GEN_DIR}
  ${SRC_DIR}
  ${SRC_DIR}/ib/${IBAPI_IMPL_DIR}
  ${IBAPI_IMPL_DIR}
  ${TEST_DIR}
)
set(V966_Marshall_test_srcs
  ${TEST_DIR}/AllTests.cpp
  MarshallTest.cpp
)
set(V966_Marshall_test_libs
  ib_api_versioned
)
cpp_gtest(V966_Marshall_test)


add_custom_target(all_versioned_tests)
add_dependencies(all_versioned_tests
  V966_Marshall_test
)