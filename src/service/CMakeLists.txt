# Services
#

message(STATUS "Services =====================================================")

add_definitions('-fPIC')

##########################################
# order_manager api
set(order_manager_incs
  ${GEN_DIR}
  ${SRC_DIR}
)
set(order_manager_srcs
  OrderManager.cpp
)
set(order_manager_libs
 proto_components
 zmq_components
 boost_system
 boost_thread
 gflags
 glog
 zmq
)
# Client implementation:
cpp_library(order_manager)


##########################################
# log_reader api
set(log_reader_incs
  ${GEN_DIR}
  ${SRC_DIR}
)
set(log_reader_srcs
  LogReader.cpp
)
set(log_reader_libs
 api_base
 proto_components
 zmq_components
 boost_system
 boost_thread
 gflags
 glog
 zmq
)
# Client implementation:
cpp_library(log_reader)

add_custom_target(all_services)
add_dependencies(all_services
  order_manager
  log_reader
)


