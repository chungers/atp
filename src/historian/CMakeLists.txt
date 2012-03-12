
message(STATUS "Historian ====================================================")

add_definitions('-fPIC')

##########################################
# Historian
set(historian_components_incs
  ${GEN_DIR}
  ${SRC_DIR}
)
set(historian_components_srcs
  Db.cpp
  DbReactorStrategy.cpp
  DbReactorClient.cpp
)
set(historian_components_libs
  proto_components
  leveldb
  zmq_components
)
cpp_library(historian_components)

##########################################
# Historian - DB command line client
set(hc_incs
  ${GEN_DIR}
  ${SRC_DIR}
)
set(hc_srcs
  Db_main.cpp
)
set(hc_libs
  gflags
  glog
  historian_components
  varz_components
  zmq_components
)
cpp_executable(hc)


##########################################
# Historian - DB ZMQ reactor
set(hz_incs
  ${GEN_DIR}
  ${SRC_DIR}
)
set(hz_srcs
  DbReactor_main.cpp
)
set(hz_libs
  gflags
  glog
  historian_components
  varz_components
  zmq_components
)
cpp_executable(hz)

##########################################
# Historian - DB ZMQ reactor client
set(hzc_incs
  ${GEN_DIR}
  ${SRC_DIR}
)
set(hzc_srcs
  DbReactorClient_main.cpp
)
set(hzc_libs
  gflags
  glog
  historian_components
  varz_components
  zmq_components
)
cpp_executable(hzc)
