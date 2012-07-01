
#
# Tests for top level services and managers

message(STATUS "TESTS: Core Services =========================================")

# OrderManager
set(OrderManagerTest_incs
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
