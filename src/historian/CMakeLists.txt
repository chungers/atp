
message(STATUS "Historian ====================================================")

add_definitions('-fPIC')

##########################################
# Historian
set(historian_components_incs
  ${GEN_DIR}
  ${SRC_DIR}
)
set(historian_components_srcs
  historian.cpp
)
set(historian_components_libs
  historian_proto
  ib_proto
  protobuf-lite
  leveldb
)
cpp_library(historian_components)

##########################################
# Historian
set(historian_incs
  ${GEN_DIR}
  ${SRC_DIR}
)
set(historian_srcs
  historian_main.cpp
)
set(historian_libs
  gflags
  glog
  historian_components
  zmq
  varz_components
)
cpp_executable(historian)
