#
# Tests for Varz


message(STATUS "TESTS: Varz ================================================")

add_definitions('-DJSON_SPIRIT_MVALUE_ENABLED')

# Complete Test Suite
set(varz_test_incs
  ${GEN_DIR}
  ${SRC_DIR}
  ${TEST_DIR}
)
set(varz_test_srcs
  ${TEST_DIR}/AllTests.cpp
  VarzTest.cpp
)
set(varz_test_libs
  json_spirit
  varz_components
)
cpp_gtest(varz_test)

add_custom_target(all_varz_tests)
add_dependencies(all_varz_tests
  varz_test
)


