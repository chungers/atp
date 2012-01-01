# Zmq
#

message(STATUS "ZeroMQ =======================================================")

add_definitions('-fPIC')

##########################################
# Zmq library
set(zmq_components_incs
  ${GEN_DIR}
  ${SRC_DIR}
)
set(zmq_components_srcs
  Reactor.cpp
  Publisher.cpp
)
set(zmq_components_libs
  boost_system
  boost_thread
  gflags
  glog
  varz_components
  zmq
)
cpp_library(zmq_components)

##########################################
# Reactor test client / server
set(zmq_reactor_incs
  ${GEN_DIR}
  ${SRC_DIR}
)
set(zmq_reactor_srcs
  reactor_main.cpp
)
set(zmq_reactor_libs
  boost_system
  gflags
  glog
  zmq_components
)
cpp_executable(zmq_reactor)
