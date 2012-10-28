# Services
#

message(STATUS "Platform =====================================================")

add_definitions('-fPIC')


##########################################
# message_processor api
set(message_processor_incs
  ${GEN_DIR}
  ${SRC_DIR}
)
set(message_processor_srcs
  LogReader.cpp
)
set(message_processor_libs
 api_base
 historian_components
 proto_components
 zmq_components
 boost_system
 boost_thread
 gflags
 glog
 zmq
)
# Client implementation:
cpp_library(message_processor)

add_custom_target(all_platform)
add_dependencies(all_platform
  message_processor
)


