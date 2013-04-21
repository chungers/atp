# Varz
#

message(STATUS "Varz =========================================================")

add_definitions('-fPIC')

##########################################
# Varz library
set(atp_varz_incs
  ${GEN_DIR}
  ${SRC_DIR}
  ${THIRD_PARTY_DIR}
)
set(atp_varz_srcs
  varz.cpp
  VarzServer.cpp
  ${THIRD_PARTY_DIR}/mongoose/mongoose.c
)
set(atp_varz_libs
  dl
  boost_system
  boost_thread
  json_spirit
  gflags
  glog
)
cpp_library(atp_varz)

