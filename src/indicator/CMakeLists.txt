# Indicator
#

message(STATUS "Indicator ====================================================")

add_definitions('-fPIC')

FILE(GLOB INDICATOR_ALL_FILES "*.cpp")

##########################################
# atp_indicator api
set(atp_indicator_incs
  ${GEN_DIR}
  ${SRC_DIR}
)
set(atp_indicator_srcs
  ${INDICATOR_ALL_FILES}
 )
set(atp_indicator_libs
 atp_proto
 ta_lib
)
cpp_library(atp_indicator)

add_custom_target(all_indicator)
add_dependencies(all_indicator
  atp_indicator
)
