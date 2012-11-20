# Services
#

message(STATUS "Platform =====================================================")

add_definitions('-fPIC')


##########################################
# atp_message_processor api
set(atp_message_processor_incs
  ${GEN_DIR}
  ${SRC_DIR}
)
set(atp_message_processor_srcs
  message_processor.cpp
 )
set(atp_message_processor_libs
 api_base
 atp_historian
 atp_proto
 atp_zmq
 boost_system
 boost_thread
 gflags
 glog
 zmq
)
# Client implementation:
cpp_library(atp_message_processor)

add_custom_target(all_platform)
add_dependencies(all_platform
  atp_message_processor
)


