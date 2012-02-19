
message(STATUS "Historian ====================================================")

add_definitions('-fPIC')

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
  historian_proto
  ib_proto
  protobuf-lite
  leveldb
  zmq
  varz_components
)
cpp_executable(historian)
