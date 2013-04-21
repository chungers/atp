#
# Tests for CouchDB


message(STATUS "TESTS: CouchDB ===============================================")

# Complete Test Suite
set(test_couchdb_all_incs
  ${IBAPI_ROOT}
  ${GEN_DIR}
  ${SRC_DIR}
  ${TEST_DIR}
  ${API_IMPL}
)
set(test_couchdb_all_srcs
  ${TEST_DIR}/AllTests.cpp
  CouchDbTest.cpp
)
set(test_couchdb_all_libs
  atp_couchdb
)
cpp_gtest(test_couchdb_all)

add_custom_target(all_couchdb_tests)
add_dependencies(all_couchdb_tests
  test_couchdb_all
)
