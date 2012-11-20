# Common
#

message(STATUS "Common =======================================================")

add_definitions('-fPIC')


##########################################
# atp_common api
set(atp_common_incs
  ${GEN_DIR}
  ${SRC_DIR}
)
set(atp_common_srcs
  time_utils.cpp
 )
set(atp_common_libs
 boost_system
 boost_thread
 glog
)
# Client implementation:
cpp_library(atp_common)

add_custom_target(all_common)
add_dependencies(all_common
  atp_common
)


