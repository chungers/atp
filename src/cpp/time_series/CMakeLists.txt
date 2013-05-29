# Time Series
#

message(STATUS "Time Series ==================================================")

add_definitions('-fPIC')


##########################################
# atp_time_series api
set(atp_time_series_incs
  ${GEN_DIR}
  ${SRC_DIR}
)
set(atp_time_series_srcs
#  time_utils.cpp
 )
set(atp_time_series_libs
 boost_system
 boost_thread
 glog
)
# Client implementation:
#cpp_library(atp_time_series)

add_custom_target(all_time_series)
#add_dependencies(all_time_series
#  atp_time_series
#)
