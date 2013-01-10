# Platform
#

message(STATUS "Platform =====================================================")

add_definitions('-fPIC')


##########################################
# atp_platform_base api
set(atp_platform_base_incs
  ${GEN_DIR}
  ${SRC_DIR}
)
set(atp_platform_base_srcs
  contract_symbol.cpp
  version_info.cpp
 )
set(atp_platform_base_libs
 atp_proto
 zmq
)
cpp_library(atp_platform_base)


##########################################
# atp_platform_message_processor api
set(atp_platform_message_processor_incs
  ${GEN_DIR}
  ${SRC_DIR}
)
set(atp_platform_message_processor_srcs
  message_processor.cpp
 )
set(atp_platform_message_processor_libs
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
cpp_library(atp_platform_message_processor)

FILE(GLOB PLATFORM_ALL_FILES "*.cpp")
##########################################
# atp_platform_all api
set(atp_platform_all_incs
  ${GEN_DIR}
  ${SRC_DIR}
)
set(atp_platform_all_srcs
  ${PLATFORM_ALL_FILES}
 )
set(atp_platform_all_libs
  atp_platform_base
  atp_platform_message_processor
)
cpp_library(atp_platform_all)


add_custom_target(all_platform)
add_dependencies(all_platform
  atp_platform_base
  atp_platform_message_processor
  atp_platform_all
)


