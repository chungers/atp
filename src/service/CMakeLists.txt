# Services
#

message(STATUS "Services =====================================================")

add_definitions('-fPIC')

##########################################
# atp_service_order_manager api
set(atp_service_order_manager_incs
  ${GEN_DIR}
  ${SRC_DIR}
)
set(atp_service_order_manager_srcs
  OrderManager.cpp
)
set(atp_service_order_manager_libs
 atp_platform_all
 gflags
 glog
)
# Client implementation:
cpp_library(atp_service_order_manager)


##########################################
# atp_service_contract_manager api
set(atp_service_contract_manager_incs
  ${GEN_DIR}
  ${SRC_DIR}
)
set(atp_service_contract_manager_srcs
  ContractManager.cpp
)
set(atp_service_contract_manager_libs
 atp_platform_all
 gflags
 glog
)
# Client implementation:
cpp_library(atp_service_contract_manager)


##########################################
# atp_service_log_reader api
set(atp_service_log_reader_incs
  ${GEN_DIR}
  ${SRC_DIR}
)
set(atp_service_log_reader_srcs
  LogReader.cpp
  LogReaderZmq.cpp
)
set(atp_service_log_reader_libs
 api_base
 atp_common
 atp_historian
 atp_platform_base
 atp_proto
 atp_zmq
 boost_iostreams
 boost_system
 boost_thread
 gflags
 glog
 zmq
 bz2
 z
)
# Client implementation:
cpp_library(atp_service_log_reader)

FILE(GLOB SERVICE_ALL_FILES "*.cpp")
set(atp_service_all_incs
  ${GEN_DIR}
  ${SRC_DIR}
)
set(atp_service_all_srcs
  ${SERVICE_ALL_FILES}
)
set(atp_service_all_libs
  atp_service_contract_manager
  atp_service_order_manager
  atp_service_log_reader
)
cpp_library(atp_service_all)


add_custom_target(all_services)
add_dependencies(all_services
  atp_service_contract_manager
  atp_service_order_manager
  atp_service_log_reader
  atp_service_all
)


