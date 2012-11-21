#
# Tests for Varz


message(STATUS "TESTS: Varz ================================================")

add_definitions('-DJSON_SPIRIT_MVALUE_ENABLED')

# Complete Test Suite
set(test_varz_all_incs
  ${GEN_DIR}
  ${SRC_DIR}
  ${TEST_DIR}
)
set(test_varz_all_srcs
  ${TEST_DIR}/AllTests.cpp
  VarzTest.cpp
)
set(test_varz_all_libs
  json_spirit
  atp_varz
)
cpp_gtest(test_varz_all)

add_custom_target(all_varz_tests)
add_dependencies(all_varz_tests
  test_varz_all
)


