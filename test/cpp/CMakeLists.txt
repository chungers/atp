

################# SUBDIRECTORIES ###################

add_subdirectory(common)
add_subdirectory(ib)
add_subdirectory(varz)
add_subdirectory(zmq)
add_subdirectory(couchdb)
add_subdirectory(service)
add_subdirectory(historian)
add_subdirectory(platform)
add_subdirectory(time_series)

add_custom_target(all_tests)
add_dependencies(all_tests
  all_common_tests
  all_zmq_tests
  all_couchdb_tests
  all_varz_tests
  all_ib_tests
  all_service_tests
  all_historian_tests
  all_platform_tests
  all_time_series_tests
)
