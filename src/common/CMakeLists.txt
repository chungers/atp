# Common
#

message(STATUS "Common =======================================================")

add_definitions('-fPIC')


##########################################
# common_components api
set(common_components_incs
  ${GEN_DIR}
  ${SRC_DIR}
)
set(common_components_srcs
  time_utils.cpp
 )
set(common_components_libs
 boost_system
 boost_thread
 glog
)
# Client implementation:
cpp_library(common_components)

add_custom_target(all_common)
add_dependencies(all_common
  common_components
)


