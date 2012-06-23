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
  Subscriber.cpp
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

