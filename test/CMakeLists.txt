
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
)
