# Services
#

message(STATUS "Services =====================================================")

add_definitions('-fPIC')

##########################################
# atp_order_manager api
set(atp_order_manager_incs
  ${GEN_DIR}
  ${SRC_DIR}
)
set(atp_order_manager_srcs
  OrderManager.cpp
)
set(atp_order_manager_libs
 atp_platform_base
 atp_proto
 atp_zmq
 boost_system
 boost_thread
 gflags
 glog
 zmq
)
# Client implementation:
cpp_library(atp_order_manager)


##########################################
# atp_contract_manager api
set(atp_contract_manager_incs
  ${GEN_DIR}
  ${SRC_DIR}
)
set(atp_contract_manager_srcs
  ContractManager.cpp
)
set(atp_contract_manager_libs
 atp_platform_base
 atp_proto
 atp_zmq
 boost_system
 boost_thread
 gflags
 glog
 zmq
)
# Client implementation:
cpp_library(atp_contract_manager)


##########################################
# atp_log_reader api
set(atp_log_reader_incs
  ${GEN_DIR}
  ${SRC_DIR}
)
set(atp_log_reader_srcs
  LogReader.cpp
  LogReaderZmq.cpp
)
set(atp_log_reader_libs
 api_base
 atp_common
 atp_historian
 atp_platform_base
 atp_proto
 atp_zmq
 boost_system
 boost_thread
 gflags
 glog
 zmq
)
# Client implementation:
cpp_library(atp_log_reader)

add_custom_target(all_services)
add_dependencies(all_services
  atp_contract_manager
  atp_order_manager
  atp_log_reader
)


