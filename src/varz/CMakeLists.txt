# Varz
#

message(STATUS "Varz =======================================================")

add_definitions('-fPIC')

##########################################
# Varz library
set(varz_components_incs
  ${GEN_DIR}
  ${SRC_DIR}
  ${THIRD_PARTY_DIR}
)
set(varz_components_srcs
  varz.cpp
  VarzServer.cpp
  ${THIRD_PARTY_DIR}/mongoose/mongoose.c
)
set(varz_components_libs
  dl
  boost_system
  gflags
  glog
)
cpp_library(varz_components)

